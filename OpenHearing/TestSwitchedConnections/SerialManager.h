/* 
 *  SerialManager
 *  
 *  Created: Chip Audette, OpenAudio, April 2017
 *  
 *  Purpose: This class receives commands coming in over the serial link.
 *     It allows the user to control the Tympan via text commands, such 
 *     as changing the volume or changing the filter cutoff.
 *  
 */

#ifndef _SerialManager_h
#define _SerialManager_h

//variables in the main sketch that I want to call from here
extern Tympan myTympan;
extern std::vector<AudioPath_Base *> allAudioPaths;

//functions in the main sketch that I want to call from here
extern void togglePrintMemoryAndCPU(void);
extern void deactivateAllAudioPaths(void);
extern int activateOneAudioPath(int index);

//now, define the Serial Manager class
class SerialManager {
public:
  SerialManager(void){};

  void respondToByte(char c);
  void printHelp(void);
private:
};

void SerialManager::printHelp(void) {
  Serial.println();
  Serial.println("SerialManager Help: Available Commands:");
  Serial.println("   h: Print this help");
  Serial.println("   C: Toggle printing of CPU and Memory usage");
  Serial.println("   0: De-activate all audio paths");
  for (int i=0; i < (int)allAudioPaths.size(); i++) {
    Serial.print("   ");  Serial.print(i+1); 
    Serial.print(": Activate only Audio Path "); Serial.print(i+1);
    Serial.print(" (" + allAudioPaths[i]->name + ")");
    Serial.println();
  }
  Serial.println();
}

//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (c) {
    case 'h':
    case '?':
      printHelp();
      break;
    case 'C':
    case 'c':
      Serial.println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU();
      break;
    case '0':
      Serial.println("SerialManager: Received 0: de-activating all audio paths...");
      deactivateAllAudioPaths(); //de-activate them all
      break;
    case '1':
      Serial.println("SerialManager: Received 1: switch to AudioPath_Sine...");
      activateOneAudioPath(1-1);  //de-activate all and then activate just the first audio path
      break;
    case '2':
      Serial.println("SerialManager: Received 2: switch to AudioPath_PassThruGain...");
      activateOneAudioPath(2-1);  //de-activate all and then activate just the second audio path
      break;
  }
}


#endif
