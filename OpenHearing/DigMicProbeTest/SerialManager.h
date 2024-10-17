
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"


//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32_UI audioSDWriter;
extern State myState;
extern SDtoSerial SD_to_serial;


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

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    //SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};
    SerialManager() : SerialManagerBase() {};

    void printHelp(void);
    void createTympanRemoteLayout(void); 
    void printTympanRemoteLayout(void); 
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

    //method for updating the GUI on the App
    void setFullGUIState(bool activeButtonsOnly = false);
    void updateGUI_inputGain(bool activeButtonsOnly = false);
    void updateGUI_inputSelect(bool activeButtonsOnly = false);

    int gainIncrement_dB = 2.5;
    int receiveFilename(String &filename,const unsigned long timeout_millis);

  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App    
};

void SerialManager::printHelp(void) {  
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println(" General: No Prefix");
  Serial.println("   h    : Print this help");
  Serial.println("   w/W/e/E: INPUT  : Switch to the PCB Mics / Pink Jack - Mic Bias / Pink Jack Line in / Digital mics");
  Serial.println("   k/K  : CHIRP  : Incr/decrease loudness of chirp (cur = " + String(myState.chirp_amp_dBFS,1) + " dBFS)");
  Serial.println("   d/D  : CHIRP  : Incr/decrease duration of chirp (cur = " + String(myState.chirp_dur_sec,1) + " sec)");
  Serial.println("   n    : CHIRP  : Start the chirp");
  Serial.println("   1-3  : SDPlay : Play files 1-3 from SD Card");
  Serial.println("   q    : SDPlay : Stop any currently plying SD files");
  Serial.println("   c    : AutoWrite : Start chirp and SD recording together");
  Serial.println("   4-6  : AutoWrite : Start files 1-3 from SD Card and SD recording together");
  Serial.println("   g/G  : OUTPUT : Incr/decrease DAC loudness (cur = " + String(myState.output_gain_dB,1) + " dB)");
  Serial.println("   r/s  : SDWrite: Manually Start/Stop recording");
  Serial.println("---------------------------------------------------");
  Serial.println("   L    : List of the files on SD card");
  Serial.println("   f    : Open the file from SD (will prompt you for filename)");
  Serial.println("   b    : Get the size of the file in bytes");
  Serial.println("   t    : Transfer the whole file from SD to Serial");

  
  #if defined(USE_MTPDISK) || defined(USB_MTPDISK_SERIAL)  //detect whether "MTP Disk" or "Serial + MTP Disk" were selected in the Arduino IDEA
    Serial.println("   >    : SDUtil : Start MTP mode to read SD from PC");
  #endif
  
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
    case 'c':
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
    case '>':
      Serial.println("Starting MTP service to access SD card (everything else will stop)");
      start_MTP();
      break;
    case 'L':
      Serial.print("Listing Files on SD:"); //don't include end-of-line
      SD_to_serial.sendFilenames(','); //send file names seperated by commas
      break;
    case 'f':
      {
        Serial.println("SerialMonitor: Opening file: Send filename (ending with newline character) within 10 seconds");
        String fname;  receiveFilename(fname, 10000);  //wait 10 seconds
        if (SD_to_serial.open(fname)) {
          Serial.println("SerialMonitor: " + fname + " successfully opened");
        } else {
          Serial.println("SerialMonitor: *** ERROR ***: " + fname + " could not be opened");
        }
      }
      break;
    case 'b':
      if (SD_to_serial.isFileOpen()) {
        SD_to_serial.sendFileSize();
      } else {
        Serial.println("SerialMonitor: *** ERROR ***: Cannot get file size because no file is open");
      }
      break;
    case 't':
      if (SD_to_serial.isFileOpen()) {
        SD_to_serial.sendFile();
        Serial.println();
      } else {
        Serial.println("SerialMonitor: *** ERROR ***: Cannot send file because no file is open");
      }
      break;

    default:
      ret_val = SerialManagerBase::processCharacter(c);  //in here, it automatically loops over the different UI elements
      break;
  }
  return ret_val;
}


int SerialManager::receiveFilename(String &filename,const unsigned long timeout_millis) {
  Serial.setTimeout(timeout_millis);  //set timeout in milliseconds
  filename.remove(0); //clear the string
  filename += Serial.readStringUntil('\n');  //append the string
  if (filename.length() == 0) filename += Serial.readStringUntil('\n');  //append the string
  Serial.setTimeout(1000);  //return the timeout to the default
  return 0;
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
    if (ble != NULL) ble->sendMessage(s); //ble is held by SerialManagerBase
    setFullGUIState();
}

// //////////////////////////////////  Methods for updating the display on the GUI

void SerialManager::setFullGUIState(bool activeButtonsOnly) {  //the "activeButtonsOnly" isn't used here, so don't worry about it
  
  updateGUI_inputGain(activeButtonsOnly);
  updateGUI_inputSelect(activeButtonsOnly);

  //Then, let's have the system automatically update all of the individual UI elements that we attached
  //to the serialManager via the setupSerialManager() function used back in the main *.ino file.
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
  }
}


#endif
