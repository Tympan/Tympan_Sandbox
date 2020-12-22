/*
    BC127_v7_BLE_Button  (requires v7.2 firmware for the BC127!)

    Created: Chip Audette, OpenAudio, Dec 2020
    Purpose: Connect via BLE to remote control button

    Assumes use of Tunai Button: Bluetooth Button
      Bluetooth 5.0

    MIT License.  use at your own risk.
*/

#include <Tympan_Library.h>
#include <stdio.h>

//Use this if you have a Tympan RevE
Tympan     myTympan(TympanRev::E); //set to C or D

usb_serial_class *USB_Serial;
HardwareSerial *BT_Serial;

#define RAW_COMMS 1   //set to 1 to ignore the Tympan interpretations of the data stream and just pipe the raw bytes back and forth

void echoIncomingUSBSerial(void) {
#if RAW_COMMS
  while (USB_Serial->available()) {
    BT_Serial->write(USB_Serial->read()); //echo messages from USB Serial over to BT Serial
  }
#else
  while (myTympan.USB_serial_available()) {
    myTympan.BT_serial_write(myTympan.USB_serial_read());
    //myTympan.write(myTympan.USB_serial_read());
  }
#endif
}

void echoIncomingBTSerial(void) {
#if RAW_COMMS
  while (BT_Serial->available()) {
    char c = BT_Serial->read();
    if (c == '\r') c = '\n';  //convert to newline
    USB_Serial->write(c);
  }
#else
  while (myTympan.BT_serial_available()) {
    //USB_Serial->print("Serial available: "); USB_Serial->println(myTympan.BT_serial_available());
    myTympan.USB_serial_write(myTympan.BT_serial_read());//echo messages from BT serial over to USB Serial
  }
#endif
}


