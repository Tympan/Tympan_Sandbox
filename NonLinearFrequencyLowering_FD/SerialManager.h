

#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//extern objects
extern Tympan myTympan;

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void togglePrintMemoryAndCPU(void);
extern float incrementFreqKnee(float);
extern float incrementFreqCR(float);
extern float incrementFreqShift(float);
extern void switchToPCBMics(void);
extern void switchToMicInOnMicJack(void);
extern void switchToLineInOnMicJack(void);

//now, define the Serial Manager class
class SerialManager : public SerialManagerBase {
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) {  };
          
    void printHelp(void);

    void createTympanRemoteLayout(void) {}; //TBD
    void printTympanRemoteLayout(void) {};  //TBD

    void setFullGUIState(bool activeButtonsOnly = false) {}; //TBD

    
    int N_CHAN;
    float channelGainIncrement_dB = 2.5f; 
    float freq_knee_increment_Hz = 100.0;
    float freq_shift_increment_Hz = 100.0;
    float freq_cr_increment = powf(2.0,1.0/6.0);
    
  private:
    TympanRemoteFormatter myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
    virtual bool processCharacter(char c);
    bool firstTimeSendingGUI = true;
};
#define myTympan myTympan

void SerialManager::printHelp(void) {  
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h: Print this help");
  myTympan.println("   g: Print the gain settings of the device.");
  myTympan.println("   C: Toggle printing of CPU and Memory usage");
  myTympan.println("   p: Switch to built-in PCB microphones");
  myTympan.println("   m: switch to external mic via mic jack");
  myTympan.println("   l: switch to line-in via mic jack");
  myTympan.print("   k/K: Increase the gain of all channels (ie, knob gain) by "); myTympan.print(channelGainIncrement_dB); myTympan.println(" dB");
  myTympan.print("   t/T: Raise/Lower freq knee (change by "); myTympan.print(freq_knee_increment_Hz); myTympan.println(" Hz)");
  myTympan.print("   r/R: Raise/Lower freq compression (change by "); myTympan.print(freq_cr_increment); myTympan.println("x)");
  myTympan.print("   f/F: Raise/Lower freq shifting (change by "); myTympan.print(freq_shift_increment_Hz); myTympan.println(" Hz)");
}


//switch yard to determine the desired action
bool SerialManager::processCharacter(char c) {
  //float old_val = 0.0, new_val = 0.0;
  bool return_val = true; //assume we'll use the given character
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementKnobGain(channelGainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-channelGainIncrement_dB);  break;
    case 'C': case 'c':
      myTympan.println("Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU(); break;
    case 'p':
      switchToPCBMics(); break;
    case 'm':
      switchToMicInOnMicJack(); break;
    case 'l':
      switchToLineInOnMicJack();break;
    case 't':
      { float new_val = incrementFreqKnee(freq_knee_increment_Hz);
      myTympan.print("Recieved: new freq knee (Hz) = "); myTympan.println(new_val);}
      break;
    case 'T':
      { float new_val = incrementFreqKnee(-freq_knee_increment_Hz);
      myTympan.print("Recieved: new freq knee (Hz) = "); myTympan.println(new_val);}
      break;
    case 'r':
      { float new_val = incrementFreqCR(freq_cr_increment);
      myTympan.print("Recieved: new freq CompRatio = "); myTympan.println(new_val);}
      break;
    case 'R':
      { float new_val = incrementFreqCR(1.0/freq_cr_increment);
      myTympan.print("Recieved: new freq CompRatio = "); myTympan.println(new_val);}
      break;    
    case 'f':
      { float new_val = incrementFreqShift(freq_shift_increment_Hz);
      myTympan.print("Recieved: new freq shift (Hz) = "); myTympan.println(new_val);}
      break;
    case 'F':
      { float new_val = incrementFreqShift(-freq_shift_increment_Hz);
      myTympan.print("Recieved: new freq shift (Hz) = "); myTympan.println(new_val);}
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
}


#endif
