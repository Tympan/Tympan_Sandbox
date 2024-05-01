
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//externally defined objects
extern Tympan myTympan;
extern State myState;
extern AudioCalcLevel_F32 calcLevel1;
extern AudioFilterFreqWeighting_F32    freqWeight1;

//functions in the main sketch that I want to call from here
extern bool enablePrintMemoryAndCPU(bool);
extern bool enablePrintingToBLE(bool);
extern int setFreqWeightType(int);
extern int setTimeAveragingType(int);

//Define a class to help manage the interactions with Serial comms (from SerialMonitor or from Bluetooth (BLE))
//see SerialManagerBase.h in the Tympan Library for some helpful supporting functions (like {"sendButtonState")
//https://github.com/Tympan/Tympan_Library/tree/master/src
class SerialManager : public SerialManagerBase {  
  public:
    SerialManager(BLE *_ble) : SerialManagerBase(_ble) { myGUI = new TympanRemoteFormatter; };
    void respondToByte(char c);
    void printHelp(void);

    //define the GUI for the App
    TympanRemoteFormatter *myGUI;  //Creates the GUI-writing class for interacting with TympanRemote App
  private:

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println("   h:   Print this help");
  myTympan.println("   c,C: Enable/Disable printing of CPU and Memory usage");
  myTympan.println("   t,T: FAST time constant or SLOW time constant on all measurements");
  myTympan.println("   f,F: BROADBAND: A-weight or C-weight for loudness");  
  myTympan.println("   v,V: BROADBAND AND OCTAVE BAND: Start/Stop sending level to TabSINT App.");
  myTympan.println("   0:   BROADBAND: Reset max loudness value.");
  myTympan.println();
}


//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  //float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'c':
      myTympan.println("Command Received: enable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(true);
      break;
    case 'C':
      myTympan.println("Command Received: disable printing of memory and CPU usage.");
      enablePrintMemoryAndCPU(false);
      break;
    case 'f':
      myTympan.println("Command Received: setting to A-weight");
      setFreqWeightType(State::FREQ_A_WEIGHT);
      break;      
    case 'F':
      myTympan.println("Command Received: setting to C-weight");
      setFreqWeightType(State::FREQ_C_WEIGHT);
      break;
    case 't':
      myTympan.println("Command Received: setting to FAST time constant");
      setTimeAveragingType(State::TIME_FAST);
      break;      
    case 'T':
      myTympan.println("Command Received: setting to SLOW time constant");
      setTimeAveragingType(State::TIME_SLOW);
      break;      
    case 'v':
      myTympan.println("Command Received: enable printing of loudness levels to BLE.");
      enablePrintingToBLE(true);
      break;
    case 'V':
      myTympan.println("Command Received: disable printing of loudness levels to BLE.");
      enablePrintingToBLE(false);
      break;
    case '0':
      myTympan.println("Command Received: reseting max SPL.");
      calcLevel1.resetMaxLevel();
      break;         
  }
};

#endif
