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
 */

#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "TympanBLE.h"

BLEDfu  bledfu;  // OTA DFU service
BLEDis  bledis;  // device information
BLEUart bleuart; // uart over ble

#define MESSAGE_LENGTH 256
uint8_t outBuffer[MESSAGE_LENGTH];
int outBufferCounter = 0;
size_t outBytesSent;

void setup() {

  Serial.begin(115200);   //USB Serial to the PC

  Serial1.setPins(0,1);   //our nRF wiring uses Pin0 for RX and Pin1 for TX
  Serial1.begin(115200/2);  //Hardware UART serial to the Tympan

  outBufferCounter = 0;
  for (uint8_t b=48; b<=122; b++) {
    outBuffer[outBufferCounter] = b; // prepare the dummy array
    outBufferCounter++;
  }
  outBuffer[outBufferCounter] = 0x00; // terminate the dummy with null

  unsigned long t = millis();
  int timeOut = 1000; // 1 second time out before we bail on a serial connection
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
  if(usingSerial){
    Serial.println("advertising as "); Serial.println(versionString);
    Serial.println("connect and send '?' for options");
  }
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
      if ((c != '\r') && (c != '\n')) {
        Serial1.print("nRF52840 Serial1: Received from Typman: ");
        Serial1.println(c);
        if ((c == 'h') || (c == '?')) {
          printHelp(&Serial1);
        } else if (c == 'a') {
          LEDsOff();
          ledToFade = -1;
          break;
        } else if (c == 'r') {
          strcpy(outString,"Fade Red"); // BLEwrite();
          LEDsOff();
          ledToFade = red;
          fadeValue = FADE_MIN;
        } else if (c == 'b') {
          strcpy(outString,"Fade Blue"); // BLEwrite();
          LEDsOff();
          ledToFade = blue;
          fadeValue = FADE_MIN;
        } else if (c == 'g') {
          strcpy(outString,"Fade Green"); // BLEwrite();
          LEDsOff();
          ledToFade = green;
          fadeValue = FADE_MIN;
        } else if (c == 't') {
          for(int i=0; i<outBufferCounter; i++){
            Serial1.write(outBuffer[i]); 
          }
          Serial1.println();
          Serial1.print("buffer counter: "); Serial.println(outBufferCounter);
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