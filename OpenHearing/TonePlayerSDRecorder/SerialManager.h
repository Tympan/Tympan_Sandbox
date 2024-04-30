
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32_UI audioSDWriter;
extern State myState;


//Extern Functions
extern void setInputConfiguration(int);
extern float incrementInputGain(float);
extern bool activateTone(bool);
extern float incrementToneLoudness(float);
//extern float incrementToneFrequency(float);
extern float incrementToneFrequency(int);
extern float incrementDacOutputGain(float incr_dB);
extern float testSineAccuracy(float32_t freq_Hz);
extern float overallTonePlusDacLoudness(void);

//externals for MTP
extern void start_MTP(void);
//extern void stop_MTP(void);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    //void updateGUI_inputGain(bool activeButtonsOnly = false);
    void updateGUI_inputSelect(bool activeButtonsOnly = false);

    int bigLoudnessIncrement_dB = 6.0;
    int smallLoudnessIncrement_dB = 1.0;

  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App    
};


void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h  : Print this help");
  Serial.println("   t  : TEST : Calc diff in F32 vs F64 sine()");
  Serial.println("   w  : INPUT : Switch to the PCB Mics");
  Serial.println("   W  : INPUT : Switch to the pink jacks (with mic bias)");
  Serial.println("   e  : INPUT : Switch to the pink jacks (as line input)");
  //Serial.println("   E  : INPUT: Switch to the digital mics");
  Serial.println("   a/A: TONE : Activate (a) or Mute (A) the tone (is_active = " + String(myState.is_tone_active) + ")");
  Serial.println("   k/K: TONE : Incr/decrease loudness of tone by " + String(bigLoudnessIncrement_dB) + " dB (cur = " + String(myState.tone_dBFS,1) + " dBFS)");
  Serial.println("   l/L: TONE : Incr/decrease loudness of tone by " + String(smallLoudnessIncrement_dB) + " dB (cur = " + String(myState.tone_dBFS,1) + " dBFS)");
  Serial.println("   f/F: TONE : Incr/decrease frequency of tone (cur = " + String(myState.tone_Hz,1) + " Hz)");
  Serial.println("   b/B: OUTPUT : Incr/decrease DAC output gain by " + String(bigLoudnessIncrement_dB) + " dB (cur = " + String(myState.output_dacGain_dB,1) + " dB)");
  Serial.println("   g/G: OUTPUT : Incr/decrease DAC output gain by " + String(smallLoudnessIncrement_dB) + " dB (cur = " + String(myState.output_dacGain_dB,1) + " dB)");
  Serial.println("   r/s: SD : Start/Stop recording");
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
    Serial.println("   >  : SD   : Start MTP mode to read SD from PC");
  #endif
  
  //Add in the printHelp() that is built-into the other UI-enabled system components.
  //The function call below loops through all of the UI-enabled classes that were
  //attached to the serialManager in the setupSerialManager() function used back
  //in the main *.ino file.
  SerialManagerBase::printHelp();  ////in here, it automatically loops over the different UI elements issuing printHelp()
  
  Serial.println();
  Serial.println("Overall Tone+DAC Loudness is Currently = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
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
    case 't':
      {
      Serial.println("Received: performing sine wave accuracy test at 1000 Hz");
      float32_t rms_diff = testSineAccuracy(1000.0);
      Serial.println("    : difference is = " + String(20.0f*log10f(rms_diff / sqrt(2.0))) + " dB");
      }
      break;
    case 'w':
      Serial.println("Received: Switch input to PCB Mics");
      setInputConfiguration(State::INPUT_PCBMICS);
      updateGUI_inputSelect();
      //updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'W':
      Serial.println("Recevied: Switch input to Mics on Jack.");
      setInputConfiguration(State::INPUT_JACK_MIC);
      updateGUI_inputSelect();
      //updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'e':
      Serial.println("Received: Switch input to Line-In on Jack");
      setInputConfiguration(State::INPUT_JACK_LINE);
      updateGUI_inputSelect();
      //updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'a':
      activateTone(true);
      Serial.println("Activating the tone to loudness of " + String(myState.tone_dBFS,1) + " dBFS");
      break;
    case 'A':
      activateTone(false);
      Serial.println("Muting the tone...");
      break;
    case 'k':
      incrementToneLoudness(bigLoudnessIncrement_dB);
      Serial.println("Increased tone loudness to " + String(myState.tone_dBFS,1) + " dBFS");
      break;
    case 'K':
      incrementToneLoudness(-bigLoudnessIncrement_dB);
      Serial.println("Decreased tone loudness to " + String(myState.tone_dBFS,1) + " dBFS");
      break;
    case 'l':
      incrementToneLoudness(smallLoudnessIncrement_dB);
      Serial.println("Increased tone loudness to " + String(myState.tone_dBFS,1) + " dBFS");
      break;
    case 'L':
      incrementToneLoudness(-smallLoudnessIncrement_dB);
      Serial.println("Decreased tone loudness to " + String(myState.tone_dBFS,1) + " dBFS");
      break;
    case 'f':
      //incrementToneFrequency(powf(2.0,1.0/3.0));  //third octave step increase
      incrementToneFrequency(1);  //step up in table
      Serial.println("Increased tone frequency to " + String(myState.tone_Hz,1) + " Hz");
      break;
    case 'F':
      //incrementToneFrequency(powf(2.0,-1.0/3.0));  //third octave step decrease
      incrementToneFrequency(-1);  //step down in table
      Serial.println("Decreased tone frequency to " + String(myState.tone_Hz,1) + " Hz");
      break;
    case 'b':
      incrementDacOutputGain(bigLoudnessIncrement_dB);  // bigger increase
      Serial.println("Increased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'B':
      incrementDacOutputGain(-bigLoudnessIncrement_dB);  // bigger decrease
      Serial.println("Decreased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'g':
      incrementDacOutputGain(smallLoudnessIncrement_dB);  // smaller increase
      Serial.println("Increased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'G':
      incrementDacOutputGain(-smallLoudnessIncrement_dB);  // smaller decrease
      Serial.println("Decreased DAC gain to " + String(myState.output_dacGain_dB,1) + " dB so that Tone+DAC yields = " + String(overallTonePlusDacLoudness(),1) + " dBFS");
      break;
    case 'r':
      audioSDWriter.startRecording();
      Serial.println("Starting recording to SD file " + audioSDWriter.getCurrentFilename());
      break;
    case 's':
      Serial.println("Stopping recording to SD file " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      break;
    case '>':
      Serial.println("Starting MTP service to access SD card (everything else will stop)");
      start_MTP();
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
          card_h->addButton("Jack as Line-In","e", "configLINE",  12);  //displayed string, command, button ID, button width (out of 12) 

      //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
      card_h = audioSDWriter.addCard_sdRecord(page_h);

      //Add a button group for analog gain
      //card_h = page_h->addCard("Input Gain (dB)");
      //    card_h->addButton("-","I","",        4);  //displayed string, command, button ID, button width (out of 12)
      //    card_h->addButton("", "", "inpGain", 4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
      //    card_h->addButton("+","i","",        4);  //displayed string, command, button ID, button width (out of 12)
               
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
  
  //updateGUI_inputGain(activeButtonsOnly);
  updateGUI_inputSelect(activeButtonsOnly);

  //Then, let's have the system automatically update all of the individual UI elements that we attached
  //to the serialManager via the setupSerialManager() function used back in the main *.ino file.
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

}

//void SerialManager::updateGUI_inputGain(bool activeButtonsOnly) {
//  setButtonText("inpGain",String(myState.input_gain_dB,1));
//}


void SerialManager::updateGUI_inputSelect(bool activeButtonsOnly) {
  if (!activeButtonsOnly) {
    setButtonState("configPCB",false);
    setButtonState("configMIC",false);
    setButtonState("configLINE",false);
  }
  switch (myState.input_source) {
    case (State::INPUT_PCBMICS):
      setButtonState("configPCB",true);
      break;
    case (State::INPUT_JACK_MIC): 
      setButtonState("configMIC",true);
      break;
    case (State::INPUT_JACK_LINE): 
      setButtonState("configLINE",true);
      break;
  }
}


#endif
