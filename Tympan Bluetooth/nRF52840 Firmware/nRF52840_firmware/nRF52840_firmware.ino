/*

      nRF52840_Firmware
      
      This code base for Adafruit "nRF52840 Express" (and similar boards) to connect
      To the Tympan Remote app.

      This code includes:
      * Bluetooth connection handlers
      * Generates unique advertising name specific to the module
      * Includes OTA DFU Service 
        >>  ALL TYMPAN nRF52  CODE MUST INCLUDE OTA DFU SERVICE  <<
      * Basic comms over BLE to blink LEDs for testing coms pipeline
      
 
    Original BLE servicing code by Joel Murphy for Flywheel Lab, February 2024
    Extended for Tympan Remote App by Chip Audette, Benchtop Engineering, for Creare LLC, February 2024
 */

#define DEBUG_VIA_USB true

#define SERIAL_TO_TYMPAN Serial1                 //use this when physically wired to a Tympan. Assumes that the nRF is connected via Serial1 pins
#define SERIAL_FROM_TYMPAN Serial1               //use this when physically wired to a Tympan. Assumes that the nRF is connected via Serial1 pins

// Include the files needed by nRF52 firmware
#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "nRF52_BLEuart_Tympan.h"
#include "nRF52_BLE_Stuff.h"
#include "nRF52_AT_API.h"


void printHelpToUSB(void) {
  Serial.println("nRF52840 Firmware: Help:");
  Serial.print("   : Version string: "); Serial.println(versionString);
  Serial.print("   : bleConnected: "); Serial.println(bleConnected);
  Serial.println("   : Send 'h' via USB to get this help");
  Serial.println("   : Send 'J' via USB to send 'J' to the Tympan");
}

void setup(void) {

  if (DEBUG_VIA_USB) {
    //Start up USB serial for debugging
    Serial.begin(115200);
    unsigned long t = millis();
    int timeOut = 5000; // 5 second time out before we bail on a serial connection
    while (!Serial) { // use this to allow for serial to time out
      if(millis() - t > timeOut)  break;  //break out of waiting
    }
    while (Serial.available()) Serial.read();  //clear input buffer
  
    // send some info to the user on the USB Serial Port
    Serial.println("*** nRF52840 Firmware: STARTING ***");
    printHelpToUSB();
  }

  //start the nRF's UART serial port that is physically connected to a Tympan or other microcrontroller (if used)
  Serial1.begin(115200);  
  //delay(500);
  while (Serial1.available()) Serial1.read();  //clear UART buffer

  //setup BLE and begin
  setupBLE();  
  startAdv();  // start advertising

  if (DEBUG_VIA_USB) { Serial.print("nRF52840 Firmware: Bluetooth name: "); Serial.println(deviceName); };
}


void loop(void) {

  //Respond to incoming messages on the USB serial
  if (DEBUG_VIA_USB) {
    if (Serial.available()) {
      char c = Serial.read();
      switch (c) {
        case 'h': case '?':
          printHelpToUSB();
          break;
        case 'J':
          Serial.println("nRF52840 Firmware: sending J to Tympan...");
          SERIAL_TO_TYMPAN.println("J");
          break;
      }
      //while (Serial->available()) AT_interpreter.processSerialCharacter(Serial.read());  
    }
  }

  //Respond to incoming UART serial messages
  serialEvent(&SERIAL_FROM_TYMPAN);  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  
  //Respond to incoming BLE messages
  if (bleConnected) { 
    //for the nRF firmware, service any messages coming in from BLE wireless link
    //BLEevent(&bleService_adafruitUART, &SERIAL_TO_TYMPAN);
    BLEevent(&bleService_tympanUART, &SERIAL_TO_TYMPAN);  
  }
}
      
