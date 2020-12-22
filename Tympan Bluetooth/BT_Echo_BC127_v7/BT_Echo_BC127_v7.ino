/*
    BT_ECHO_BC127_V7_DataViaCommandMode

    Created: Chip Audette, OpenAudio, Nov-Dec 2020
    Purpose: Send data back and forth with Tympan RevD/RevE even though the Tympan
      is in Command_Mode rather than data mode.  For BC127 with firmware at 7.2
    MIT License.  use at your own risk.
*/

#include <Tympan_Library.h>

//Use this if you have a Tympan RevE
//Tympan     myTympan(TympanRev::D); //set to D or E
Tympan     myTympan(TympanRev::E); //set to D or E

//This primary issue is that the BC127 V7 really wants SPP to be via
//Command_Mode rather than through Data_Mode.  So, we need to wrap our
//data messages.  Set MEDIATED_COMMS to 1 to enable this mediation
#define MEDIATED_COMMS 1 

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;

void echoIncomingUSBSerial(void) {
#if MEDIATED_COMMS 
  while (myTympan.USB_serial_available()) {
    myTympan.BT_serial_write(myTympan.USB_serial_read());
  }
#else
  while (USB_Serial->available()) {
    BT_Serial->write(USB_Serial->read()); //echo messages from USB Serial over to BT Serial
  }
#endif
}

void echoIncomingBTSerial(void) {
#if MEDIATED_COMMS
  while (myTympan.BT_serial_available()) {
    myTympan.USB_serial_write(myTympan.BT_serial_read());//echo messages from BT serial over to USB Serial
  }
#else
  while (BT_Serial->available()) {
    char c = BT_Serial->read();
    //if (c == '\r') c = '\n';  //replace carriage return with newline...better for Arduino SerialMonitor
    USB_Serial->write(BT_Serial->read());
  }
#endif
}


// Arduino setup() function, which is run once when the device starts
void setup() {
  // Start the serial ports on the Tympan
  myTympan.beginBothSerial(); delay(2000);
  USB_Serial = myTympan.getUSBSerial();
  BT_Serial = myTympan.getBTSerial();
  USB_Serial->println("*** BT_ECHO_BC127_V7_DataViaCommandMode.  Starting...");

  //set all the control pins to below (inactive)
  TympanPins pins(myTympan.getTympanRev());
  if (pins.BT_PIO0 != NOT_A_FEATURE) { pinMode(pins.BT_PIO0,OUTPUT); pinMode(pins.BT_PIO0,LOW); }  //go low (off?)
  if (pins.BT_PIO4 != NOT_A_FEATURE) { pinMode(pins.BT_PIO4,OUTPUT); pinMode(pins.BT_PIO4,LOW); } //go high-impedance
  if (pins.BT_PIO5 != NOT_A_FEATURE) { pinMode(pins.BT_PIO5,OUTPUT); pinMode(pins.BT_PIO5,LOW); } //go high-impedance

  //transition to Serial echo mode to allow playing around, if desired
  delay(1000);
  USB_Serial->println("Making connectable and discoverable");
  BT_Serial->print("SET BT_STATE_CONFIG=1 1");BT_Serial->print('\r');
  BT_Serial->print("ADVERTISING ON");BT_Serial->print('\r');
  BT_Serial->print("SET AUTOCONN=1");BT_Serial->print('\r');
  BT_Serial->print("WRITE");BT_Serial->print('\r');
  BT_Serial->print("RESET");BT_Serial->print('\r');
    
  USB_Serial->println("Now entering BT <-> USB Echo mode.  Connect to phone and send characters back and forth...");
  
}

// Arduino loop() function, which is run over and over for the life of the device
void loop() {

  echoIncomingUSBSerial();     //echo messages from USB Serial over to BT Serial
  echoIncomingBTSerial();  //echo messages from BT serial over to USB Serial

  delay(2);

} //end loop();
