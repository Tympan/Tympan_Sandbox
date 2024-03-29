
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32_UI audioSDWriter;
extern State myState;


//Extern Functions
extern void setConfiguration(int);
extern float incrementInputGain(float);
extern float changeSampleRate(float);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void printHelp(void);
    void createTympanRemoteLayout(void);
    void printTympanRemoteLayout(void);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGUI_inputGain(bool activeButtonsOnly = false);
    void updateGUI_inputSelect(bool activeButtonsOnly = false);
    void updateGUI_fsSelect(bool activeButtonsOnly = false);

    int gainIncrement_dB = 2.5;

  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
};

void SerialManager::printHelp(void) {
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h: Print this help");
  Serial.println("   w: Switch Input to PCB Mics");
  Serial.println("   W: Switch Input to Headset Mics");
  Serial.println("   e: Switch Input to LineIn on the Mic Jack");
  Serial.println("   i: Input: Increase gain by " + String(gainIncrement_dB) + " dB");
  Serial.println("   I: Input: Decrease gain by " + String(gainIncrement_dB) + " dB");
  Serial.println("   1: SampleRate: 48000 Hz");
  Serial.println("   2: SampleRate: 96000 Hz");
  Serial.println("   3: SampleRate: 120000 Hz");
  Serial.println("   4: SampleRate: 144000 Hz");
  Serial.println("   5: SampleRate: 168000 Hz");
  Serial.println("   6: SampleRate: 192000 Hz");


  //Add in the printHelp() that is built-into the other UI-enabled system components.
  //The function call below loops through all of the UI-enabled classes that were
  //attached to the serialManager in the setupSerialManager() function used back
  //in the main *.ino file.
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()

  Serial.println();
}




//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) { //this is called by SerialManagerBase.respondToByte(char c)
  bool ret_val = true; //assume at first that we will find a match
  switch (c) {
    case 'h':
      printHelp();
      break;
    case 'J': case 'j':           //The TympanRemote app sends a 'J' to the Tympan when it connects
      printTympanRemoteLayout();  //in resonse, the Tympan sends the definition of the GUI that we'd like
      break;
    case 'i':
      incrementInputGain(gainIncrement_dB);
      Serial.println("Received: Increased input gain to: " + String(myState.input_gain_dB) + " dB");
      updateGUI_inputGain();
      break;
    case 'I':   //which is "shift i"
      incrementInputGain(-gainIncrement_dB);
      Serial.println("Received: Decreased input gain to: " + String(myState.input_gain_dB) + " dB");
      updateGUI_inputGain();
      break;
    case 'w':
      Serial.println("Received: Switch input to PCB Mics");
      setConfiguration(State::INPUT_PCBMICS);
      updateGUI_inputSelect();
      updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'W':
      Serial.println("Recevied: Switch input to Mics on Jack.");
      setConfiguration(State::INPUT_JACK_MIC);
      updateGUI_inputSelect();
      updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'e':
      myTympan.println("Received: Switch input to Line-In on Jack");
      setConfiguration(State::INPUT_JACK_LINE);
      updateGUI_inputSelect();
      updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case '1':
      {
        float new_fs_Hz = 48000.0;
        Serial.println("Received: change sample rate to: " + String(new_fs_Hz));
        changeSampleRate(new_fs_Hz);
        setFullGUIState();
      }
      break;
    case '2':
      {
        float new_fs_Hz = 96000.0;
        Serial.println("Received: change sample rate to: " + String(new_fs_Hz));
        changeSampleRate(new_fs_Hz);
        setFullGUIState();
      }
      break;
    case '3':
      {
        float new_fs_Hz = 120000.0;
        Serial.println("Received: change sample rate to: " + String(new_fs_Hz));
        changeSampleRate(new_fs_Hz);
        setFullGUIState();
      }
      break;
    case '4':
      {
        float new_fs_Hz = 144000.0;
        Serial.println("Received: change sample rate to: " + String(new_fs_Hz));
        changeSampleRate(new_fs_Hz);
        setFullGUIState();
      }
      break;
    case '5':
      {
        float new_fs_Hz = 168000.0;
        Serial.println("Received: change sample rate to: " + String(new_fs_Hz));
        changeSampleRate(new_fs_Hz);
        setFullGUIState();
      }
      break;
    case '6':
      {
        float new_fs_Hz = 192000.0;
        Serial.println("Received: change sample rate to: " + String(new_fs_Hz));
        changeSampleRate(new_fs_Hz);
        setFullGUIState();
      }
      break;
    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break;
  }
  return ret_val;
}



// //////////////////////////////////  Methods for defining the GUI and transmitting it to the App

