  
#ifndef SerialManager_h
#define SerialManager_h

#include <Tympan_Library.h>

//extern classes
extern State myState;
extern AudioEffectFreqShift_FD_F32 freqShift;

//extern functions
extern float incrementFormantShift(float);
extern int incrementFreqShift(int);
extern bool enableFormantShift(bool);
extern bool enableFreqShift(bool);


//now, define the Serial Manager class
class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {  };

    void printHelp(void); 
    int freq_shift_increment = 1;
    float formantScaleIncrement = powf(2.0,1.0/6.0);   //shift by 1/6th of an octave 

    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void);

    void setFullGUIState(bool activeButtonsOnly = false);
    void updateFreqProcGUI(bool activeButtonsOnly = false);
    void updateFormantScaleFacGUI(void);

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
  Serial.print("   1/!: Enable/Disable Formant Shifting"); if (myState.is_enabled_formant) { Serial.println(" (ACTIVE!)"); } else { Serial.println(); };
  Serial.print("   2/@: Enable/Disable Frequency Shifting");  if (myState.is_enabled_freq) { Serial.println(" (ACTIVE!)"); } else { Serial.println(); };
  Serial.print("   r/R: Raise/Lower Formant Shift (current "); Serial.print(myState.cur_scale_factor); Serial.println("x)");
  Serial.print("   f/F: Raise/Lower Freq Shift (current "); Serial.print(freqShift.getFrequencyOfBin(myState.cur_shift_bins)); Serial.println(" Hz)");
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
    case '1':
      Serial.println("Received: enable Formant shifting.");
      enableFormantShift(true);
      updateFreqProcGUI();
      break;
    case '!':
      Serial.println("Received: disable Formant shifting.");
      enableFormantShift(false);
      updateFreqProcGUI();
      break;
    case 'q':
      Serial.println("Received : toggling Formant shifting.");
      enableFormantShift(!myState.is_enabled_formant);
      updateFreqProcGUI();
      break;
    case '2':
      Serial.println("Received: enable Frequency shifting.");
      enableFreqShift(true);
      updateFreqProcGUI();
      break;
    case '@':
      Serial.println("Received: disable Frequency shifting.");
      enableFreqShift(false);
      updateFreqProcGUI();
      break;
    case 'w':
      Serial.print("Received : toggling Frequency shifting.");
      enableFreqShift(!myState.is_enabled_freq);
      updateFreqProcGUI();
      break;      
    case 'f':
      { int new_val = incrementFreqShift(freq_shift_increment);
      Serial.print("Received: new freq shift = "); Serial.println(new_val);}
      updateFreqProcGUI();
      break;
    case 'F':
      { int new_val = incrementFreqShift(-freq_shift_increment);
      Serial.print("Received: new freq shift = "); Serial.println(new_val);}
      updateFreqProcGUI();
      break;      
    case 'r':
      { float new_val = incrementFormantShift(formantScaleIncrement);
      Serial.print("Received: new format scale = "); Serial.println(new_val);}
      //updateFreqProcGUI();
      updateFormantScaleFacGUI();
      break;
    case 'R':
      { float new_val = incrementFormantShift(1./formantScaleIncrement);
      Serial.print("Received: new format scale = "); Serial.println(new_val);}
      //updateFreqProcGUI();
      updateFormantScaleFacGUI();
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
  page_h = myGUI.addPage("Frequency Processing");
    card_h = page_h->addCard("Algorithm");  
      card_h->addButton("Formant Shift",   "q", "algForm", 12);   //12 is full width
      card_h->addButton("Frequency Shift", "w", "algFreq", 12); //12 is full width
    card_h = page_h->addCard("Formant Shift (x)");  
      card_h->addButton("-", "R", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "formShift", 4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "r", "",          4);  //label, command, id, width
    card_h = page_h->addCard("Frequency Shift (Hz)");  
      card_h->addButton("-", "F", "",          4);  //label, command, id, width
      card_h->addButton("",  "",  "freqShift", 4);  //label, command, id, width //display the formant shift value
      card_h->addButton("+", "f", "",          4);  //label, command, id, width
  
  //add second page to GUI
  page_h = myGUI.addPage("Globals");
  card_h = myState.addCard_cpuReporting(page_h);
    
  //myGUI.addPredefinedPage("serialMonitor");
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
  updateFreqProcGUI(activeButtonsOnly);
}

void SerialManager::updateFreqProcGUI(bool activeButtonsOnly) {
  
  //update formant shifting fields
  Serial.print("SerialManager: updateFreqProcGUI: is_enabled_formant = ");Serial.println(myState.is_enabled_formant);
  if (myState.is_enabled_formant) {
    setButtonState("algForm",true);delay(3);
  } else {
    if (!activeButtonsOnly) { setButtonState("algForm",false); delay(3); }
  }
  updateFormantScaleFacGUI();



  //update freq shifting fields
  Serial.print("SerialManager: updateFreqProcGUI: is_enabled_freq = ");Serial.println(myState.is_enabled_freq);
  if (myState.is_enabled_freq) {
    setButtonState("algFreq",true);delay(3);
  } else {
    if (!activeButtonsOnly) { setButtonState("algFreq",false); delay(3); }
  }
  Serial.println("SerialManager: updateFreqProcGUI: update freqShift");delay(10);
  setButtonText("freqShift", String(freqShift.getFrequencyOfBin(myState.cur_shift_bins)+0.0001f,1));delay(3);

}

void SerialManager::updateFormantScaleFacGUI(void) {
  Serial.println("SerialManager: updateFreqProcGUI: update formShift");delay(10);
  setButtonText("formShift", String(myState.cur_scale_factor,2));delay(3);  
}


#endif
