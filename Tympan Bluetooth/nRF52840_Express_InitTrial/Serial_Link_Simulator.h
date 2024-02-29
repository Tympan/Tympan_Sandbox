
// /////////////////////////////////////////////////////////////////////////////////////////
//
// This code is only needed because we're pretending that there is a Tympan, when in reality
// we're just simulating some of its functions here as part of the nRF device.  In other words,
// when we are physically wiring the nRF to the Tympan, this code in this file won't be
// necessary at all.
//
// ////////////////////////////////////////////////////////////////////////////////////////

#ifndef Serial_Simulator_h
#define Serial_Simulator_h

#include "nRF52_BLEUart_Tympan.h"

#define SERIAL_N_BUFFER 1024
class Serial_Simulator : public HardwareSerial {
  public:
    Serial_Simulator(void) {}
    
    virtual void begin(unsigned long value) {};
    virtual void begin(uint32_t baud, uint16_t format=0) {};
    virtual void end() {};
    virtual void flush(void) {};
    virtual int availableForWrite(void) {return true;}

    //using Print::write;
    operator bool()			{ return true; }

  	// Transmit a single byte
  	size_t write(unsigned long n) { return write((uint8_t)n); }
	  // Transmit a single byte
	  size_t write(long n) { return write((uint8_t)n); }
	  // Transmit a single byte
	  size_t write(unsigned int n) { return write((uint8_t)n); }
	  // Transmit a single byte
	  size_t write(int n) { return write((uint8_t)n); }

    //make these better in the future
    size_t write(const uint8_t *buffer, size_t size) { 
      for (int i=0; i < size; i++) write(buffer[i]);
      return size; 
    }
    virtual size_t write(uint8_t n) {
      buff[write_ind] = (char)n;
      write_ind++;
      if (write_ind >= n_buff) write_ind = 0;
      return 1;
    };

    // void update(void) {
    //    int success = -1;
    //    while (ble_ptr->available()) {
    //       buff[write_ind] = ble_ptr->read();
    //       write_ind++;
    //       if (write_ind >= n_buff) write_ind = 0;
    //    }
    // }

    int available(void) {
      int n_avail = 0;
      if (read_ind > write_ind) {
        n_avail = ((SERIAL_N_BUFFER-1) - read_ind) + write_ind;
      } else {
        n_avail = write_ind - read_ind;
      }
      return n_avail;
    }

    int read(void) {
      if (read_ind != write_ind) {
        char val = buff[read_ind];
        read_ind++;
        if (read_ind >= n_buff) read_ind = 0;
        return (int) val;
      }
      return -1;
    }

    int peek(void) {
      if (read_ind != write_ind) {
        char val = buff[read_ind];
        return (int) val;
      }
      return -1;
    }

  private:
    //BLEUart_Tympan *ble_ptr = NULL;
    const int n_buff = SERIAL_N_BUFFER;
    char buff[SERIAL_N_BUFFER];
    int read_ind=0;
    int write_ind=0;

};




#endif