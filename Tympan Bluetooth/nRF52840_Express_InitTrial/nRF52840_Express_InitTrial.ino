/*

      nRF52840_TympanRemoteApp
      
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


// Include the files needed by nRF52 firmware
#include <Arduino.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include "nRF52_BLEuart_Tympan.h"
#include "nRF52_BLE_Stuff.h"
#include "nRF52_AT_API.h"


// Include the files needed for this simulation, which pretends there's also a Tympan attached
#include "Serial_Link_Simulator.h"  //only needed because we're faking that there is a Tympan
#include "Tympan_BLE_nRF52.h"              //only needed because we're faking that there is a Tympan
#include "TympanState.h"            //only needed because we're faking that there is a Tympan
#include "TympanSerialManager.h"    //only needed because we're faking that there is a Tympan

//#define SERIAL_TO_TYMPAN Serial1                 //use this when physically wired to a Tympan. Assumes that the nRF is connected via Serial1 pins
//#define SERIAL_FROM_TYMPAN Serial1               //use this when physically wired to a Tympan. Assumes that the nRF is connected via Serial1 pins
#define SERIAL_TO_TYMPAN (serial_BLE_to_Tympan)    //Use when also simulating the connection to the Tympan.
#define SERIAL_FROM_TYMPAN (serial_Tympan_to_BLE)  //Use when also simulating the connection to the Tympan.

// /////////////////////// This brick of code is just for when we want to pretend that this firmware is also a Tympan
//Simulate the Serial link that should be between the Tympan and the BLE module
Serial_Simulator serial_Tympan_to_BLE;  //have the simulated serial link talk to/from the BLE UART object
Serial_Simulator serial_BLE_to_Tympan;  //have the simulated serial link talk to/from the BLE UART object

//Create BLE class that'll format messages for the Tympan Remote App
BLE_nRF52           tympanBle(&SERIAL_FROM_TYMPAN, &SERIAL_TO_TYMPAN);

//Instantiate the classes that would be running on the Tympan itself
TympanSerialManager tympanSerialManager;
TympanState         tympanState;

// /////////////////////////  Last piece of firmware nRF firmware
//Here is a firmware class that we need, but had to wait to instantiate until SERIAL_TO_TYMPAN was defined...normally, this would just be Serial1, which means it could be earlier
nRF52_AT_API    AT_interpreter(&bleService_tympanUART, &SERIAL_TO_TYMPAN);  //interpreter for the AT command set that we're inventing


// /////////////////////////////////////////////////// Main Arduino Setup() Functions

void setup_nRF52(void) {

  //start the nRF's UART serial port that is physically connected to a Tympan or other microcrontroller (if used)
  Serial1.begin(115200);  
  delay(500);

  //setup BLE and begin
  setupBLE();  
  startAdv();  // start advertising

}

void setup_for_Demo_and_Simulated_Tympan(void) {

  // initialize the LEDs for usein this demonstration
  pinMode(tympanState.LED1_pin,OUTPUT);
  pinMode(tympanState.LED2_pin,OUTPUT);
  updateLEDs();    //light the LEDs to the default state of the system
  tympanSerialManager.updateLEDbuttons(); 

  // send some info to the user on the USB Serial Port
  Serial.println(); Serial.println(versionString);
  Serial.print("Please use Bluetooth to connect to ");
  Serial.println(deviceName);

  // print the Tympan's help menu
  tympanSerialManager.printHelp();
}

void setup() {

  //start the nRF's USB serial port (it's probably already active) for debugging messages.  Not needed in production firmware.
  Serial.begin(115200);
  unsigned long t = millis();
  int timeOut = 5000; // 5 second time out before we bail on a serial connection
  while (!Serial) { // use this to allow for serial to time out
    if(millis() - t > timeOut)  break;  //break out of waiting
  }
  while (Serial.available()) Serial.read();  //clear input buffer

  // setup() items for the nRF firmware
  setup_nRF52();

  // setup() items for this demonstration, including the simulated Tympan
  setup_for_Demo_and_Simulated_Tympan();


}

// /////////////////////////////////////////////////// Main Arduino Loop() Functions

void bleUnitServiceSerial(void) {
  serialEvent(&SERIAL_FROM_TYMPAN);  //for the nRF firmware, service any messages coming in the serial port from the Tympan
}

bool loop_nRF52(void) {
  // Here are the loop items for the nR52 firmware
  //This assumes that the real USB Serial is going to the simulated Tympan instead of being serviced here by the nRF52
  bool did_we_do_anything = false;

  //Respond to incoming serial messages
  //serialEvent(&SERIAL_FROM_TYMPAN);  //for the nRF firmware, service any messages coming in the serial port from the Tympan
  bleUnitServiceSerial();
  if (bleConnected) { 
    //for the nRF firmware, service any messages coming in from BLE wireless link
    //BLEevent(&bleService_adafruitUART, &SERIAL_TO_TYMPAN);
    BLEevent(&bleService_tympanUART, &SERIAL_TO_TYMPAN);  
    did_we_do_anything = true;
  }

  return did_we_do_anything;
}
      
bool loop_for_Demo_and_Simulated_Tympan(void) {
  //Here are the loop items associated with the Tympan that we are simulating here in this test firmware
  //This assumes that the real USB Serial is going to the simulated Tympan
  bool did_we_do_anything = false;  

  if (Serial.available()) {
    tympanSerialManager.respondToByte((char)Serial.read());  //for the Tympan simulation, service any messages PC via USB Serial
    did_we_do_anything = true;
  }
  if (tympanBle.available()) {
    tympanSerialManager.respondToByte((char)tympanBle.read()); //for the Tympan simulation, service any messages received form the BLE module
    did_we_do_anything = true;
  }

  ///service the BLE advertising state ... this is a hold-over from the RevD / RevE BLE with the BC127.  Do we need for nRF?
  // tympanBle.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

  return did_we_do_anything;
}

void loop() {
  bool did_we_do_anything = false;

  // execute the loop() functions for the nRF52 firwmare functions
  did_we_do_anything |= loop_nRF52();    // if loop_nRF52() does anything useful, "did_we_do_anything" becomes true

  // execute the loop() functions needed just for this demo and for the simulated Tympan
  did_we_do_anything |= loop_for_Demo_and_Simulated_Tympan(); // if loop_for_Demo_and_Simulated_Tympan() does anything useful, "did_we_do_anything" becomes true

  // be nice
  if (did_we_do_anything == false) delay(1);

}

// ///////////////////////////////////// Functions to reposnd to Serial Commands


