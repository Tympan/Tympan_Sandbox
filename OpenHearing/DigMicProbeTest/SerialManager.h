
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32_UI audioSDWriter;
extern State myState;
extern AudioSDWriter_F32_UI audioSDWriter;


//Extern Functions
extern void setConfiguration(int);
extern float incrementInputGain(float);
extern float incrementChirpLoudness(float);
extern float incrementChirpDuration(float);
extern void startSignalWithDelay(float);
extern float incrementOutputGain_dB(float increment_dB);
extern void forceStopSDPlay(void);

//externals for MTP
extern void start_MTP(void);
//extern void stop_MTP(void);

class SerialManager : public SerialManagerBase  {  // see Tympan_Library SerialManagerBase for more functions!
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    void setFullGUIState(bool activeButtonsOnly = false);
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void updateGUI_inputGain(bool activeButtonsOnly = false);
    void updateGUI_inputSelect(bool activeButtonsOnly = false);

    int gainIncrement_dB = 2.5f;
  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App    
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h    : Print this help");
  Serial.println("   w/W/e/E/o: INPUT  : Switch to the PCB Mics / Pink Jack - Mic Bias / Pink Jack Line in / Digital mics / Hybrid");
  Serial.println("   k/K  : CHIRP  : Incr/decrease loudness of chirp (cur = " + String(myState.chirp_amp_dBFS,1) + " dBFS)");
  Serial.println("   d/D  : CHIRP  : Incr/decrease duration of chirp (cur = " + String(myState.chirp_dur_sec,1) + " sec)");
  Serial.println("   n    : CHIRP  : Start the chirp");
  Serial.println("   1-3  : SDPlay : Play files 1-3 from SD Card");
  Serial.println("   q    : SDPlay : Stop any currently plying SD files");
  Serial.println("   b    : AutoWrite : Start chirp and SD recording together");
  Serial.println("   4-6  : AutoWrite : Start files 1-3 from SD Card and SD recording together");
  Serial.println("   g/G  : OUTPUT : Incr/decrease DAC loudness (cur = " + String(myState.output_gain_dB,1) + " dB)");
  Serial.println("   r/s  : SDWrite: Manually Start/Stop recording");
  Serial.println("   l    : Toggle level meter");
  
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
    Serial.println("   >    : SDUtil : Start MTP mode to read SD from PC");
  #endif
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
      Serial.println("Received: Switch input to Line-In on Jack");
      setConfiguration(State::INPUT_JACK_LINE);
      updateGUI_inputSelect();
      updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'E':
      Serial.println("Received: Switch input to digital PDM mics");
      setConfiguration(State::INPUT_PDM_MICS);
      updateGUI_inputSelect();
      updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;
    case 'o':
      Serial.println("Received: Switch input to hybrid: Tympan-Mic_Jack and Shield-PDM Mics");
      setConfiguration(State::INPUT_MIC_JACK_WTIH_PDM_MIC);
      updateGUI_inputSelect();
      updateGUI_inputGain(); //changing inputs changes the input gain, too
      break;

