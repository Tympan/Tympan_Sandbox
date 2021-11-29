
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//Extern variables from the main *.ino file
extern Tympan myTympan;
extern AudioSDWriter_F32_UI audioSDWriter;
extern float tone_freq_Hz;
extern float tone_amp_dB;

//Extern Functions
extern float incrementFrequency(float incr_Hz);
extern float incrementAmplitude(float incr_dB);
extern bool muteTone(bool set_mute);
extern int switchState(int new_state);
extern void writeTextToSD(String myString);  //for writing a text log to the SD card

class SerialManager : public SerialManagerBase  {  // see Tympan_Library for SerialManagerBase for more functions!
  public:
    SerialManager(void) : SerialManagerBase() {};
    //SerialManager(BLE *_ble) : SerialManagerBase(_ble) {};

    void printHelp(void);
    //void createTympanRemoteLayout(void); 
    //void printTympanRemoteLayout(void); 
    bool processCharacter(char c);  //this is called automatically by SerialManagerBase.respondToByte(char c)

  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App    
};

void SerialManager::printHelp(void) {  
  Serial.println("CalibrationViaExternalMic: Help menu:");
  Serial.println("  h: print this help");
  Serial.println("  w/W/e: Switch between PCB Mics (w) and Line-in on Mic Jack (W) and one of each (e)");
  Serial.println("  u/t/T: Testing Off, Manual, or Automatic Tones.");
  Serial.println(" Test Tones: (no prefix)");
  Serial.println("  f/F: incr/decrease the frequency (currently: " + String(tone_freq_Hz/1000,1) + " kHz)");
  Serial.println("  a/A: incr/decrease the amplitude (currently: " + String(tone_amp_dB,1) + " dB)");
  Serial.println("  m/M: mute/unmute the tone");

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
  float new_val;
  switch (c) {
    case 'h':
      printHelp();
      break;
   case 'w':
      Serial.println("Switching to PCB mics");
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
      break;
   case 'W':
      Serial.println("Switching to line in on pink jack");
      //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
      break;
   case 'e':
      Serial.println("Switching to Left as line-in on pink jack and Right as PCB mic");
      myTympan.inputSelect(TYMPAN_LEFT_JACK_AS_LINEIN_RIGHT_ON_BOARD_MIC);
      break;
   case 'f':
      new_val = incrementFrequency(1000.0);
      Serial.println("Changing frequency to " + String(new_val) + " Hz");
      break;
   case 'F':
      new_val = incrementFrequency(-1000.0);
      Serial.println("Changing frequency to " + String(new_val) + " Hz");
      break;
   case 'a':
      new_val = incrementAmplitude(3.0);
      Serial.println("Changing amplitude to " + String(new_val));
      break;
   case 'A':
      new_val = incrementAmplitude(-3.0);
      Serial.println("Changing amplitude to " + String(new_val));
      break;
   case 'm':
      muteTone(true);
      Serial.println("Muting the tone");
      break;
   case 'M':
      muteTone(false);
      Serial.println("Unmuting the tone");
      break; 
   case 'u':
      Serial.println("Switching to OFF test mode.");
      switchState(STATE_OFF);
      break;
   case 't':
      Serial.println("Switching to MANUAL test mode.");
      switchState(STATE_MANUAL);
      break;
   case 'T':
      Serial.println("Swithing to AUTOMATIC test mode.");
      switchState(STATE_AUTOMATIC);
      break;   
  }
  return ret_val;
}



#endif
