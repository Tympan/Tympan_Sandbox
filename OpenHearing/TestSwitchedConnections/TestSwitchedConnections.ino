/*
*   TestSwtichedAudioInstances
*
*   Created: Chip Audette, OpenAudio, Sept 2024
*   Purpose: Dynamically switches audio between two different audio paths without creating
*      or destroying audio objects nor audio connections.
*
*   Use the serial monitor to command the switching.
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include <vector>
#include "AudioPaths.h"
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz    = 32000.0f ; //choose from 8kHz to 96kHz
const int audio_block_samples = 64;        //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                  myTympan(TympanRev::E, audio_settings);   //do TympanRev::D or E or F
AudioInputI2S_F32       *audioInput;                //create it later
AudioMixer8_F32         *audioMixerL, *audioMixerR; //create them later 
AudioOutputI2S_F32      *audioOutput;               //create it later
std::vector<AudioPath_Base *> allAudioPaths;        //fill it later
std::vector<AudioConnection_F32 *> otherPatchCords; //fill it later
int activeAudioPathIndex = -1;

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager;

//helper functions
void deactivateAllAudioPaths(void) {
  for (auto & audioPath : allAudioPaths) { audioPath->setActive(false); } //iterate over each audio path object and set whether it is active.
  activeAudioPathIndex = -1;  //none are active
}

int activateOneAudioPath(int index) {
  deactivateAllAudioPaths(); //de-activate them all
  if (index < (int)allAudioPaths.size()) {
    activeAudioPathIndex = index;
    allAudioPaths[activeAudioPathIndex]->setActive(true);  //activate just the one
  }
  return activeAudioPathIndex;
}

void createAndConnectAllAudio(AudioSettings_F32 &_audio_settings) {
  //Instantiate all of the Audio and AudioPath objects
  audioInput = new AudioInputI2S_F32(audio_settings);
  allAudioPaths.push_back(new AudioPath_Sine(audio_settings, audioInput));   //pass pointer to audioInput
  allAudioPaths.push_back(new AudioPath_PassThruGain(audio_settings, audioInput)); //pass pointer to audioInput

  
  //Create the output mixer and audio output mixers so that the objects are updated in the right 
  //order (ie, last is last) thereby minimizing audio latency
  audioMixerL = new AudioMixer8_F32(audio_settings);  //for left output ...only mixes up to 8 outputs! (defaults to all gains = 1.0)
  audioMixerR = new AudioMixer8_F32(audio_settings);  //for right output ...only mixes up to 8 outputs!  (defaults to all gains = 1.0
   for (int i=0; i < (int)allAudioPaths.size(); i++) {
    allAudioPaths[i]->connectToDestination(0, audioMixerL, i);  //connect left output (0) of audio path to the left mixer  (pass pointer to audiomixer)
    allAudioPaths[i]->connectToDestination(1, audioMixerR, i);  //connect right output (1) of audio path to the right mixer (pass pointer to audiomixer)
  }
  
  //Create the final output and connect to it
  audioOutput = new AudioOutputI2S_F32(audio_settings);
  otherPatchCords.push_back( new AudioConnection_F32(*audioMixerL, 0, *audioOutput, 0) ); //connect left mixer to left (0) output (needs actual references, not pointers)
  otherPatchCords.push_back( new AudioConnection_F32(*audioMixerR, 0, *audioOutput, 1) ); //connect right mixer to right (1) output (needs actual references, not pointers)

  //de-activate all audio paths (for now)
  deactivateAllAudioPaths(); //de-activate them all
}

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 5.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  Serial.println("TestSwitchedConnections: Starting setup()...");

  //Create all the audio objects and connections
  createAndConnectAllAudio(audio_settings);

  //Choose which audio path is active
  activateOneAudioPath(0);  //de-activate all and then activate just the first audio path

  //allocate the audio memory
  AudioMemory_F32(100,audio_settings); //allocate memory

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  //End of setup
  Serial.println("Setup complete.");
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands, if any have been received
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),500); //update every 3000 msec

  //service any main-loop needs of the active AudioPath object
  if ((activeAudioPathIndex >=0) && (activeAudioPathIndex < (int)allAudioPaths.size())) allAudioPaths[activeAudioPathIndex]->serviceMainLoop();

} //end loop();