    case 'k':
      incrementChirpLoudness(3.0);
      Serial.println("Increased chirp loudness to " + String(myState.chirp_amp_dBFS,1) + " dBFS");
      break;
    case 'K':
      incrementChirpLoudness(-3.0);
      Serial.println("Decreased chirp loudness to " + String(myState.chirp_amp_dBFS,1) + " dBFS");
      break;
    case 'd':
      incrementChirpDuration(1.0);
      Serial.println("Increased chirp duration to " + String(myState.chirp_dur_sec,1) + " sec");
      break;
    case 'D':
      incrementChirpDuration(-1.0);
      Serial.println("Decreased chirp duration to " + String(myState.chirp_dur_sec,1) + " sec");
      break;
    case 'n':
      Serial.println("Starting chirp...");
      startSignalWithDelay(0.0);  //no delay, start right away
      break;
    case 'b':
      Serial.println("Received: start combination of chirp and SD recording...");
      if (myState.auto_sd_state != State::DISABLED) {
        Serial.println("   : ERROR : Already doing an auto-triggered SD recording.");
        Serial.println("           : Ignoring the last command.");
      } else {
        audioSDWriter.startRecording();
        myState.autoPlay_signal_type = State::PLAY_CHIRP;
        startSignalWithDelay(myState.auto_SD_start_stop_delay_sec);
        myState.auto_sd_state = State::WAIT_START_SIGNAL;
      }
      break; 
    case '1': case '2': case '3':
      myState.autoPlay_signal_type = c - ((int)'1') + State::PLAY_CHIRP + 1;  //This is me *computing* my way into the enum instead of a switch block.  Not recommended, but whatever.
      startSignalWithDelay(0.0); //play immediately
      break; 
    case 'q':
      Serial.println("Received: force stop the playing of any SD files...");
      forceStopSDPlay();
      break;
    case '4': case '5': case '6':
      Serial.println("Received: start combination SD Playing and SD recording...");
      if (myState.auto_sd_state != State::DISABLED) {
        Serial.println("   : ERROR : Already doing an auto-triggered SD recording.");
        Serial.println("           : Ignoring the last command.");
      } else {
        audioSDWriter.startRecording();
        myState.autoPlay_signal_type = c - ((int)'4') + State::PLAY_CHIRP + 1;  //This is me *computing* my way into the enum instead of a switch block.  Not recommended, but whatever.
        startSignalWithDelay(myState.auto_SD_start_stop_delay_sec);
        myState.auto_sd_state = State::WAIT_START_SIGNAL;
      }
      break; 
    case 'r':
      audioSDWriter.startRecording();
      Serial.println("Starting recording to SD file " + audioSDWriter.getCurrentFilename());
      break;
    case 's':
      Serial.println("Stopping recording to SD file " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      break;
    case 'g':
      incrementOutputGain_dB(3.0);
      Serial.println("Increased DAC loudness to " + String(myState.output_gain_dB,1) + " dBFS");
      break;
    case 'G':
      incrementOutputGain_dB(-3.0);
      Serial.println("Decreased DAC loudness to " + String(myState.output_gain_dB,1) + " dBFS");
      break;
    case 'l':
      //Toggle flag for the level meter.
      myState.flag_PrintInputLevel = !myState.flag_PrintInputLevel;
      if(myState.flag_PrintInputLevel) {
        Serial.println("Level Meter enabled");
      } else {
        Serial.println("Level Meter disabled");
      }
      break;
    case '>':
      Serial.println("Starting MTP service to access SD card (everything else will stop)");
      start_MTP();
      break;
    // Default:  Automatically loop over the different UI elements
    default:
      ret_val = false;  //in here, it automatically loops over the different UI elements
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
          card_h->addButton("PCB Mics",       "w", "configPCB",   12);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("Jack as Mic",    "W", "configMIC",   12);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("Jack as Line-In","e", "configLINE",  12);  //displayed string, command, button ID, button width (out of 12) 
          card_h->addButton("Digital Mics"   ,"E", "configPDM",  12);  //displayed string, command, button ID, button width (out of 12) 
          card_h->addButton("Mic Jack w/ PDM","o", "configMicJackPDM",  12);  //displayed string, command, button ID, button width (out of 12) 

      //Add a button group for SD recording...use a button set that is built into AudioSDWriter_F32_UI for you!
      card_h = audioSDWriter.addCard_sdRecord(page_h);

      //Add a button group for analog gain
      card_h = page_h->addCard("Input Gain (dB)");
          card_h->addButton("-","I","",        4);  //displayed string, command, button ID, button width (out of 12)
          card_h->addButton("", "", "inpGain", 4);  //displayed string (blank for now), command (blank), button ID, button width (out of 12)
          card_h->addButton("+","i","",        4);  //displayed string, command, button ID, button width (out of 12)
               
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

  //update all of the individual UI elements
  SerialManagerBase::setFullGUIState(activeButtonsOnly); //in here, it automatically loops over the different UI elements

}

void SerialManager::updateGUI_inputGain(bool activeButtonsOnly) {
  setButtonText("inpGain",String(myState.input_gain_dB,1));
}


void SerialManager::updateGUI_inputSelect(bool activeButtonsOnly) {
  if (!activeButtonsOnly) {
    setButtonState("configPCB",false);
    setButtonState("configMIC",false);
    setButtonState("configLINE",false);
    setButtonState("configPDM",false);
    setButtonState("configMicJackPDM",false);
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
    case (State::INPUT_PDM_MICS): 
      setButtonState("configPDM",true);
      break;
    case (State::INPUT_MIC_JACK_WTIH_PDM_MIC): 
      setButtonState("configMicJackPDM",true);
      break;

  }
}


#endif
