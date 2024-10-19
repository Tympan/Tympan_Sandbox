 #!/usr/bin/env python
"""Provides serial communciation to the Tympan

MIT License

Functionality
- 

"""
from threading import Thread
import queue
from time import sleep
from time import time
from serial.tools import list_ports
from serial import Serial
from serial import SerialException

class TympanSerial:
    """Manages the serial port to the Tympan
    """
    def __init__(self):
        self.all_ports = []
        self.tymp_ports = []
        self.port_h = None
        self.err = None

        # Initialize an RX buffer queue
        self.rx_buffer_q = QueueWithPeek()                              #Extend queue with a peek function.
        
    def list_ports(self, target_vid_list=[5824], print_flag=False):
        """Updates the list of available ports and filters them by VID
        - VID defaults to [5824], which was tested with Rev-E and Rev-F.
        
        Args:
            target_vid_list (list, optional): VIDs to filter Tympan ports. 
            print_flag (bool, optional): Set to print out available ports. Defaults to False.
        """
        # Retrieve available COM ports
        self.all_ports = sorted(list_ports.comports(include_links=False))

        # IF VID provided, filter the list of ports
        self.tymp_ports = []
        if target_vid_list:
            self.tymp_ports = [p for p in self.all_ports if p.vid in target_vid_list]

        # Print available serial ports
        if print_flag and self.tymp_ports:
            print("Tympan ports:")
            for p in self.tymp_ports:
                print("{:<32}".format(p.description))

            # Print otjher serial ports
        if print_flag and self.all_ports:
            print("Other ports:")
            other_ports = [p for p in self.all_ports if not(p.vid in target_vid_list)]
            for p in other_ports:
                print("{:<32}".format(p.description))

    def connect(self, port_desc="", num_retries=5):
        """Opens specified "port", or if port left blank, a Tympan port if only one is listed 

        Args:
            port_desc (str, optional): Port to open. Or leave empty to open the only Tympan port . Defaults to "".
            num_retries (int, optional): Number of retries for serial connection. Defaults to 5.
        
        Returns:
            err: error, or if no error, then []
            
        """
        port_to_open = None
        self.port_h = None
        err = None

        # Update list of available ports
        self.list_ports()
        try:
            # IF port is specified, set it to open
            if port_desc != "":
                port_to_open = [p for p in self.all_ports if port_desc in p.description][0]

            # ELSE IF no Tympan ports are found, error
            elif len(self.tymp_ports)<1:
                err = "Err: No Tympan ports found."
                print(err)
                print("Available ports: ")
                for p in self.all_ports:
                    print("{:<32}".format(p.description))

            # ELSE IF one Tympan port is found, set it to open
            elif len(self.tymp_ports)==1:
                port_to_open = self.tymp_ports[0]

            # ELSE IF more than one Tympan port is found, error
            else:
                err = "Err: Multiple Tympan ports found. Please specify"
                print(err)
                print("Tympan ports: ")
                for p in self.tymp_ports:
                    print("{:<32}".format(p.description))

        except SyntaxError as err:
            print(err)

        except Exception as err:
            print(err)


        # Open the port
        connected_flag = False

        if port_to_open:
            while ( not (connected_flag) and (num_retries>0) ):
                try:
                    self.port_h = Serial(port=port_to_open.device, baudrate=115200, timeout=0.5)

                    if self.port_h.is_open :
                        print("Opened: ", port_to_open.device)
                        connected_flag = True

                except SerialException as err:
                    print(err)
                    print("Closing ", port_to_open.device)

                    # close and try again
                    if self.port_h:
                        self.port_h.close()
                
                    num_retries -= 1
                    sleep(1)

        # Start thread for reading serial
        if (err is None) and connected_flag:
            #print("Starting Rx thread")
            self.start_rx_thread()

        return err

    def send_char(self, command_char, eol_str='\n'):
        """_summary_

        Args:
            command_char (_type_): single character to send
            EOL (str, optional): end of line character to send last. Defaults to '\n'.

        Returns:
            _type_: _description_
        """
        num_bytes = self.port_h.write(bytes(command_char + eol_str, 'utf-8'))

        # Give some time for the device to respond
        sleep(0.05)

        return num_bytes

    def send_string(self, command_str, eol_str='\n'):
        num_bytes = self.port_h.write(bytes(command_str + eol_str, 'utf-8'))
        sleep(0.05)

    def read_line(self, timeout_s=0.5, eof_str='\n'):
        found_eol_flag = False
        buf = bytearray()

        # Convert EOF to byte
        eof_byte = bytes(eof_str, 'utf-8')

        # Start timer
        last_time_s = time()

        # Keep reading bytes until timeout or EOL
        while ( (time()<(last_time_s+timeout_s)) and not found_eol_flag ):
            # Peek at Rx buffer
            peek_buf = self.peek_rx_buffer()
            
            # IF EOL found, read buffer up to that point (includeing the EOL).
            if eof_byte in peek_buf:
                buf += self.get_rx_buffer(timeout_s=timeout_s, num_bytes=peek_buf.index(eof_byte) + len(eof_byte))
                found_eol_flag = True

        if(time()>=last_time_s+timeout_s):
            print("Error: timeout")

        # Decode buffer to u
        return buf.decode("utf-8")
    
    def read_all(self):
        buf = self.get_rx_buffer()

        # Decode buffer to u
        return (buf.decode("utf-8"))

    def peek_rx_buffer(self):
        buf = self.rx_buffer_q.peek()
        buf = b''.join(buf)               # Convert deque object to bytes
        buf = bytearray(buf)                # Covnert bytes to bytearray
        return buf

    def get_rx_buffer(self, timeout_s=0.5, num_bytes=None):
        buf = bytearray()

        # Start timer
        last_time_s = time()

        # If num_bytes not specified, get all queued bytes
        if num_bytes is None:
            while ( self.rx_buffer_q.qsize()>0 and (time()<last_time_s+timeout_s) ):
                buf+=self.rx_buffer_q.get()

        # Else get specified number of bytes from buffer
        else:
            #print('Waiting for ', num_bytes, 'bytes')
            #print("Buffer has ", len(self.rx_buffer_q.peek()), "bytes")
            #print("Receiving Serial", end =" ")

            while ( (num_bytes>0) and (time()<last_time_s+timeout_s) ):
                # Get 1 byte
                if self.rx_buffer_q.qsize()>0:
                    tmp_buf = self.rx_buffer_q.get()  #hard-code timeout=1-sec between bytes
                    buf+=tmp_buf

                    # update num_bytes waiting for
                    num_bytes-=len(tmp_buf)

        return buf

    def read_rx_thread(self):
        while self.rx_thread.is_alive:
            if self.port_h.in_waiting > 0:
                data = self.port_h.read(size=1024)
                self.rx_buffer_q.put(data)

    def start_rx_thread(self):
        self.rx_thread = Thread(target=self.read_rx_thread)
        self.rx_thread.daemon = True
        self.rx_thread.start()

    def stop_rx_thread(self):
        self.rx_thread.running = False
        self.rx_thread.join()

class QueueWithPeek(queue.Queue):
    """ Adds peek to Queue so that the queue can be examined without removing elements

    Args:
        queue (_type_): _description_
    """
    def peek(self):
        with self.mutex:
            return(self.queue)
