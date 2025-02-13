
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//classes from the main sketch that might be used here
extern Tympan myTympan;                  //created in the main *.ino file
extern AudioSettings_F32 audio_settings; //created in the main *.ino file  
extern BLE_nRF52 &ble_nRF52;


//functions in the main sketch that I want to call from here
extern void changeGain(float);
extern void printGainLevels(void);

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble)  {};
      
    void printHelp(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" h: Print this help");
  Serial.println(" v:   BLE: Get 'Version' of firmware from module");
  Serial.println(" n:   BLE: Get 'Name' from module");
  Serial.println(" g/G: BLE: Get 'Connected' status via software (g) or GPIO (G)");
  Serial.println();
}


//switch yard to determine the desired action...gets called by SerialManagerBase::respondToCharacter
bool SerialManager::processCharacter(char c) {  //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true;
  int val = 0;
  int err_code = 0;

  switch (c) {
    case 'h':
      printHelp(); 
      break;
    case 'n':
      {
        String name = String("");
        int err_code = ble_nRF52.getBleName(&name);
        Serial.println("serialManager: BLE: Name from module = " + name);
        if (err_code != 0) Serial.println("serialManager:  ble_nRF52.getBleName returned error code " + String(err_code));
        setButtonText("bleName",name);
      }
      break;
    case 'g':
      val = ble_nRF52.isConnected(BLE_nRF52::GET_VIA_SOFTWARE);
      if (val < 0) {
        Serial.println("SerialManager: ble_nRF52.isConnected(Software) returned err_code = " + String(val));
      } else {
        Serial.println("serialManager: BLE: Connected (via software) = " + String(val));
        setButtonText("isConn_s", String(val ? "YES" : "NO"));
        setButtonText("isConn_g", String(""));
      }
      break;
    case 'G':
      val = ble_nRF52.isConnected(BLE_nRF52::GET_VIA_GPIO);
      if (val < 0) {
        Serial.println("SerialManager: ble_nRF52.isConnected(GPIO) returned err_code = " + String(val));
      } else {
        Serial.println("serialManager: BLE: Connected (via GPIO) = " + String(val));
        setButtonText("isConn_g", String(val ? "YES" : "NO"));
        setButtonText("isConn_s", String(""));
      }
      break;
    case 'v':
      {
        String version;
        err_code = ble_nRF52.version(&version);
        if (err_code < 0) {
          Serial.println("serialManager: BLE: version returned err_code " + String(err_code));
        } else {
          Serial.println("serialManager: BLE module firmware: " + version);
          setButtonText("version",version);
        }
      }
      break; 
    default:
      ret_val = false;
  }
  return ret_val;
}

#endif

