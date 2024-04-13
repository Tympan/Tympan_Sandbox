/*

      TympanRadio_basefirmware.ino
      
      This is the most basic code base for flashing the nRF52840 BLE module on the Tympan Rev F
      * Bluetooth connection handlers
      * Generates unique advertising name specific to the module
      * Includes OTA DFU Service 
        >>  ALL TYMPAN nRF52  CODE MUST INCLUDE OTA DFU SERVICE  <<
        >>  BLEDfu  bledfu;  << after the #include list
        >>  bledfu.begin();  << within setup()
      * Basic comms over BLE to blink LEDs for testing coms pipeline
      * UART benchmarking with dummy data
      
      THIS FIRMWARE IS FOR FLASHING OVER J-LINK
      TARGETTING FOB BLANK MDBT50Q-1MV2 nRF52840 MODULE

 
    Made by Joel Murphy, Flywheel Lab, for Creare February 2024
    Updated by Joel Murphy to benchmark UART

    Target: nRF52840 radio module with Adafruit Express bootloader
    This code is the nRF52 side of a menchmarking tool
    See Tympan_SerialEcho.ino for the other half of the tool

    Originally made by Chip Audette for Creare
    Modified by Joel Murphy, Flywheel Lab, for Creare

    This code establishes a serial connection on Serial1 to com with the Tympan Teensy 4.1
    The serial port is set up to read in bytes until it encounters a '\n' char
    The terminating char could be different, for example any of the ASCII control chars, or 0x00
    
    The test buffer is sent when a 't' control char is recieved from the Tympan

    Use this and the other half to benchmark the UART
    Make sure that this nRF_UART_BAUD is the same as the other side
    Make sure that the USB_UART_BAUD is well above the nRF_UART_BAUD to keep it from interfering??

 */

#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "TympanBLE.h"

// change these here for tidy
#define nRF_UART_BAUD 230400
#define USB_UART_BAUD 1000000

BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble

#define UART_MESSAGE_LENGTH 256
uint8_t uartOutBuffer[UART_MESSAGE_LENGTH];
uint8_t dummyChar;
int uartOutBufferCounter = 0;

void setup() {

  Serial.begin(USB_UART_BAUD);   //1M baud USB Serial to the PC

  Serial1.setPins(0,1);   //our nRF wiring uses Pin0 for RX and Pin1 for TX
  Serial1.begin(nRF_UART_BAUD);  //Hardware UART serial to the Tympan

  uartOutBufferCounter = 0;
  
  for (int i=0; i<UART_MESSAGE_LENGTH; i++) {
    dummyChar = ('0' + (i%8)) & 0xFF; // count from 0 to 9 over and over
    uartOutBuffer[uartOutBufferCounter] = dummyChar; // prepare the dummy array
    uartOutBufferCounter++;
  }

  unsigned long t = millis();
  unsigned long timeOut = 5000L; // 1 second time out before we bail on a serial connection
  while (!Serial) { // use this to allow for serial to time out
    if(millis() - t > timeOut){
      usingSerial = false;
      Serial.end();
      break;
    }
  }
  delay(1000);
  setupBLE(); // establish nane and start services
  startAdv(); // start advertising
  for(int i=0; i<3; i++){
    pinMode(ledPin[i],OUTPUT);
  }
  ledToFade = blue; // initialize this as you like
  // fadeDelay = FADE_DELAY_FAST;
  fadeDelay = FADE_DELAY_SLOW;
  lastShowTime = millis();
}



void loop() {

  if(!printedHelp){ printHelp(); }
  if(!printedBLEhelp){ printBLEhelp(); }
  
  if(ledToFade > 0){ showRGB_LED(millis()); }

  if(usingSerial){ serialEvent(); }
  if(bleConnected){ BLEevent(); }

  if (Serial1.available()) {
    while (Serial1.available()) {
      char c = Serial1.read();
      Serial.print("i got "); Serial.println(c);
      if ((c != '\r') && (c != '\n')) {
        Serial1.print("nRF52840 Serial1 Received: ");
        Serial1.println(c);
        if ((c == 'h') || (c == '?')) {
          Serial.println("sending help");
          // printHelp(&Serial1);
          printTympanHelp();
        } else if (c == 'a') {
          LEDsOff();
          Serial1.println("LEDs off");
          ledToFade = -1;
          break;
        } else if (c == 'r') {
          LEDsOff();
          ledToFade = red;
          fadeValue = FADE_MIN;
          Serial1.println("fade red");
        } else if (c == 'b') {
          LEDsOff();
          ledToFade = blue;
          fadeValue = FADE_MIN;
          Serial1.println("fade blue");
        } else if (c == 'g') {
          LEDsOff();
          ledToFade = green;
          fadeValue = FADE_MIN;
          Serial1.println("fade green");
        } else if (c == 't') {
          Serial.println("send buffer");
          for(int i=0; i<uartOutBufferCounter; i++){
            Serial1.write(uartOutBuffer[i]); 
          }
          Serial1.println();
          Serial1.print("nRF52840 uartOutBufferCounter: "); Serial1.println(uartOutBufferCounter);
        }
      }
    } 
  }
}


void showRGB_LED(unsigned long m){
  if(ledToFade < 0){ return; }
  if(m > lastShowTime + fadeDelay){
    lastShowTime = m; // keep time
    fadeValue += fadeRate;  // adjust brightness
    if(fadeValue >= FADE_MIN){ // LEDs are Common Anode
      fadeValue = FADE_MIN; // keep in bounds
      fadeRate = 0 - FADE_RATE; // change direction
    } else if(fadeValue <= FADE_MAX){ // LEDs are Common Anode
      fadeValue = FADE_MAX; // keep in bounds
      fadeRate = 0 + FADE_RATE; // change direction
    }
  analogWrite(ledToFade,fadeValue);
  }
}

void LEDsOff(){
  for(int i=0; i<3; i++){
    analogWrite(ledPin[i],FADE_MIN);
  }
}