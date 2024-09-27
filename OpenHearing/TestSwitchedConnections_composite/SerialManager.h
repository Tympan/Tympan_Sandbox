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
extern std::vector<AudioStreamComposite_F32 *> allAudioPaths;

//functions in the main sketch that I want to call from here
extern void togglePrintMemoryAndCPU(void);
extern void deactivateAllAudioPaths(void);
extern int activateOneAudioPath(int index);
AudioStreamComposite_F32 *getActiveAudioPath(void);

//now, define the Serial Manager class
class SerialManager {
public:
  SerialManager(void){};

  void respondToByte(char c);
  void printHelp(void);
  bool interpretAsSwitchAudioPath(char c);
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
    Serial.print(" (" + allAudioPaths[i]->instanceName + ")");
    Serial.println();
  }

  //add help from active audio path
  AudioStreamComposite_F32 *audio_path = getActiveAudioPath();
  if (audio_path != NULL) {
    Serial.println("  Commands for active Audiopath (" + audio_path->instanceName + "):");
    audio_path->printHelp();
  }
  Serial.println();
}

//try to interpret byte as a command ("1"-"9") to switch the audio path
bool SerialManager::interpretAsSwitchAudioPath(char c) {
  bool was_switch_command = false;
  int possible_audio_path_index = (int)(c - '1');
  if ((possible_audio_path_index >= 0) && (possible_audio_path_index < (int)allAudioPaths.size())) {
    Serial.print("SerialManager: Received "); Serial.print(c);
    Serial.println(": switching to " + allAudioPaths[possible_audio_path_index]->instanceName + "...");
    activateOneAudioPath(possible_audio_path_index);  //de-activate all and then activate just the second audio path
    was_switch_command = true;  
  }
  return was_switch_command;
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
        bool was_switch_command = interpretAsSwitchAudioPath(c);
        if (was_switch_command==false) {
          AudioStreamComposite_F32 *audio_path = getActiveAudioPath();
          if (audio_path != NULL) audio_path->respondToByte(c);
        }
      }
  }
}


#endif
