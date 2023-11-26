/*********************************************************************
 This is an example for our nRF51822 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

#include <Arduino.h>
#include <SPI.h>
#include "Adafruit_BLE.h"
#include "Adafruit_BluefruitLE_SPI.h"
#include "Adafruit_BluefruitLE_UART.h"

#include "BluefruitConfig.h"
#include "BLE_local.h"

#if SOFTWARE_SERIAL_AVAILABLE
  #include <SoftwareSerial.h>
#endif

/* This example demonstrates how to use Bluefruit callback API :
 * - setConnectCallback(), setDisconnectCallback(), setBleUartRxCallback(),
 * setBleGattRxCallback() are used to install callback function for specific
 * event. 
 * - Furthermore, update() must be called inside loop() for callback to
 * be executed.
 * 
 * The sketch will add an custom service with 2 writable characteristics,
 * and install callback to execute when there is an update from central device
 * - one hold string
 * - one hold a 4-byte integer
  */

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE     Perform a factory reset when running this sketch
   
                            Enabling this will put your Bluefruit LE module
                            in a 'known good' state and clear any config
                            data set in previous sketches or projects, so
                            running this at least once is a good idea.
   
                            When deploying your project, however, you will
                            want to disable factory reset by setting this
                            value to 0.  If you are making changes to your
                            Bluefruit LE device via AT commands, and those
                            changes aren't persisting across resets, this
                            is the reason why.  Factory reset will erase
                            the non-volatile memory where config data is
                            stored, setting it back to factory default
                            values.
       
                            Some sketches that require you to bond to a
                            central device (HID mouse, keyboard, etc.)
                            won't work at all with this feature enabled
                            since the factory reset will clear all of the
                            bonding data stored on the chip, meaning the
                            central device won't be able to reconnect.
                            
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features    
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE        1
/*=========================================================================*/



// Create the bluefruit object, either software serial...uncomment these lines

/* ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST */
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

/* ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST */
//Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
//                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
//                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

int32_t charid_string;

BLE_local ble_local;

// A small helper
void error(const __FlashStringHelper*err) {
  Serial.println(err);
  while (1);
}

String serviceUUID = String("BC-2F-4C-C6-AA-EF-43-51-90-34-D6-62-68-E3-28-F0");
String characteristicUUID = String("06-D1-E5-E7-79-AD-4A-71-8F-AA-37-37-89-F7-D9-3C");

void startAdvertising(void) {
  Serial.println("startAdvertising: starting advertising");
  ble.sendCommandCheckOK("AT+GAPSTARTADV");
}

void addServiceUUIDtoAdvertising(void) {
  //https://learn.adafruit.com/adafruit-feather-m0-bluefruit-le/ble-gap
  String n_bytes_str = "11";           //0x11 => 17 => 1 for the Data Type Value and 16 for the Service UUID
  String Data_Type_Value_str = "06";  //0x06 for Incomplete list of 128-bit UUIDs
  String foo_str;
  foo_str = String("AT+GAPSETADVDATA=");
  foo_str = foo_str + n_bytes_str;
  foo_str = foo_str + String("-") + Data_Type_Value_str;
  for (int i=0; i<16; i++) {
    int ind = serviceUUID.length() - (3*i) - 2;
    foo_str = foo_str + String("-") + String(serviceUUID[ind]) + String(serviceUUID[ind+1]);
  }
  Serial.println("TympanRemote needs to see service UUID in the advertising payload: ");
  Serial.println("  : " + foo_str);
  ble.sendCommandCheckOK(foo_str.c_str());
} 

void connected(void) { 
  Serial.println( F("connected: Connected!") );
}

void disconnected(void) { 
  Serial.println("disconnected: Disconnected!");
  startAdvertising();
}

