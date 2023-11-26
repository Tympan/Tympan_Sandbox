/*
    ESP32_InitTrial
	
	Created: Chip Audette, Open Audio, Nov 2023
   	    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
        Which was ported to Arduino ESP32 by Evandro Copercini
		
	This code was explored using an AdafruitESP32-S3 Feather (8MB Flash, No PSRAM)
	
	The purpose of this code is to configure the ESP32 to send/receive bytes via BLE.
		* At first, I used the "NRF Connect" App on my Android phone
		* I used the BC127 BT module on the Tympan Rev E
		* I looked at the BLE services and characteristics and other options it provided
		* I then worked on this sketch for the ESP32 and tried to match all the entries,
		  options, and values reported for the BC127
		* Once I got the ESP32 to look as similar to the BC127 as I could, I would try
		  to get the ESP32 to interact with the Tympan Remote phone app.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "BLE_local.h" 

BLEServer *pServer = NULL;
BLECharacteristic * p_N_W_Characteristic;
BLECharacteristic * p_N_WNR_Characteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

BLE_local ble_local;

#define SERVICE_UUID  "BC2F4CC6-AAEF-4351-9034-D66268E328F0" // From the Tympan Remote App
#define CHARACTERISTIC_UUID_N_WNR "818AE306-9C5B-448D-B51A-7ADD6A5D314D"  //for Characteristic that does "Notify" and "Write with No Reply"
#define CHARACTERISTIC_UUID_N_W "06D1E5E7-79AD-4A71-8FAA-373789F7D93C"    //for Characteristic that does "Notify" and "Write"  (This is the primary on

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("MyServerCallbacks: connected.");
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("MyServerCallbacks: disconnected.");
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    public:
      MyCallbacks(String _name) : BLECharacteristicCallbacks() {
        name = String(_name);
      }
      void onWrite(BLECharacteristic *pCharacteristic) {
        String rxValue = pCharacteristic->getValue();
  
        if (rxValue.length() > 0) {
          Serial.println("*********");
          Serial.print("MyCallbacks (" + name + "): Received Value: ");
          for (int i = 0; i < rxValue.length(); i++) Serial.print(rxValue[i]);
          Serial.println();
          Serial.println("*********");
        }
      }
      String name;
};

void setup() {
  Serial.begin(115200);
  delay(500);

  // Create the BLE Device
  BLEDevice::init("TympU");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  p_N_W_Characteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_N_W,
                        BLECharacteristic::PROPERTY_NOTIFY |
                        BLECharacteristic::PROPERTY_WRITE
                      );

  BLE2902 *ble2902charA = new BLE2902();
  ble2902charA->setNotifications(false);
  ble2902charA->setIndications(false);
  p_N_W_Characteristic->setCallbacks(new MyCallbacks("N_W"));
  p_N_W_Characteristic->addDescriptor(ble2902charA);
  ble_local.BLE_TX_ptr = p_N_W_Characteristic;

  p_N_WNR_Characteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_N_WNR,
                        BLECharacteristic::PROPERTY_NOTIFY | 
                        BLECharacteristic::PROPERTY_WRITE_NR
                      );

  BLE2902 *ble2902charB = new BLE2902();
  ble2902charB->setNotifications(false);
  ble2902charB->setIndications(false);
  p_N_WNR_Characteristic->setCallbacks(new MyCallbacks("N_WNR"));
  p_N_WNR_Characteristic->addDescriptor(ble2902charB);

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

unsigned long prev_millis = 0;
char c;
uint8_t foo_uint8;
bool flag_printIsConnected = false;
String tympanResponse = String("JSON={'icon':'tympan.png','pages':[{'title':'Treble Boost Demo','cards':[{'name':'Highpass Gain (dB)','buttons':[{'label':'-','cmd':'K','width':'4'},{'id':'gainIndicator','width':'4'},{'label':'+','cmd':'k','width':'4'}]},{'name':'Highpass Cutoff (Hz)','buttons':[{'label':'-','cmd':'T','width':'4'},{'id':'cutoffHz','width':'4'},{'label':'+','cmd':'t','width':'4'}]}]},{'title':'Globals','cards':[{'name':'Analog Input Gain (dB)','buttons':[{'id':'inpGain','width':'12'}]},{'name':'CPU Usage (%)','buttons':[{'label':'Start','cmd':'c','id':'cpuStart','width':'4'},{'id':'cpuValue','width':'4'},{'label':'Stop','cmd':'C','width':'4'}]}]}],'prescription':{'type':'BoysTown','pages':['serialMonitor']}}");
void loop() {
  if (Serial.available() > 0) {
    c = Serial.read();  
    if (c == 'a') {
      Serial.println("Received 'a': starting advertising...");
      pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
      pServer->getAdvertising()->start();
    } else if (c == 't') {
      Serial.println("Received 't': sending tympan string.");
      ble_local.sendMessage(tympanResponse);
//      for (int I=0; I<tympanResponse.length(); I++) {
//        Serial.print(tympanResponse[I]);
//        foo_uint8 = (uint8_t)tympanResponse[I];
//        p_N_W_Characteristic->setValue(&foo_uint8, 1);
//        p_N_W_Characteristic->notify();
//        delay(5);
//      }
      Serial.println();
    } else {
      Serial.print("Received from PC and sending to BLE: " + String(c));
      ble_local.sendMessage(c);
      delay(10);
      while (Serial.available() > 0) {
        c = Serial.read();
        if ( (c != '\r') && (c != '\n')){
          Serial.print(c);
          ble_local.sendMessage(c);
          delay(5);
        }
      }
      Serial.println();
    }
  }

  if (deviceConnected) {
    if (flag_printIsConnected) {
      Serial.println("Connected!");
      flag_printIsConnected = false;
    }
    if (0) {
      if ((millis() - prev_millis) > 1000UL) {
        Serial.println("Sending value = " + String(txValue));
        p_N_W_Characteristic->setValue(&txValue, 1);
        p_N_W_Characteristic->notify();
       txValue++;
       prev_millis = millis();
      }
    }
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    //pServer->startAdvertising(); // restart advertising
    pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
    pServer->getAdvertising()->start();
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    Serial.println("Connecting...");
    oldDeviceConnected = deviceConnected;
    flag_printIsConnected = true;
  }
}
