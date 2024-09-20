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

  //auto-populate entries for the audio paths that have already been created
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
    default:
      {
        //look to see if we were commanded to swtich audio paths (via "1" - "9")
        int possible_audio_path_index = (int)(c - '1');
        if ((possible_audio_path_index >= 0) && (possible_audio_path_index < (int)allAudioPaths.size())) {
          Serial.print("SerialManager: Received "); Serial.print(c);
          Serial.println(": switching to " + allAudioPaths[possible_audio_path_index]->name + "...");
          activateOneAudioPath(possible_audio_path_index);  //de-activate all and then activate just the second audio path
          break;  
        }
      }
  }
}


#endif
