
class BLE_local {
  public:
    BLECharacteristic *BLE_TX_ptr = NULL;
    int send(const char c) {
      send(String(c));
    }
    int send(const String &str) {
      if (BLE_TX_ptr == NULL) {
        Serial.println("BLE_local.send: *** ERROR ***: BLE_TX_ptr is NULL.  Cannot send.");
        Serial.println("    : Returning without sending str.");
        return 0;
      }
      uint8_t tx_array[str.length()];
      for (int i=0; i<str.length(); i++) tx_array[i] = (uint8_t)str[i];
      BLE_TX_ptr->setValue(tx_array, str.length());
      BLE_TX_ptr->notify();
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
      const int payloadLen = 19;
      size_t sentBytes = 0;
  
      String header;
      header = "\xab\xad\xc0\xde"; // ABADCODE, message preamble
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
          //Serial.println("BLE: sendMessage: ***WARNING*** message is being padded with a space to avoid its length being an unallowed value.");
          s.concat(' '); //append a space character
  
          //regenerate the size-related information for the header
          int lenBytes = (s.length() << 1) | 0x8001; //the 0x8001 is avoid the first message having the 2nd-to-last byte being NULL
          header[5] = ((char)highByte(lenBytes));
          header[6] = ((char)lowByte(lenBytes));
      }
  
      //Serial.println("BLE: sendMessage: Header (" + String(header.length()) + " bytes): '" + header + "'");
      //Serial.println("BLE: Message: '" + s + "'");
  
      //send the packet with the header information
      //Question: is "buf" actually used?  It doesn't look like it.  It looks like only "header" is used.
      char buf[21];  //was 16, which was too small for the 21 bytes that seem to be provided below
      sprintf(buf, "%02X %02X %02X %02X %02X %02X %02X", header.charAt(0), header.charAt(1), header.charAt(2), header.charAt(3), header.charAt(4), header.charAt(5), header.charAt(6));
      int a = sendString(header); //for V5.5, this will return an error if there is no active BT connection
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
