/*
    Target: Teensy 4.1 based Tympan Rev F
    This code is the Tympan side of a menchmarking tool
    See nRF52840_SerialEcho.ino for the other half of the tool

    Originally made by Chip Audette for Creare
    Modified by Joel Murphy, Flywheel Lab, for Creare

    This code establishes a serial connection on Serial7 to com with the nRF52
    The serial port is set up to read in bytes until it encounters a '\n' char
    The terminating char could be different, for example any of the ASCII control chars, or 0x00

    Use this and the other half to benchmark the UART
    Make sure that this nRF_UART_BAUD is the same as the other side
    Make sure that the USB_UART_BAUD is well above the nRF_UART_BAUD to keep it from interfering??

*/

#define UART_MESSAGE_LENGTH 256
#define GREEN_LED 15
#define RED_LED 16
// change these here for tidy
#define nRF_UART_BAUD 230400
#define USB_UART_BAUD 1000000

uint8_t uartOutBuffer[UART_MESSAGE_LENGTH];
int uartOutBufferCounter = 0;

char from_nRF[UART_MESSAGE_LENGTH];
size_t from_nRFbyteCounter;
bool countIncommingBytes = true;
bool ledState = false;
unsigned long toggleTime = 600;
unsigned long lastToggle;

void printHelp(void) {
  Serial.println("Tympan Help:");
  Serial.println("    : h : print this help");
  Serial.println("    : All characters (including 'h') are sent to BLE");
    Serial.print("Serial7 baud rate: "); Serial.println(nRF_UART_BAUD);
}

void setup() {
  pinMode(RED_LED,OUTPUT); digitalWrite(RED_LED,ledState);
  pinMode(GREEN_LED,OUTPUT); digitalWrite(GREEN_LED,!ledState);
  Serial.begin(USB_UART_BAUD); //1M baud USB Serial to the PC
  Serial7.begin(nRF_UART_BAUD); 
  delay(2000);
  Serial.println("Tympan Serial Echo: starting...");
  uartOutBufferCounter = 0;
  printHelp();
  lastToggle = millis();
}

void loop() {

  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'h'){ printHelp(); }
    Serial.println("Tympan sending to nRF: " + String(c));
    Serial7.write(c);  // print(c);
  }

  // echo characters from Serial7 to the USB serial
  if (Serial7.available()) {
    from_nRF[from_nRFbyteCounter] = Serial7.read();
    if(from_nRF[from_nRFbyteCounter] == '\n'){
      from_nRFbyteCounter--; // remove the '\n' so we only save the data. Could set to NULL for string compatability?
      Serial.print("Tympan Received ");
      Serial.print(from_nRFbyteCounter); // This is the number of data bytes that we received.
      Serial.println(" bytes from nRF");
      for(size_t s=0; s<from_nRFbyteCounter; s++){
        Serial.print(from_nRF[s]);
      }
      Serial.println(); // break the line
      from_nRFbyteCounter = 0;  // reset the counter
    } else {
      from_nRFbyteCounter++;  // increment through the buffer
    }
  }

  toggleLEDs(millis());

}

void toggleLEDs(unsigned long m){
  if(m > lastToggle+toggleTime){
    lastToggle = m;
    ledState = !ledState;
    digitalWrite(GREEN_LED,ledState);
    digitalWrite(RED_LED, !ledState);
    digitalWrite(13,ledState);
  }
}


/*
The following is from the serial port on Arduino when you run this and send 't'

Tympan Serial Echo: starting...
Tympan Help:
    : h : print this help
    : All characters (including 'h') are sent to BLE
Serial7 baud rate: 230400
Tympan sending to nRF: t
Tympan Received 28 bytes from nRF
nRF52840 Serial1 Received: t
Tympan Received 256 bytes from nRF
0123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567012345670123456701234567
Tympan Received 34 bytes from nRF
nRF52840 uartOutBufferCounter: 256

*/
