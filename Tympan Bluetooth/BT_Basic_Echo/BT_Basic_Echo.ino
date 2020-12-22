/*
    BT_Basic_Echo

    Created: Chip Audette, OpenAudio, Dec 2020
    Purpose: Interact with the BC127 Unit via Serial terminal

    MIT License.  use at your own risk.
*/

#include <Tympan_Library.h>

//Choose your hardware's configuration
#if 0
  Tympan     myTympan(TympanRev::D); //set to D or E
  #define IS_BC127_V5 (true)  //usually RevD is V5 (unless someone changed it)
#else
  Tympan     myTympan(TympanRev::E); //set to D or E
  #define IS_BC127_V5 (false)  //usually RevE is V7 (unless someone changed it)
#endif

//Change the line endings to make it more pleasant to read in Arduino SerialMonitor?
#if IS_BC127_V5
  #define REPLACE_CR_WITH_NEWLINE (false)  //no manipulation is needed for V5?
#else
  #define REPLACE_CR_WITH_NEWLINE (true)   //if you really want a pure "echo", flip this to false
#endif

// ///////////////////////////////////////////////////////////////////////////////////////

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;

void echoIncomingUSBSerial(void) {
  while (USB_Serial->available()) {
    BT_Serial->write(USB_Serial->read()); //echo messages from USB Serial over to BT Serial
  }
}

void echoIncomingBTSerial(void) {
  while (BT_Serial->available()) {
    char c = BT_Serial->read();
    #if REPLACE_CR_WITH_NEWLINE
      if (c == '\r') c = '\n';  //convert CR to newline characters for better display in Arduino SerialMonitor
    #endif
    USB_Serial->write(c);
  }
}


// ///////////////////////////////////////////////////////////////////////////////////////

// Arduino setup() function, which is run once when the device starts
void setup() {
  // Start the serial ports on the Tympan
  myTympan.beginBothSerial(); delay(2000);
  USB_Serial = myTympan.getUSBSerial();
  BT_Serial = myTympan.getBTSerial();
  USB_Serial->println("*** BT_Basic_Echo.  Starting...");

  //set all the control pins to below (inactive)
  TympanPins pins(myTympan.getTympanRev());
  if (pins.BT_PIO0 != NOT_A_FEATURE) { pinMode(pins.BT_PIO0,OUTPUT); pinMode(pins.BT_PIO0,LOW); }  //go low (off?)
  if (pins.BT_PIO4 != NOT_A_FEATURE) { pinMode(pins.BT_PIO4,OUTPUT); pinMode(pins.BT_PIO4,LOW); } //go high-impedance
  if (pins.BT_PIO5 != NOT_A_FEATURE) { pinMode(pins.BT_PIO5,OUTPUT); pinMode(pins.BT_PIO5,LOW); } //go high-impedance

  //get into command mode, if needed
  if (IS_BC127_V5 == true) {
    USB_Serial->println("*** Switching Tympan BT module to command mode...");
    myTympan.forceBTtoDataMode(false);   delay(400); 
    BT_Serial->print("$$$");delay(200);echoIncomingBTSerial();  //should respond "CMD"
    USB_Serial->println("*** If successful, should see 'CMD'.");
  }
    
  //transition to Serial echo mode to allow playing around, if desired
  USB_Serial->println();
  USB_Serial->println("*** Now entering BT <-> USB Echo mode...feel free to send messages back and forth.");
  USB_Serial->println();
  if ((myTympan.getTympanRev() == TympanRev::D) || (myTympan.getTympanRev() == TympanRev::E)) {
    USB_Serial->println("BC127: To see BLE devices that are active, try typing: HELP");
    USB_Serial->println("BC127: To see BLE connection status, try typing : STATUS");
    USB_Serial->println("BC127: To see BLE devices that are active, try typing : SCAN 5"); //the "5" is for 5 seconds of scanning
    USB_Serial->println("BC127: To see SPP devices that are active, try typing : INQUIRY");
    USB_Serial->println();
  }
  USB_Serial->println("Remember that the BC127 only wants carriage returns, so switch your end-of-line to CR");
}

// Arduino loop() function, which is run over and over for the life of the device
void loop() {
  echoIncomingUSBSerial();     //echo messages from USB Serial over to BT Serial
  echoIncomingBTSerial();  //echo messages from BT serial over to USB Serial
} //end loop();