//define the GUI for the App
void SerialManager::createTympanRemoteLayout(void) {

  // Create some temporary variables
  TR_Page *page_h;  //dummy handle for a page
  TR_Card *card_h;  //dummy handle for a card

  //Add first page to GUI  (the indentation doesn't matter; it is only to help us see it better)
  page_h = myGUI.addPage("Audio Recorder");
  //Add a button group for selecting the input source
  card_h = page_h->addCard("Select Input");
  card_h->addButton("PCB Mics",      "w", "configPCB",   12);  //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("Jack as Mic",   "W", "configMIC",   12);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  card_h->addButton("Jack as Line-In", "e", "configLINE",  12); //displayed string, command, button ID, button width (out of 12)

  card_h = page_h->addCard("Sample Rate (Hz)");
  card_h->addButton("48 kHz",      "1", "fs48",   6);  //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("96 kHz",      "2", "fs96",   6);  //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("120 kHz",     "3", "fs120",  6);  //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("144 kHz",     "4", "fs144",  6);  //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("168 kHz",     "5", "fs168",  6);  //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("192 kHz",     "6", "fs192",  6);  //displayed string, command, button ID, button width (out of 12)

  //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
  card_h = audioSDWriter.addCard_sdRecord(page_h);

  //Add a button group for analog gain
  card_h = page_h->addCard("Input Gain (dB)");
  card_h->addButton("-", "I", "",        4); //displayed string, command, button ID, button width (out of 12)
  card_h->addButton("", "", "inpGain", 4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
  card_h->addButton("+", "i", "",        4); //displayed string, command, button ID, button width (out of 12)



  //add some pre-defined pages to the GUI (pages that are built-into the App)
  //myGUI.addPredefinedPage("serialPlotter");  //if we send data in the right format, the App will plot the signal levels in real-time!
  myGUI.addPredefinedPage("serialMonitor");
}


// Print the layout for the Tympan Remote app, in a JSON-ish string
void SerialManager::printTympanRemoteLayout(void) {
  if (myGUI.get_nPages() < 1) createTympanRemoteLayout();  //create the GUI, if it hasn't already been created
  String s = myGUI.asString();
  Serial.println(s);
  ble->sendMessage(s); //ble is held by SerialManagerBase
  setFullGUIState();
}

// //////////////////////////////////  Methods for updating the display on the GUI

void SerialManager::setFullGUIState(bool activeButtonsOnly) {  //the "activeButtonsOnly" isn't used here, so don't worry about it

  updateGUI_inputGain(activeButtonsOnly);
  updateGUI_inputSelect(activeButtonsOnly);
  updateGUI_fsSelect(activeButtonsOnly);

  //Then, let's have the system automatically update all of the individual UI elements that we attached
  //to the serialManager via the setupSerialManager() function used back in the main *.ino file.
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

}

void SerialManager::updateGUI_inputGain(bool activeButtonsOnly) {
  setButtonText("inpGain", String(myState.input_gain_dB, 1));
}


void SerialManager::updateGUI_inputSelect(bool activeButtonsOnly) {
  if (!activeButtonsOnly) {
    setButtonState("configPCB", false);
    setButtonState("configMIC", false);
    setButtonState("configLINE", false);
  }
  switch (myState.input_source) {
    case (State::INPUT_PCBMICS):
      setButtonState("configPCB", true);
      break;
    case (State::INPUT_JACK_MIC):
      setButtonState("configMIC", true);
      break;
    case (State::INPUT_JACK_LINE):
      setButtonState("configLINE", true);
      break;
  }
}

void SerialManager::updateGUI_fsSelect(bool activeButtonsOnly) {
  if (!activeButtonsOnly) {
    setButtonState("fs48", false);
    setButtonState("fs96", false);
    setButtonState("fs120", false);
    setButtonState("fs144", false);
    setButtonState("fs168", false);
    setButtonState("fs192", false);
  }
  if (abs(myState.sample_rate_Hz - 48000.0) < 1) {
    setButtonState("fs48", true);
  } else if (abs(myState.sample_rate_Hz - 96000.0) < 1) {
    setButtonState("fs96", true);
  } else if (abs(myState.sample_rate_Hz - 120000.0) < 1) {
    setButtonState("fs120", true);
  } else if (abs(myState.sample_rate_Hz - 144000.0) < 1) {
    setButtonState("fs144", true);
  } else if (abs(myState.sample_rate_Hz - 168000.0) < 1) {
    setButtonState("fs168", true);
  } else if (abs(myState.sample_rate_Hz - 192000.0) < 1) {
    setButtonState("fs192", true);
  }
}

#endif
