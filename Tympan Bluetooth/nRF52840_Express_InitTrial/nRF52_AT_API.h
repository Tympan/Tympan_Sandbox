// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This code will run on the **nRF52** to interpret the AT-style commands coming in over the wired Serial1 link
// from the Tympan.   This code will likely be part of our nRF52 firmware.
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef nRF52_AT_API_H
#define nRF52_AT_API_H

//declared in BLE_Stuff.h or TympanBLE.h.  Should we move them inside here?  
extern bool bleConnected;
extern char BLEmessage[];


//running on the nRF52, this interprets commands coming from the hardware serial and send replies out to the BLE
#define nRF52_AT_API_N_BUFFER 512
class nRF52_AT_API {
  public:
    nRF52_AT_API(BLEUart_Tympan *bleuart) : ble_ptr(bleuart) {}
    
    virtual int processSerialCharacter(char c);
    virtual int lengthSerialMessage(void);
    virtual int processSerialMessage(void);

  protected:
    BLEUart_Tympan *ble_ptr = NULL;
    char EOC = '\r';

    //circular buffer for reading from Serial
    char serial_buff[nRF52_AT_API_N_BUFFER];
    int serial_read_ind = 0;
    int serial_write_ind = 0;

    bool compareStringInSerialBuff(const char* test_str, int n);
    int bleWriteFromSerialBuff(void);
    void debugPrintMsgFromSerialMuff(void);
    void debugPrintMsgFromSerialMuff(int, int);
};

int nRF52_AT_API::processSerialCharacter(char c) {
  //look for carriage return
  if (c == EOC) {  //look for the end-of-command character
    processSerialMessage();
  } else {
    serial_buff[serial_write_ind] = c;
    serial_write_ind++;
    if (serial_write_ind >= nRF52_AT_API_N_BUFFER) serial_write_ind = 0;
  }
  return 0;
}

int nRF52_AT_API::lengthSerialMessage(void) {
  int len = 0;
  if (serial_read_ind > serial_write_ind) {
    len = ((nRF52_AT_API_N_BUFFER-1) - serial_read_ind) + serial_write_ind;
  } else {
    len = serial_write_ind - serial_read_ind;
  }
  return len;
}

bool nRF52_AT_API::compareStringInSerialBuff(const char* test_str, int n) {
  int ind = serial_read_ind;  //where to start in the read buffer
  for (int i=0; i < n; i++) {
    if (serial_buff[ind] != test_str[i]) return false;
    ind++;  //increment the read buffer
    if (ind >= nRF52_AT_API_N_BUFFER) ind = 0;  //wrap around the read buffer index, if necessary
  }
  return true;
}

int nRF52_AT_API::processSerialMessage(void) {
  int ret_val = -1;
  int len = lengthSerialMessage();     // how long is the message that was received

  //Serial.println("nRF52_AT_API::processSerialMessage: starting...");

  //test for the command "SEND "
  int test_n_char = 5;   //how long is "SEND "
  if (len > test_n_char) {  //is the current message long enough for this test?
    if (compareStringInSerialBuff("SEND ",test_n_char)) {  //does the current message start this way
      serial_read_ind += test_n_char;
      //Serial.println("nRF52_AT_API::processSerialMessage: SEND detected!");
      bleWriteFromSerialBuff();
      ret_val = 0; 
    }
  }
  
  // serach for another command
  //   anything?

  // give error message if message isn't known
  if (ret_val != 0) {
    Serial.print("nRF52_AT_API: *** WARNING ***: msg not understood: ");
    debugPrintMsgFromSerialMuff();
    serial_read_ind = serial_write_ind;  //remove the message
  }

  //return
  return ret_val;
}

int nRF52_AT_API::bleWriteFromSerialBuff(void) {
  //copy the message from the circular buffer to the straight buffer
  int counter = 0;
  while (serial_read_ind != serial_write_ind) {
    BLEmessage[counter] = serial_buff[serial_read_ind];
    counter++;
    serial_read_ind++;  
    if (serial_read_ind >= nRF52_AT_API_N_BUFFER) serial_read_ind = 0;
  }

  //if BLE is connected, fire off the message
  if (bleConnected) {
    ble_ptr->write( BLEmessage, counter );
    return counter;
  } else {
    return -1;
  }
}

void nRF52_AT_API::debugPrintMsgFromSerialMuff(void) {
  debugPrintMsgFromSerialMuff(serial_read_ind, serial_write_ind);
}

void nRF52_AT_API::debugPrintMsgFromSerialMuff(int start_ind, int end_ind) {
  while (start_ind != end_ind) {
    Serial.print(serial_buff[start_ind]);
    start_ind++; 
    if (start_ind >= nRF52_AT_API_N_BUFFER) start_ind = 0;
  }  
}

#endif
