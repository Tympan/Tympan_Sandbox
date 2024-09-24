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
#include "AudioPath_Base.h"
#include "SerialManager.h"

//set the sample rate and block size
const float sample_rate_Hz    = 96000.0f ; //choose from 8kHz to 96kHz
const int audio_block_samples = 128;        //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

#define USE_FOUR_CHANNELS (false)

//create audio library objects for handling the audio
Tympan                     myTympan(TympanRev::E, audio_settings);   //do TympanRev::D or E or F
#if USE_FOUR_CHANNELS
  EarpieceShield           earpieceShield(TympanRev::E, AICShieldRev::A);  //Note that EarpieceShield is defined in the Tympan_Libarary in AICShield.h 
  AudioInputI2SQuad_F32    *audioInput;            //create it later
  AudioOutputI2SQuad_F32   *audioOutput;           //create it later
#else
  AudioInputI2S_F32        *audioInput;            //create it later
  AudioOutputI2S_F32       *audioOutput;           //create it later
#endif

//create other audio-related items
std::vector<AudioMixer8_F32 *> allOutputMixers;         //fill it later
std::vector<AudioPath_Base *> allAudioPaths;            //fill it later
std::vector<AudioConnection_F32 *> patchCords;     //fill it later
int activeAudioPathIndex = -1;  //which audio path is active (counting from zero)

//create items to help AudioPaths control the hardware
Tympan *tympan_ptr = &myTympan;
#if USE_FOUR_CHANNELS
  EarpieceShield *shield_ptr = &earpieceShield;
#else
  EarpieceShield *shield_ptr = NULL;
#endif


// ////////////////////////////////////////////////////////////////////////////////////////////////////
// 
/// /////////// DEAR USER!!!!  Add or remove your AudioPath objects below!!!  /////////////////////////
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////

// Include your header files
#include "AudioPath_Sine.h"
#include "AudioPath_PassThruGain_Analog.h"
#include "AudioPath_PassThruGain_PDM.h"
#include "AudioPath_Sine_wFFT.h"

// Create your AudioPath
void createMyAudioPathObjects(AudioSettings_F32 &_audio_settings) {
  
  //Add Audio Path: Sine wave generator
  allAudioPaths.push_back( new AudioPath_Sine(audio_settings, tympan_ptr, shield_ptr) );  

  //Add Audio Path: Audio pass-thru with gain (analog)
  allAudioPaths.push_back( new AudioPath_PassThruGain_Analog(audio_settings, tympan_ptr, shield_ptr) );

  //Add Audio Path: Audio pass-thru with gain (PDM mics)
  allAudioPaths.push_back( new AudioPath_PassThruGain_PDM(audio_settings, tympan_ptr, shield_ptr) );

  //Add Audio Path: Sine wave generator with FFT analysis of the input
  allAudioPaths.push_back( new AudioPath_Sine_wFFT(audio_settings, tympan_ptr, shield_ptr) );

  //Add Audio Path: Add yours here!
  //allAudioPaths.push_back( new myAudioPathClassName(audio_settings, tympan_ptr, shield_ptr) ); 
}


// ////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// The rest of the code should not care which (or how many) AudioPath objects you are using.
// You should not have to touch any of this code when you add/remove your AudioPath objects.
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////

//Instantiate all of the Audio and AudioPath objects
void createAllAudioObjects(AudioSettings_F32 &_audio_settings) {
  //Start with instatiating the input
  #if USE_FOUR_CHANNELS
    audioInput = new AudioInputI2SQuad_F32(audio_settings);
  #else
    audioInput = new AudioInputI2S_F32(audio_settings);
  #endif

  //Next, let the user instantiate all of the AudioPaths that they might ever decide to use
  createMyAudioPathObjects(_audio_settings);

  //Next, create the audio output mixers...one for Left0, Right0, Left1, Right1
  for (int i = 0; i < 4; i++) allOutputMixers.push_back(new AudioMixer8_F32(audio_settings) );  //...only mixes up to 8 outputs! (defaults to all gains = 1.0)
  
  //Finally, create the audio outputs
  #if USE_FOUR_CHANNELS
    audioOutput = new AudioOutputI2SQuad_F32(audio_settings);
  #else
    audioOutput = new AudioOutputI2S_F32(audio_settings);
  #endif
}