// Arduino setup() function, which is run once when the device starts
void setup() {
  // Start the serial ports on the Tympan
  myTympan.beginBothSerial(); delay(2000);
  USB_Serial = myTympan.getUSBSerial();
  BT_Serial = myTympan.getBTSerial();
  USB_Serial->println("*** BT_Echo_TestBC127.  Starting...");

  //set all the control pins to below (inactive)
  TympanPins pins(myTympan.getTympanRev());
  if (pins.BT_PIO0 != NOT_A_FEATURE) { 
    pinMode(pins.BT_PIO0,OUTPUT); 
    pinMode(pins.BT_PIO0,LOW);  //go low (off?)
    //IS_PIO0_INPUT = false; 
   }
  if (pins.BT_PIO4 != NOT_A_FEATURE) { pinMode(pins.BT_PIO4,OUTPUT); pinMode(pins.BT_PIO4,LOW); } //go high-impedance
  if (pins.BT_PIO5 != NOT_A_FEATURE) { pinMode(pins.BT_PIO5,OUTPUT); pinMode(pins.BT_PIO5,LOW); } //go high-impedance


  // setup BT Module
  myTympan.println("Making connectable and discoverable");
  USB_Serial->println("SET BT_STATE_CONFIG=1 1");  //should be saved at startup, if you issue WRITE later
  BT_Serial->print("SET BT_STATE_CONFIG=1 1");BT_Serial->print('\r'); delay(200);
  USB_Serial->println(String("Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'OK' 

  USB_Serial->println("BT_STATE ON ON");
  BT_Serial->print("BT_STATE ON ON");BT_Serial->print('\r'); delay(200);
  USB_Serial->println(String("Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'OK' 

  USB_Serial->println("ADVERTISING ON");
  BT_Serial->print("ADVERTISING ON");BT_Serial->print('\r'); delay(200);
  USB_Serial->println(String("Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'OK' 

  /*
  myTympan.BT_serial_write("SET AUTOCONN=1");myTympan.BT_serial_write('\r');
  myTympan.BT_serial_write("WRITE");myTympan.BT_serial_write('\r');
  myTympan.BT_serial_write("RESET");myTympan.BT_serial_write('\r');
  */

  //Tell the human user to put the button into connection/pairing mode
  USB_Serial->println("Put the remote system into pairing mode.");
  USB_Serial->println("Press any key to continue...");
  while (USB_Serial->available() == 0) delay(20); //wait for character
  while (USB_Serial->available() == 1) USB_Serial->read(); //read to empty the buffer

  //Scan for the remote device
  String targetName = "Tunai";
  USB_Serial->println(String("BLE Scanning for 5 seconds...looking for: ") + targetName);
  BT_Serial->print("SCAN 5");BT_Serial->print('\r');
  //BT_Serial->setTimeout(1000);
  bool is_done = false;
  int n_empty = 0;
  char BTaddr[40];
  String addr, device_name;
  while ((!is_done) && (n_empty < 5)) {
    String fooString = BT_Serial->readStringUntil('\r');
    USB_Serial->print("> Received: ");USB_Serial->println(fooString);
    if (fooString.length() == 0) {
      n_empty++;
    } else {
      
      if (fooString.startsWith("SCAN ")) {
        
        
        //String foo = receivedString.substring(5,5+12);
        int ind1  = fooString.indexOf(" "); //Look for end of SCAN and start of BRT address
        int ind2  = fooString.indexOf(" ", ind1+1); //look for end of BT address
        int ind3  = fooString.indexOf("<", ind2+1); //look for start of name
        int ind4  = fooString.indexOf(">", ind3+1); //look for end of name
        if (ind2 > ind1+1) {addr = fooString.substring(ind1+1, ind2); } else { addr = String(""); }
        if (ind4 > ind3+1) {device_name = fooString.substring(ind3+1, ind4); } else { device_name = String(""); }
        //USB_Serial->print("> Detected BT Device: "); USB_Serial->println(device_name + String(' ') + addr);
        if (device_name.startsWith(targetName)) {
          strcpy(BTaddr,addr.c_str());         
          //USB_Serial->print("> Keeping candidate: "); USB_Serial->println(BTaddr);
          USB_Serial->print("> Detected BT Device: "); USB_Serial->println(device_name + String(' ') + String(BTaddr));
        }
      } else if (fooString.startsWith("SCAN_OK")) {
        USB_Serial->println("> Detected SCAN_OK: finished."); 
        is_done = true;
      }
    }
  }

  //Tell the human user to put the button into connection/pairing mode
  USB_Serial->println();
  USB_Serial->println();
  USB_Serial->println("Refresh before connecting: Put the remote system into pairing mode.");
  USB_Serial->println("Press any key to continue...");
  while (USB_Serial->available() == 0) delay(20); //wait for character
  while (USB_Serial->available() == 1) USB_Serial->read(); //read to empty the buffer
  

  //Connect to remote device
  USB_Serial->println(String("> Connecting to: ") + String(BTaddr));
  USB_Serial->println(String("OPEN ") + String(BTaddr) + String(" BLE 1"));
  BT_Serial->print(String("OPEN ") + String(BTaddr) + String(" BLE 1"));BT_Serial->print('\r');
  USB_Serial->println(String("> Received: ") +  BT_Serial->readStringUntil('\r'));  //should be 'PENDING'
  while (USB_Serial->available() == 1) USB_Serial->read(); //read to empty the buffer
  String response = BT_Serial->readStringUntil('\r');
  USB_Serial->println(String("> Received: ") + response);  //should be 'OPEN_OK'
  
  if (response.startsWith("OPEN_OK")) {
    USB_Serial->println("*** Connected! ***");

    // Add security
    USB_Serial->println("BLE_SECURITY 14");
    BT_Serial->print("BLE_SECURITY 14");BT_Serial->print('\r'); delay(200);
    USB_Serial->println(String("Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'Pending'
    USB_Serial->println(String("Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'OK' 
    

    ///// Now request notifications

    #if 0
    //I don't think that this one is needed
    //sign up for notifications of button presses
    USB_Serial->println("Requesting BLE notification 0x001D...");
    USB_Serial->println("BLE_WRITE 14 001D 2");
    BT_Serial->print("BLE_WRITE 14 001D 2");BT_Serial->print('\r'); delay(200);
    USB_Serial->println(String("> Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'PENDING'
    USB_Serial->println("1 1");
    BT_Serial->write((byte)1); BT_Serial->write((byte)1); delay(200); //write two ones
     USB_Serial->println(String("> Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'OK'
    #endif
    
    //sign up for notifications of button presses
    USB_Serial->println("Requesting BLE notification 0x0021...");
    USB_Serial->println("BLE_WRITE 14 0021 2");
    BT_Serial->print("BLE_WRITE 14 0021 2");BT_Serial->print('\r'); delay(200);
    USB_Serial->println(String("> Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'PENDING'
    USB_Serial->println("1 1");
    BT_Serial->write((byte)1); BT_Serial->write((byte)1); delay(200); //write two ones
     USB_Serial->println(String("> Received: ") + BT_Serial->readStringUntil('\r'));  //should be 'OK'
 
  } else {
    BT_Serial->println("*** ERROR *** Could not connect.");
  }

  myTympan.println();
  myTympan.println("If you received 'OK', notifications should work.  Press the remote device's buttons!");
  myTympan.println();
  myTympan.println("*** Now entering BT <-> USB Echo mode...feel free to send messages back and forth.");
  
}

// Arduino loop() function, which is run over and over for the life of the device
void loop() {
  echoIncomingUSBSerial();     //echo messages from USB Serial over to BT Serial
  echoIncomingBTSerial();  //echo messages from BT serial over to USB Serial
} //end loop();