void sendTympanRemoteJSON(void) {
  String tympanResponse = String("JSON={'icon':'tympan.png','pages':[{'title':'Treble Boost Demo','cards':[{'name':'Highpass Gain (dB)','buttons':[{'label':'-','cmd':'K','width':'4'},{'id':'gainIndicator','width':'4'},{'label':'+','cmd':'k','width':'4'}]},{'name':'Highpass Cutoff (Hz)','buttons':[{'label':'-','cmd':'T','width':'4'},{'id':'cutoffHz','width':'4'},{'label':'+','cmd':'t','width':'4'}]}]},{'title':'Globals','cards':[{'name':'Analog Input Gain (dB)','buttons':[{'id':'inpGain','width':'12'}]},{'name':'CPU Usage (%)','buttons':[{'label':'Start','cmd':'c','id':'cpuStart','width':'4'},{'id':'cpuValue','width':'4'},{'label':'Stop','cmd':'C','width':'4'}]}]}],'prescription':{'type':'BoysTown','pages':['serialMonitor']}}");
  ble_local.sendMessage(tympanResponse);
}

void BleGattRX(int32_t chars_id, uint8_t data[], uint16_t len)
{
  //print the received info 
  Serial.print( F("[BLE GATT RX] (" ) );
  Serial.print(chars_id);
  Serial.print(") ");

  //does it match our expected characteristic id?
  if (chars_id == charid_string) {  
    Serial.write(data, len);
    Serial.println();

    //look to see if it matches one of our target characters for responding!
    for (int i=0; i<len; i++) { //loop over all characters in the payload
      if ( ((char)data[i]) == 'J' ) {
        Serial.println("BleGattRX: Detected 'J'.  Sending TympanRemote layout");
        sendTympanRemoteJSON();
      }
    }
  }
}

/**************************************************************************/
/*!
    @brief  Sets up the HW an the BLE module (this function is called
            automatically on startup)
*/
/**************************************************************************/
void setup(void)
{
  while (!Serial);  // required for Flora & Micro
  delay(500);

  Serial.begin(115200);
  Serial.println(F("Adafruit Bluefruit Callbacks Example"));
  Serial.println(F("-------------------------------------"));

  /* Initialise the module */
  Serial.print(F("Initialising the Bluefruit LE module: "));

  if ( !ble.begin(VERBOSE_MODE) )  {
    error(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring?"));
  }

  /* Perform a factory reset to make sure everything is in a known state */
  Serial.println(F("Performing a factory reset: "));
  if (!ble.factoryReset())  error(F("Couldn't factory reset"));
 
  Serial.println("Adding GATT Service and String N-W Characteristic");
  String foo_str = String("AT+GATTADDSERVICE=UUID128=") + serviceUUID;
  ble.sendCommandCheckOK(foo_str.c_str());
  foo_str = String("AT+GATTADDCHAR=UUID128=") + characteristicUUID + String(",PROPERTIES=0x18,MIN_LEN=1,MAX_LEN=20,DATATYPE=string,DESCRIPTION=string,VALUE=init");
  ble.sendCommandWithIntReply(foo_str.c_str(), &charid_string);

  //attach the BLE info to our local ble message formatter
  ble_local.BLE_TX_ptr = &ble;
  ble_local.ble_char_id = charid_string;

  Serial.println("Adding Service UUID to Advertising (for TympanRemote App)");
  addServiceUUIDtoAdvertising();

  String name = String("TympB");
  Serial.println("Set advertising name: " + name);
  foo_str = String("AT+GAPDEVNAME=" + name);
  ble.sendCommandCheckOK(foo_str.c_str());

  // Adafruit says that slowing down the transmission is a good idea for the super long TX lines!
  // ble.setInterCharWriteDelay(5); // 5 ms

  ble.reset();

  /* Disable command echo from Bluefruit */
  ble.echo(false);

  Serial.println("Requesting Bluefruit info:");
  ble.info();
  
  /* Set callbacks */
  ble.setConnectCallback(connected);
  ble.setDisconnectCallback(disconnected);
  
  /* Only one BLE GATT function should be set, it is possible to set it 
  multiple times for multiple Chars ID  */
  ble.setBleGattRxCallback(charid_string, BleGattRX);

  startAdvertising();
}



/**************************************************************************/
/*!
    @brief  Constantly poll for new command or response data
*/
/**************************************************************************/
void loop(void)
{  
  ble.update(200);
}