void connectAllAudioObjects(void) {
  //Connect each AudioPath to the inputs
  for (int Ipath=0; Ipath < (int)allAudioPaths.size(); Ipath++) {  //loop over each AudioPath
    #if USE_FOUR_CHANNELS
      patchCords.push_back( new AudioConnection_F32(*audioInput, EarpieceShield::PDM_LEFT_FRONT,  *allAudioPaths[Ipath]->getStartNode(), 0) );
      patchCords.push_back( new AudioConnection_F32(*audioInput, EarpieceShield::PDM_LEFT_REAR,   *allAudioPaths[Ipath]->getStartNode(), 1) );
      patchCords.push_back( new AudioConnection_F32(*audioInput, EarpieceShield::PDM_RIGHT_FRONT, *allAudioPaths[Ipath]->getStartNode(), 2) );
      patchCords.push_back( new AudioConnection_F32(*audioInput, EarpieceShield::PDM_RIGHT_REAR,  *allAudioPaths[Ipath]->getStartNode(), 3) );
    #else
      patchCords.push_back( new AudioConnection_F32(*audioInput, 0,  *allAudioPaths[Ipath]->getStartNode(), 0) );
      patchCords.push_back( new AudioConnection_F32(*audioInput, 1,  *allAudioPaths[Ipath]->getStartNode(), 1) );
    #endif
  }

  //Connect each AudioPath to each of the output mixers
  for (int Imixer=0; Imixer < (int)allOutputMixers.size(); Imixer++) {  //loop over each output mixer
    for (int Ipath=0; Ipath < (int)allAudioPaths.size(); Ipath++) {  //loop over each AudioPath
      int path_output_channel = Imixer;  //make it clearer what we're doing
      int mixer_input_chanenl = Ipath;   //make it clearer what we're doing
      patchCords.push_back( new AudioConnection_F32(*allAudioPaths[Ipath]->getEndNode(), path_output_channel, *allOutputMixers[Imixer], mixer_input_chanenl) );
    }
  }
  
  //Connect the output mixers to the outputs
  #if USE_FOUR_CHANNELS
    patchCords.push_back( new AudioConnection_F32(*allOutputMixers[0], 0, *audioOutput, EarpieceShield::OUTPUT_LEFT_TYMPAN) );    //connect left0 mixer to output (needs actual references, not pointers)
    patchCords.push_back( new AudioConnection_F32(*allOutputMixers[1], 0, *audioOutput, EarpieceShield::OUTPUT_RIGHT_TYMPAN) );   //connect right0 mixer to output (needs actual references, not pointers)
    patchCords.push_back( new AudioConnection_F32(*allOutputMixers[2], 0, *audioOutput, EarpieceShield::OUTPUT_LEFT_EARPIECE) );  //connect left1 mixer to output (needs actual references, not pointers)
    patchCords.push_back( new AudioConnection_F32(*allOutputMixers[3], 0, *audioOutput, EarpieceShield::OUTPUT_RIGHT_EARPIECE) ); //connect right1 mixer to output (needs actual references, not pointers)
  #else
    patchCords.push_back( new AudioConnection_F32(*allOutputMixers[0], 0, *audioOutput, 0) ); //connect left0 mixer to output (needs actual references, not pointers)
    patchCords.push_back( new AudioConnection_F32(*allOutputMixers[1], 0, *audioOutput, 1) ); //connect right0 mixer to output (needs actual references, not pointers)
  #endif
}

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

AudioPath_Base *getActiveAudioPath(void) {
  if (activeAudioPathIndex >= 0) {
    return allAudioPaths[activeAudioPathIndex];
  }
  return NULL;
}

//Define other control, display and serial interactions
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager;

// ////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// Arduino setup() and loop() functions
//
// ////////////////////////////////////////////////////////////////////////////////////////////////////

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 5.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  while (!Serial && (millis() < 2000UL)) {
    delay(1); //stall;
  }
  Serial.println("TestSwitchedConnections: Starting setup()...");

  //Create all the audio objects and connections
  createAllAudioObjects(audio_settings);  //instantiates all of the audio objects and paths (in the best order for lowest latency)
  connectAllAudioObjects();               //connect all of the audio and audiopath objects
  deactivateAllAudioPaths();             //de-activate all the audiopaths (they're probably already de-activated from their own constructors, but this makes sure)

  //allocate the audio memory
  AudioMemory_F32(180,audio_settings); //allocate memory

  //configure the Tympan
  myTympan.enable();                               //Enable the Tympan to start the audio flowing!
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); //Choose the desired input and set volume levels (on-board mics)
  myTympan.setInputGain_dB(input_gain_dB);         //set input volume, 0-47.5dB in 0.5dB setps
  myTympan.volume_dB(0.0);                         //set volume of headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  #if USE_FOUR_CHANNELS
    //repeat for the earpiece shield
    earpieceShield.enable();                                 //Enable the Tympan to start the audio flowing!
    earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //The earpiece shield doesn't have on-board mics, so use pink jack as line-in
    earpieceShield.setInputGain_dB(input_gain_dB);           //set input volume, 0-47.5dB in 0.5dB setps
    earpieceShield.volume_dB(0);                             //set volume of headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  #endif

  //Activate just the audio path that we want to start with (this function first de-activates all audio paths)
  if (1) activateOneAudioPath(1-1);                //must be after the Tympan and Earpiece shield are enabled or else the setupHardware() method within won't work.

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


