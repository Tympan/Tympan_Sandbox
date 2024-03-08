// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This code will run on the **nRF52** to interpret the AT-style commands coming in over the wired Serial1 link
// from the Tympan.   This code will likely be part of our nRF52 firmware.
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef nRF52_AT_API_H
#define nRF52_AT_API_H

#include <bluefruit.h>  //gives us the global "Bluefruit" class instance

//declared in BLE_Stuff.h or TympanBLE.h.  Should we move them inside here?  
extern bool bleConnected;
extern char BLEmessage[];
extern void startAdv(void);
extern const char versionString[];

//running on the nRF52, this interprets commands coming from the hardware serial and send replies out to the BLE
#define nRF52_AT_API_N_BUFFER 512
class nRF52_AT_API {
  public:
    nRF52_AT_API(BLEUart_Tympan *_bleuart, HardwareSerial *_ser_ptr) : ble_ptr(_bleuart) , serial_ptr(_ser_ptr) {}
    
    virtual int processSerialCharacter(char c);  //here's the main entry point to the AT message parsing
    virtual int lengthSerialMessage(void);
    virtual int processSerialMessage(void);

  protected:
    BLEUart_Tympan *ble_ptr = NULL;
    HardwareSerial *serial_ptr = &Serial1;
    char EOC = '\r'; //all commands (including "SEND") from the Tympan must end in this character

    //circular buffer for reading from Serial
    char serial_buff[nRF52_AT_API_N_BUFFER];
    int serial_read_ind = 0;
    int serial_write_ind = 0;

    bool compareStringInSerialBuff(const char* test_str, int n);  
    int processSetMessageInSerialBuff(void);  
    int processGetMessageInSerialBuff(void);
    int setBleNameFromSerialBuff(void);
    int bleWriteFromSerialBuff(void);
    void debugPrintMsgFromSerialBuff(void);
    void debugPrintMsgFromSerialBuff(int, int);
    void sendSerialOkMessage(void);
    void sendSerialOkMessage(const char* reply_str);
};


//here's the main entry point to the AT message parsing
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
  if (len >= test_n_char) {  //is the current message long enough for this test?
    if (compareStringInSerialBuff("SEND ",test_n_char)) {  //does the current message start this way
      serial_read_ind += test_n_char; //increment the reader index for the serial buffer
      Serial.print("nRF52_AT_API: recvd: SEND "); debugPrintMsgFromSerialBuff(); Serial.println();
      bleWriteFromSerialBuff();
      ret_val = 0; 
    }
  }

  test_n_char = 4; //how long is "SET "
  if (len >= test_n_char) {
    if (compareStringInSerialBuff("SET ",test_n_char)) {  //does the current message start this way
      serial_read_ind += test_n_char; //increment the reader index for the serial buffer
      Serial.print("nRF52_AT_API: recvd: SET "); debugPrintMsgFromSerialBuff(); Serial.println();
      processSetMessageInSerialBuff();
      ret_val = 0;
    }
  }

  test_n_char = 4; //how long is "GET "
  if (len >= test_n_char) {
    if (compareStringInSerialBuff("GET ",test_n_char)) {  //does the current message start this way
      serial_read_ind += test_n_char; //increment the reader index for the serial buffer
      Serial.print("nRF52_AT_API: recvd: GET "); debugPrintMsgFromSerialBuff(); Serial.println();
      ret_val = processGetMessageInSerialBuff();
    }
  } 

  test_n_char = 7; //how long is "VERSION"
  if (len >= test_n_char) {
    if (compareStringInSerialBuff("VERSION",test_n_char)) {  //does the current message start this way
      serial_read_ind += test_n_char; //increment the reader index for the serial buffer
      Serial.println("nRF52_AT_API: recvd: VERSION"); Serial.print("nRF52_AT_API: returning: "); Serial.println(versionString);
      sendSerialOkMessage(versionString);
      ret_val = 0;
    }
  } 
  // serach for another command
  //   anything?

  // give error message if message isn't known
  if (ret_val != 0) {
    if (DEBUG_VIA_USB) {
      Serial.print("nRF52_AT_API: *** WARNING ***: msg not understood: ");
      debugPrintMsgFromSerialBuff();
      Serial.println();
    }
    serial_read_ind = serial_write_ind;  //remove the message
  }

  //return
  serial_read_ind = serial_write_ind;  //remove any remaining message
  return ret_val;
}

int nRF52_AT_API::processSetMessageInSerialBuff(void) {
  int test_n_char;
  int ret_val = -1;
  
  test_n_char = 5; //length of "NAME="
  if (compareStringInSerialBuff("NAME=",test_n_char)) {
    serial_read_ind += test_n_char; //increment the reader index for the serial buffer
    if (DEBUG_VIA_USB) {
      Serial.print("nRF52_AT_API: processSetMessageInSerialBuff: setting NAME to ");
      debugPrintMsgFromSerialBuff();
      Serial.println();
    }
    ret_val = setBleNameFromSerialBuff();
    serial_read_ind = serial_write_ind;  //remove the message
    sendSerialOkMessage();
  }

  // serach for another command
  //   anything?

  // give error message if message isn't known
  if (ret_val != 0) {
    if (DEBUG_VIA_USB) {
      Serial.print("nRF52_AT_API: *** WARNING ***: SET msg not understood: ");
      debugPrintMsgFromSerialBuff();
    }
    serial_read_ind = serial_write_ind;  //remove the message
  }
  return ret_val;
}

