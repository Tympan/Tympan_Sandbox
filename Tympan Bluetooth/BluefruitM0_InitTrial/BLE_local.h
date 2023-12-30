
char foo_digit2ascii(uint8_t digit) {
  return ( digit + ((digit) < 10 ? '0' : ('A'-10)) );
}

int printByteArray(uint8_t const bytearray[], int size)
{
  Serial.print("BLE_local: printByteArray: ");
  while(size--)
  {
    uint8_t byte = *bytearray++;
    Serial.write( foo_digit2ascii((byte & 0xF0) >> 4) );
    Serial.write( foo_digit2ascii(byte & 0x0F) );
    if ( size!=0 ) Serial.write('-');
  }
  Serial.println();

  return (size*3) - 1;
}

int printString(const String &str) {
  char c_str[str.length()];
  str.toCharArray(c_str, str.length());
  return printByteArray((uint8_t *)c_str,str.length());
}

class BLE_local {
  public:
    Adafruit_BluefruitLE_SPI *BLE_TX_ptr = NULL;
    int32_t ble_char_id = 0;
    
    int send(const char c) {
      return send(String(c));
    }
    int send(const String &str) {
      if (BLE_TX_ptr == NULL) {
        Serial.println("BLE_local.send: *** ERROR ***: BLE_TX_ptr is NULL.  Cannot send.");
        Serial.println("    : Returning without sending str.");
        return 0;
      }

      //Serial.println("BLE_local: send(string)...printing string...");
      printString(str);
      

      BLE_TX_ptr->print("AT+GATTCHAR=");
      BLE_TX_ptr->print(ble_char_id);
      BLE_TX_ptr->print(",");
      BLE_TX_ptr->println(str);
      //BLE_TX_ptr->update(0); //added WEA DEC 30, 2023
      if (! BLE_TX_ptr->waitForOK() ) {
        Serial.println(F("BLE_local: send: Failed to send?"));
      }
      //ble.println(str, HEX);
      return str.length();
    }
    int sendString(const String &s) {
      return sendString(s,false);
    }
    int sendString(const String &s, bool print_debug) {
      int ret_val = send(s);
      if (ret_val == s.length() ) {
        return ret_val;  //if send() returns non-zero, send the length of the transmission
      } else {
        if (print_debug) Serial.println("BLE: sendString: ERROR sending!  ret_val = " + String(ret_val) + ", string = " + s);
      }
      return 0;   //otherwise return zero (as a form of error?)
    }
    
    int sendMessage(const char c) {
      return sendMessage(String(c));
    }
    
    int sendMessage(const String &orig_s) {
      String s = orig_s;
      const int payloadLen = 18; //was 19
      size_t sentBytes = 0;

      Serial.println("BLE_local: sendMessage: commanded to send: " + orig_s);
  
      String header;
      header.concat('\xab'); header.concat('\xad'); header.concat('\xc0');header.concat('\xde'); // ABADCODE, message preamble
      header.concat('\xff');       // message type
  
      // message length
      if (s.length() >= (0x4000 - 1))  { //we might have to add a byte later, so call subtract one from the actual limit
          Serial.println("BLE: Message is too long!!! Aborting.");
          return 0;
      }
      int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
      header.concat((char)highByte(lenBytes));
      header.concat((char)lowByte(lenBytes));
  
      //check to ensure that there isn't a NULL or a CR in this header
      if ((header[6] == '\r') || (header[6] == '\0')) {
          //add a character to the end to avoid an unallowed hex code code in the header
          Serial.println("BLE: sendMessage: ***WARNING*** message is being padded with a space to avoid its length being an unallowed value.");
          s.concat(' '); //append a space character
  
          //regenerate the size-related information for the header
          int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
          header[5] = ((char)highByte(lenBytes));
          header[6] = ((char)lowByte(lenBytes));
      }

      if (0) {
        Serial.println("BLE_local: sendMessage: ");
        Serial.println("    : Header length = " + String(header.length()) + " bytes");
        Serial.println("    : header (String) = " + header);
        Serial.print(  "    : header (HEX) = ");
        for (int i=0; i<header.length(); i++) Serial.print(header[i],HEX);
        Serial.println();
        Serial.println("    : Message: '" + s + "'");
      }
  
      //send the packet with the header information
      //Question: is "buf" actually used?  It doesn't look like it.  It looks like only "header" is used.
      char buf[21];  //was 16, which was too small for the 21 bytes that seem to be provided below
      sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", header.charAt(0), header.charAt(1), header.charAt(2), header.charAt(3), header.charAt(4), header.charAt(5), header.charAt(6));
      int a = sendString(header); //for V5.5, this will return an error if there is no active BT connection
      //Serial.println("    : return value a = " + String(a));
      if (a != 7)  {
        //With v5, it returns an error if there is no BT connection.
        //Checking if there is a BT connection really slows down this transaction.
        //So, let's just suppress this error message for V5 and only allow for V7.
        //Then, for V7, it automatically tracks the connection status via having a
        //valid BLE_id_num.  So, only print an error message for V7 if there is indeed
        //a valid BLE_id_num.
        
        //if ((BC127_firmware_ver > 6) && (BLE_id_num > 0)) {
          Serial.println("BLE: sendMessage: Error (a=" + String(a) + ") in sending message: " + String(header));  
        //} 
        //if we really do get an error, should we really try to transmit all the packets below?  Seems like we shouldn't.
      }
      
  
      //break up String into packets
      int numPackets = ceil(s.length() / (float)payloadLen);
      for (int i = 0; i < numPackets; i++)  {
          String bu = String((char)(0xF0 | lowByte(i)));
          bu.concat(s.substring(i * payloadLen, (i * payloadLen) + payloadLen));
          sentBytes += (sendString(bu) - 1); 
          delay(4); //20 characters characcters at 9600 baud is about 2.1 msec...make at least 10% longer (if not 2x longer)
      }
  
      //Serial.print("BLE: sendMessage: sentBytes = "); Serial.println((unsigned int)sentBytes);
      if (s.length() == sentBytes)
          return sentBytes;
  
      return 0;
    }
};
