
// //////////////////////////////////////////////////////////////////////////////////////////////////
//
// This code will run on the **TYMPAN** to handle all user communications once grabbed from USB or from BLE
//
// //////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef TympanSerialMananger_h
#define TympanSerialMananger_h

#include "TympanState.h"

extern BLE_nRF52 tympanBle;
extern TympanState tympanState;
extern BLE_nRF52 tympanBle;
extern void updateLEDs(void);
extern void printBleName(void);
extern void setBleName(const String &s);

// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// functions that would normally run on the Tympan to respond to commands coming from the phone (via BLE through this SerialManager)
void printBleName(void) {
  String name;
  int ret_val = tympanBle.getBleName(name);
  Serial.println("(tympan main *.ino) printBleName: ret_val = " + String(ret_val) + ", name = " + name);
}

void setBleName(const String &name) {
  int ret_val = tympanBle.setBleName(name);
  Serial.println("(tympan main *.ino) setBleName: ret_val = " + String(ret_val) + " for name = " + name);
}

void getBleFirmwareVersion(void) {
  bool printResponse = true;
  int ret_val = tympanBle.version(printResponse);
}

//for this demo, have the system activate the LEDs that the user wants
void updateLEDs(void) {
  digitalWrite(tympanState.LED1_pin, (tympanState.is_active_LED1) ? HIGH : LOW);
  digitalWrite(tympanState.LED2_pin, (tympanState.is_active_LED2) ? HIGH : LOW);
}


class TympanSerialManager {
  public:
    TympanSerialManager(void) {};

    void printHelp(void);
    void respondToByte(char c);
    void printTympanRemoteLayout(void);

    void setButtonState(String btnId, bool newState);
    void setButtonText(String btnId, String text);

    void updateLEDbuttons(void);
  
};

void TympanSerialManager::printHelp(void) {
  Serial.println(versionString);
  Serial.print("    : BLE Address: "); Serial.println(deviceName);
  Serial.println("TympanSerialManager Help: Available Commands:");
  Serial.println("   h:   Print this help");
  Serial.println("   1:   Turn on LED 1");
  Serial.println("   2:   Turn on LED 2");
  Serial.println("   0:   Turn of all LEDs");
  Serial.println("   g:   Get the BLE name");
  Serial.println("   n:   Set the BLE name to Tympan-NEW");
  Serial.println("   v:   Get the BLE unit's firmware version");
  Serial.println("   j:   Send JOSN string for TympanRemote GUI");
  Serial.println();
}

void TympanSerialManager::respondToByte(char c) {
  switch (c) {
    case 'h': case '?':
      printHelp();
      break;
    case '0':
      Serial.println("TympanSerialManager: turning off LEDs...");
      tympanState.is_active_LED1 = false;
      tympanState.is_active_LED2 = false;
      updateLEDs(); updateLEDbuttons();
      break;
    case '1':
      Serial.println("TympanSerialManager: activate LED1...");
      tympanState.is_active_LED1 = true;
      updateLEDs(); updateLEDbuttons();
      break;
    case '2':
      Serial.println("TympanSerialManager: activate LED2...");
      tympanState.is_active_LED2 = true;
      updateLEDs(); updateLEDbuttons();
      break;
    case 'g':
      Serial.println("TympanSerialManager: getting nRF's BLE name...");
      printBleName();
      break;
    case 'n':
      Serial.println("TympanSerialManager: setting nRF's BLE name to Tympan-NEW...");
      setBleName("Tympan-NEW");
      Serial.println("TympanSerialManager: getting nRF's new BLE name...");
      printBleName();
      break;
    case 'v':
      Serial.println("TympanSerialManager: getting nRF's firmware version...");
      getBleFirmwareVersion();
      break;
    case 'j': case 'J':
      Serial.println("TympanSerialManager: sending JSON string for TympanApp GUI...");
      printTympanRemoteLayout();
      break;
    default:
      if ( (c != '\n') && (c != '\r') ) {
        Serial.print("TympanSerialManager: respondToByte: did not understand character ");
        Serial.print(c); Serial.print(", which is 0x"); Serial.print((byte)c,HEX); Serial.print(")");
        Serial.println();
      }
      break;
  }
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// Please don't put commas or colons in your ID strings!
void TympanSerialManager::printTympanRemoteLayout(void) {
  char jsonConfig[] = "JSON="
      "{'pages':"
        "["
          "{'title':'BLE Demo (nRF)','cards':"
            "["
              "{'name':'Active LED','buttons':"
                "["
                  "{'label':'None','id':'butNoLed','cmd':'0','width':'4'},"
                  "{'label':'LED1','id':'butLed1','cmd':'1','width':'4'},"
                  "{'label':'LED2','id':'butLed2','cmd':'2','width':'4'}"
                "]"
              "}"
            "]"
          "}"
        "]"                
      "}";

  Serial.println(jsonConfig);
  tympanBle.sendMessage(jsonConfig);
  updateLEDbuttons();
}

void TympanSerialManager::updateLEDbuttons(void) {
  setButtonState("butNoLed",false);
  setButtonState("butLed1",tympanState.is_active_LED1 ? HIGH : LOW);
  setButtonState("butLed2",tympanState.is_active_LED2 ? HIGH : LOW);
}

//Send button state to Tympan Remote App
void TympanSerialManager::setButtonState(String btnId, bool newState) {
  String msg;
  if (newState) {
    msg = "STATE=BTN:" + btnId + ":1";
  } else {
    msg = "STATE=BTN:" + btnId + ":0";
  }
  Serial.println("TympanSerialManager: sending: " + msg);
  tympanBle.sendMessage(msg);
}

//Send new button text to Tympan Remote App
void TympanSerialManager::setButtonText(String btnId, String text) {
  String msg = "TEXT=BTN:" + btnId + ":" + text;
  Serial.println("TympanSerialManager: sending: " + msg);
  tympanBle.sendMessage(msg);
}


#endif