int nRF52_AT_API::processGetMessageInSerialBuff(void) {
  int test_n_char;
  int ret_val = -1;
  
  //Serial.print("nRF52_AT_API::processGetMessageInSerialBuff: interpreting ");  debugPrintMsgFromSerialBuff(); Serial.println();
  //Serial.println("nRF52_AT_API::processGetMessageInSerialBuff: compareStringInSerialBuff('NAME',test_n_char) = " + String(compareStringInSerialBuff("NAME",test_n_char)));

  test_n_char = 4; //length of "NAME"
  if (compareStringInSerialBuff("NAME",test_n_char)) {
    if ((serial_read_ind+test_n_char==serial_write_ind) || (serial_buff[serial_read_ind + test_n_char] == EOC)) {
        //get the current BLE name 
        static const uint16_t n_len = 64;
        char name[n_len];
        uint32_t act_len = Bluefruit.getName(name, n_len);
        name[act_len]='\0';  //the BLE module does not appear to null terminate, so let's do it ourselves
        //if (foo_ret_val == NRF_SUCCESS) {
        if ((act_len > 0) || (act_len < n_len)) {
          //Serial.println("nRF52_AT_API: DEBUG: processGetMessageInSerialBuff: Good!  name = " + String(name));
          ret_val = 0;  //it's good!
        } else {
          //Serial.println("nRF52_AT_API: DEBUG: processGetMessageInSerialBuff: Bad!  ret_val = " + String(ret_val) + ", name = " + String(name));
        }

        //send the name back via the serial link
        //serial_ptr->println(name);
        sendSerialOkMessage(name);
        Serial.print("nRF52_AT_API: recvd: SET NAME "); Serial.println(name);
    }
    serial_read_ind = serial_write_ind;  //remove the message
  }  

  test_n_char = 7; //length of "VERSION"
  if (compareStringInSerialBuff("VERSION",test_n_char)) {
    if ((serial_read_ind+test_n_char==serial_write_ind) || (serial_buff[serial_read_ind + test_n_char] == EOC)) {
      sendSerialOkMessage(versionString);
    }
    serial_read_ind = serial_write_ind;  //remove the message
  }  

  // serach for another command
  //   anything?

  // give error message if message isn't known
  if (ret_val != 0) {
    //Serial.print("nRF52_AT_API: *** WARNING ***: GET msg not understood: ");
    //debugPrintMsgFromSerialBuff();
    serial_read_ind = serial_write_ind;  //remove the message
  }

  return ret_val;
}

//returns -1 if failed
int nRF52_AT_API::setBleNameFromSerialBuff(void) {
  int end_ind = serial_read_ind;
  while ((end_ind < serial_write_ind) && (serial_buff[end_ind] != EOC)) end_ind++; //see if there is an end-of-command character
  if (end_ind != serial_write_ind) {
    static const int max_len_name = 16;  //16 character max?? 
    char new_name[max_len_name+1]; //11 characters plus the null termination
    int given_len_name = end_ind - serial_read_ind;
    int new_len = min(max_len_name,given_len_name);  //choose the smaller of the given length or the max length
    if (new_len > 0) {
      //copy the name to a null-terminated buffer
      for (int i=0;i < new_len; i++) new_name[i] = serial_buff[serial_read_ind + i]; //copy from the serial buffer
      new_name[new_len] = '\0'; //add the null termination
      if (DEBUG_VIA_USB) Serial.println("nRF52_AT_API: setBleNameFromSerialBuff: new_name = " + String(new_name));

      //stop any advertising
      //Bluefruit.Advertising.stop();

      //send the new name to the module
      Bluefruit.setName(new_name);

      //restart advertising
      //startAdv();

      return 0;
    } else {
      //length of new name was zero.  So assume this was a fail?
    }
  } else {
    //didn't find an EOC character.  This shouldn't happen.
  }
  return -1;
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
  }
  return -1;
}

void nRF52_AT_API::sendSerialOkMessage(const char* reply_str) {
  serial_ptr->print("OK ");
  serial_ptr->print(reply_str);
  serial_ptr->println(EOC);
}
void nRF52_AT_API::sendSerialOkMessage(void) {
  serial_ptr->print("OK ");
  serial_ptr->println(EOC);
}

void nRF52_AT_API::debugPrintMsgFromSerialBuff(void) {
  debugPrintMsgFromSerialBuff(serial_read_ind, serial_write_ind);
}

void nRF52_AT_API::debugPrintMsgFromSerialBuff(int start_ind, int end_ind) {
  while (start_ind != end_ind) {
    if (DEBUG_VIA_USB) Serial.print(serial_buff[start_ind]);
    start_ind++; 
    if (start_ind >= nRF52_AT_API_N_BUFFER) start_ind = 0;
  }  
}



#endif
