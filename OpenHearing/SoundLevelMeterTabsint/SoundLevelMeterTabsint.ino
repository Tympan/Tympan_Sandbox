/*
*   SoundLevelMeter
*
*   Created: Chip Audette, OpenAudio, June 2018 (Updated June 2021 for BLE and App)
*   Purpose: Compute the current sound level, dBA-Fast or whatever
*            Uses exponential time weighting.
*
*   March 2024: Extended to show loudness in octave bands
*
*   Uses Tympan RevC, RevD, or RevE.
*   Uses BLE and TympanRemote App
*
*   MIT License.  use at your own risk.
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "SerialManager.h"
//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

#define N_FILTER_BANDS 9   //throw-away the top and bottom (which are shelving filters), it leaves 7 proper octave-band filters in the middle
#define FILTER_ORDER 6     //what filter order to use for each of the filters in the filterbank

//create audio library objects for handling the audio
Tympan    myTympan(TympanRev::E);            //use TympanRev::E or TympanRev::D or TympanRev::C
#include "AudioConnections.h"

//Create BLE and serialManager
BLE ble(&myTympan); //&Serial1 is the serial connected to the Bluetooth module
SerialManager serialManager(&ble);
State myState(&audio_settings, &myTympan);

//calibraiton information for the microphone being used
float32_t mic_cal_dBFS_at94dBSPL_at_0dB_gain = -47.4f + 9.2175;  //PCB Mic baseline with manually tested adjustment.   Baseline:  http://openaudio.blogspot.com/search/label/Microphone

//control display and serial interaction
bool enable_printCPUandMemory = false;
bool enablePrintMemoryAndCPU(bool _enable = false) { return enable_printCPUandMemory = _enable; }
bool enablePrintingToBLE(bool _enable = false) {return myState.enable_printTextToBLE = _enable; };

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 15.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1000);
  myTympan.println("SoundLevelMeter: Starting setup()...");

  //make the remaining audio connections
  makePerBandAudioConnections();  //see AudioConnections.h

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(50,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC
 
  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  // Configure the octave-band level monitoring, set the processing parametes
  configOctaveBandProcessing(N_FILTER_BANDS,FILTER_ORDER,State::TIME_SLOW);  //see AudioConnections.h

  //Set frequency weighting for broadband processing.  See #define in: https://github.com/Tympan/Tympan_Library/blob/master/src/utility/FreqWeighting_IEC1672.h
  myTympan.println("Frequency Weighting: A_WEIGHT");
  setFreqWeightType(State::FREQ_A_WEIGHT);

  // Set Time-weighting for all level measuring objects.  See #define in https://github.com/Tympan/Tympan_Library/blob/master/src/AudioFilterTimeWeighting_F32.h
  myTympan.println("Time Weighting: SLOW");
  setTimeAveragingType(State::TIME_SLOW);

  

  // //////////// setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE(myTympan.getBTFirmwareRev());

  myTympan.println("Setup complete.");
  serialManager.printHelp();
  
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());
  
  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
  
  //printing of sound level
  if (myState.enable_printTextToBLE) {
    printLoudnessLevels(millis(),1000);  //print a value every 1000 msec
  }

  //check to see whether to print the CPU and Memory Usage
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

} //end loop();

// ///////////////// Supporting routines


//This routine prints the current SPL and the maximum since the last printing
void printLoudnessLevels(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static bool firstTime = true;
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (firstTime) {
      myTympan.println("Printing: current SPL (dB), max SPL (dB)");
      firstTime = false;
    }
    
    float32_t cal_factor_dB = -mic_cal_dBFS_at94dBSPL_at_0dB_gain + 94.0f - input_gain_dB;
    float32_t cur_SPL_dB = calcLevel1.getCurrentLevel_dB() + cal_factor_dB;
    float32_t max_SPL_dB = calcLevel1.getMaxLevel_dB() + cal_factor_dB;
    String msg = "TL=" + String(cur_SPL_dB,2) + " " + String(max_SPL_dB,2);
    
    msg.append(String("TO="));
    for (int i=1; i < (N_FILTER_BANDS-1);i++) {  //skip the first and last filter because they're shelving and not octave-band
      float32_t cur_SPL_dB = calcLevelPerOct[i].getCurrentLevel_dB() + cal_factor_dB;
      msg.append(String(cur_SPL_dB,2));
      if (i < (N_FILTER_BANDS-1-1)) msg.append(", ");
    }
    
    myTympan.println(msg); // print to USB
    ble.sendMessage(msg); // This is the line required for Tabsint

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}


//Set the frequency weighting.  Options include State::FREQ_A_WEIGHT and State::FREQ_C_WEIGHT
int setFreqWeightType(int type) {
  switch (type) {
    case State::FREQ_A_WEIGHT:
      freqWeight1.setWeightingType(A_WEIGHT);
      break;
    case State::FREQ_C_WEIGHT:
      freqWeight1.setWeightingType(C_WEIGHT);
      break;
    default:
      type = State::FREQ_A_WEIGHT;
      freqWeight1.setWeightingType(A_WEIGHT);
      break;
  }
  return myState.cur_freq_weight = type;
}

//Set the time averaging.  Options include State::TIME_SLOW and State::TIME_FAST
int setTimeAveragingType(int type) {
  switch (type) {
    case State::TIME_SLOW:
      calcLevel1.setTimeConst_sec(TIME_CONST_SLOW);
      for (int i=0; i < N_FILTER_BANDS; i++) calcLevelPerOct[i].setTimeConst_sec(TIME_CONST_SLOW);
      break;
    case State::TIME_FAST:
      calcLevel1.setTimeConst_sec(TIME_CONST_FAST);
      for (int i=0; i < N_FILTER_BANDS; i++) calcLevelPerOct[i].setTimeConst_sec(TIME_CONST_FAST);
      break;
    default:
      type = State::TIME_SLOW;
      calcLevel1.setTimeConst_sec(TIME_CONST_SLOW);
      for (int i=0; i < N_FILTER_BANDS; i++) calcLevelPerOct[i].setTimeConst_sec(TIME_CONST_SLOW);
      break;
  }
  return myState.cur_time_averaging = type;
}
