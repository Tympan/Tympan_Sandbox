    
#ifndef SerialManager_h
#define SerialManager_h

#include <Tympan_Library.h>

//Extern classes and variables
extern State myState;
extern EarpieceMixer_F32_UI earpieceMixer;
extern AudioSDWriter_F32_UI audioSDWriter;
extern const int LEFT, RIGHT;

//Extern Functions (from main sketch file)
extern float incrementInputGain(float);
extern float incrementDigitalGain(float);

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {  };
    
    void printHelp(void); 
    const float increment_input_gain_dB = 3.0f;  
    const float increment_digital_gain_dB = 3.0f;  

    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void);
    
    void setFullGUIState(bool activeButtonsOnly = false);
    void setButtonState_gains(bool activeButtonsOnly = false);
   

  protected:
      TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
      virtual bool processCharacter(char c);
      bool firstTimeSendingGUI = true;
};

void SerialManager::printHelp(void) {
  Serial.println();  // extra line at beginning
  Serial.println(F("SerialManager Help: Available Commands:"));
  Serial.println(F(" General: No Prefix"));
  Serial.println(F("   h: Print this help"));
  Serial.println(F("   g: Print the gain settings of the device."));
  Serial.print  (F("   i/I: Increase/Decrease Input Gain by ")); Serial.print(increment_input_gain_dB); Serial.println(" dB");
  Serial.print  (F("   k/K: Increase/Decrease Overall Volume by ")); Serial.print(increment_digital_gain_dB); Serial.println(" dB");
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
  Serial.println();  // extra line at end
}

//return whether or not the given character was used
bool SerialManager::processCharacter(char c) {
  Serial.print("SerialManager: processCharacter: ");Serial.println(c);
  //float new_val = 0.0;
  bool return_val = true;
  switch (c) {
    case 'h':
      printHelp(); break;
    case 'g':
      myState.printGainSettings(); break;
    case 'k':
      incrementDigitalGain(increment_digital_gain_dB); 
      setButtonState_gains();
      //setButtonState("inp_mute",false);
      break;
    case 'K':   //which is "shift k"
      incrementDigitalGain(-increment_digital_gain_dB); 
      setButtonState_gains(); 
      //setButtonState("inp_mute",false);
      break;
    case 'J':
      printTympanRemoteLayout();
      setFullGUIState(firstTimeSendingGUI);
      firstTimeSendingGUI = false;
      break;
    default:
      return_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break; 
 }
  return return_val;
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::createTympanRemoteLayout(void) {
  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI
  page_h = myGUI.addPage("Input Select");
    card_h = earpieceMixer.addCard_audioSource(page_h); //use its predefined group of buttons for input audio source
    card_h = page_h->addCard("Broadband Gain");  
      card_h->addButton("-", "K", "",       4);
      card_h->addButton("0", "",  "digGain", 4);
      card_h->addButton("+", "k", "",        4);
    card_h = page_h->addCard("Swipe R->L For More Pages");

  //add second page to GUI
  page_h = earpieceMixer.addPage_digitalEarpieces(&myGUI); //use its predefined page for controlling the digital earpieces

  //add third page to GUI
  page_h = myGUI.addPage("Globals");
    card_h = myState.addCard_cpuReporting(page_h);
    card_h = audioSDWriter.addCard_sdRecord(page_h);
    
  myGUI.addPredefinedPage("serialMonitor");
}

void SerialManager::printTympanRemoteLayout(void) {
    String s = myGUI.asString();
    Serial.println(s);
    ble->sendMessage(s);
}

void SerialManager::setFullGUIState(bool activeButtonsOnly) {
  //update all of the individual UI elements
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

  //manually update any other elements
  setButtonState_gains(activeButtonsOnly);
}

void SerialManager::setButtonState_gains(bool activeButtonsOnly) {
  setButtonText("digGain", String(myState.digitalGain_dB,1));
}


#endif
