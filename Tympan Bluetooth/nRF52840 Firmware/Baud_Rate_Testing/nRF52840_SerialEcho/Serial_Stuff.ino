/*
 *  Serial port control and communication
 */

 void serialEvent(){

    if(Serial.available()){
      char inChar = Serial.read();
      switch(inChar){
        case 'a':
          Serial.println("Stop fading");
          LEDsOff();
          ledToFade = -1;
          break;
        case 'r':
          Serial.println("Fade red LED 0");
          LEDsOff();
          ledToFade = red;
          fadeValue = FADE_MIN;
          break;
       case 'g':
          Serial.println("Fade green LED 1");
          LEDsOff();
          ledToFade = green;
          fadeValue = FADE_MIN;
          break;
       case 'b':
          Serial.println("Fade blue LED 2");
          LEDsOff();
          ledToFade = blue;
          fadeValue = FADE_MIN;
          break;
        case 't':
          Serial.println("Sending buffer");
          for(int i=0; i<uartOutBufferCounter; i++){
            Serial1.write(uartOutBuffer[i]); 
          }
          Serial1.println();
          Serial1.print("nRF52840 uartOutBufferCounter: "); Serial1.println(uartOutBufferCounter);
          break;
        case '?':
          printHelp();
          break;
        default:
      	  Serial.print(inChar); Serial.print(" not recognised\n");
          break;
      }
    }
 }
 
// void printHelp(HardwareSerial *serial_p) {
//   serial_p->print("nRF52840 Firmware: "); 
//   serial_p->print(versionString);
//   serial_p->print(", BLE name: "); 
//   serial_p->print(deviceName);
//   serial_p->println();
// }

void printTympanHelp(){
  Serial1.print("nRF52840 Firmware: "); 
  Serial1.print(versionString);
  Serial1.print(", BLE name: "); 
  Serial1.print(deviceName);
  Serial1.println();
}

void printHelp(){
  if(usingSerial){
    Serial.println("\nnRF52840_SerialEcho.ino ");
    Serial.println(versionString);
    Serial.print("BLE Address: "); Serial.println(deviceName);
    Serial.println("Serial Keyboard Commands");
    Serial.println("Send 'a' to stop fading");	
    Serial.println("Send 'r' to fade red LED 0");	
    Serial.println("Send 'g' to fade green LED 1");
    Serial.println("Send 'b' to fade blue LED 2");
    Serial.println("Send '?' to print this help");
    Serial.println("Please use Bluetooth to connect to BLE Address");
    Serial.println("Once connected, use bleuart and send '?' to receive help");
    Serial.println("I will echo serial input from Serial1");
    Serial.println("If Serial1 sends 't' I will send Serial1 the outBuffer");
    Serial.print("Serial1 baud rate: "); Serial.println(nRF_UART_BAUD);
  }

  printedHelp = true;
}


