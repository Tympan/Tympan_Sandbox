/* //////////////////////////////
 *  
 *  FreqDomainDemo_liveUSB_BLE
 *  
 *  Created: Chip Audette, OpenAudio, June 2021
 *  
 *  Purpose: Demonstrate formant shifting and frequency shifting.
 *     Uses Tympan RevE (easily switched to RevD)
 *     Uses the built-in microphones (easily switched an alternative microphone)
 *     Uses BLE and the TympanRemote App
 *     Streams output audio to USB for recording on a PC or for use in video conferencing
 *     
 *  MIT License.  Use at your own risk.
 *  
 */ /////////////////////////

#include <Audio.h>  //for the base Teensy Audio classes(ie, the non Tympan_Library audio classes)
#include <Tympan_Library.h>
#include "State.h"
#include "SerialManager.h"

Tympan              myTympan(TympanRev::E);     //do TympanRev::C or TympanRev::D or TympanRev::E
AudioSettings_F32   audio_settings;  //use the defaults...as required for using USB audio

float assumed_sample_rate_Hz = audio_settings.sample_rate_Hz;
int assumed_audio_block_samples = audio_settings.audio_block_samples;
float desired_mic_gain_dB = 25.0f; //gain on the PCB microphones
  
//create audio objects
AudioInputI2S                   i2s_in;          //Digital audio *from* the Tympan AIC.AudioOutputUSB           audioOutput; // must set Tools > USB Type to Audio
AudioConvert_I16toF32           convertI16toF32;
AudioSwitch4_F32                audioSwitch(audio_settings);
AudioEffectFormantShift_FD_F32  formantShift(audio_settings);    //create the frequency-domain processing block
AudioEffectFreqShift_FD_F32     freqShift(audio_settings);      //Freq domain processing!  https://github.com/Tympan/Tympan_Library/blob/master/src/AudioEffectFreqShiftFD_F32.h
AudioMixer4_F32                 audioMixer(audio_settings);
AudioConvert_F32toI16           convertF32toI16;
AudioOutputUSB                  audioOutput; // in the Arduino IDE, must set Tools > USB Type to Audio
AudioOutputI2S                  i2s_out;

//make audio connections
AudioConnection          patchCord01(i2s_in,          0, convertI16toF32, 0);  //left ("0") input only.  convert to F32...next audio connection must be a AudioConnection_F32
AudioConnection_F32      patchCord11(convertI16toF32, 0, formantShift,    0);  //connect to formant shifter
AudioConnection_F32      patchCord21(formantShift,    0, freqShift,       0);  //connect to frequency shifter
AudioConnection_F32      patchCord31(freqShift,       0, convertF32toI16, 0);  //convert to I16...next audio connection must be a plain AudioConnection
AudioConnection          patchCord41(convertF32toI16, 0, audioOutput,     0);  //left output ("0")
AudioConnection          patchCord42(convertF32toI16, 0, audioOutput,     1);  //right output ("1")
AudioConnection          patchCord51(convertF32toI16, 0, i2s_out,         0);  //left output ("0")
AudioConnection          patchCord52(convertF32toI16, 0, i2s_out,         1);  //right output ("1")

//Create BLE
BLE ble = BLE(&Serial1);

//control display and serial interaction
SerialManager serialManager(&ble);
State myState(&audio_settings, &myTympan);
String overall_name = String("Tympan: Frequency Domain Demonstration with live USB Audio");



void switchToPCBMics(float new_input_gain_dB) {
  Serial.println("Switching to PCB Mics.");
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the microphone jack - defaults to mic bias OFF
  myTympan.setInputGain_dB(new_input_gain_dB);
}

bool enableFormantShift(bool _enable) {
  return myState.is_enabled_formant = formantShift.enable(_enable);
}

bool enableFreqShift(bool _enable) {
  return myState.is_enabled_freq = freqShift.enable(_enable);
}

void connectClassesToOverallState(void) {
  //myState.earpieceMixer = &earpieceMixer.state;
}

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&myState);
  
  serialManager.createTympanRemoteLayout();
}


// //////////////////////////////////////////////////////////////////////////

void setup() {
  myTympan.beginBothSerial();
  myTympan.setEchoAllPrintToBT(false);  //if we're using BLE, don't echo the print statements over the BT link
  delay(1000);
  Serial.print(overall_name); Serial.println(": setup():...");
  Serial.print("  Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("  Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);

  //allocate Audio memory
  AudioMemory(20);  //for use by the base Teensy Audio library
  AudioMemory_F32(40,audio_settings);  //for use by the Tympan_Library
  
  // Configure the frequency-domain algorithm(s)
  int overlap_factor_formant = 4;  //set to 4 or 8 or either 75% overlap (4x) or 87.5% (8x)
  int N_FFT_formant = assumed_audio_block_samples * overlap_factor_formant;
  formantShift.setup(audio_settings, N_FFT_formant); //do after AudioMemory_F32();
  setFormantShift(myState.cur_scale_factor); //1.0 is no formant shifting.

  int overlap_factor_freq = 4;  //set to 4 or 8 or either 75% overlap (4x)
  int N_FFT_freq = assumed_audio_block_samples * overlap_factor_freq;
  freqShift.setup(audio_settings, N_FFT_freq); //do after AudioMemory_F32();
  freqShift.setShift_bins(myState.cur_shift_bins); //0 is no frquency shifting

  float formant_shift_gain_correction_dB = 0.0;
  if (overlap_factor_formant == 4) {
    formant_shift_gain_correction_dB = -3.0;
  } else if (overlap_factor_formant == 8) {
    formant_shift_gain_correction_dB = -9.0;
  }

  //choose the default algorithm
  enableFormantShift(true);
  enableFreqShift(false);
  
  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 100.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, cutoff_Hz, assumed_sample_rate_Hz); //set to false to disble

   //Choose the desired input
  switchToPCBMics(desired_mic_gain_dB + formant_shift_gain_correction_dB);        //use PCB mics as input
  //switchToMicInOnMicJack(desired_mic_gain_dB + formant_shift_gain_correction_dB); //use Mic jack as mic input (ie, with mic bias)
  //switchToLineInOnMicJack(desired_mic_gain_dB + formant_shift_gain_correction_dB);  //use Mic jack as line input (ie, no mic bias)

  //Set the desired volume levels
  myTympan.volume_dB(0); 

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  ble.setupBLE();

  //setup serial manager
  setupSerialManager();

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();
}


void loop() {
  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) respondToByte(msgFromBle[i]);
  }

  //If there is no BLE connection, make sure that we keep advertising
  ble.updateAdvertising(millis(),5000);  //check every 5000 msec

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(),3000); //print every 3000 msec
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(),3000);

  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec
}


//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, const unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    
    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0 / 10.0) * (float)((int)(10.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //use the potentiometer value to control something interesting
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      //set the amount of formant shifting
      float new_scale_fac = powf(2.0, (val - 0.5) * 2.0);
      Serial.print("servicePotentiometer: new scale factor = "); Serial.println(new_scale_fac);
      setFormantShift(new_scale_fac);
      serialManager.updateFormantScaleFacGUI(); //also update the GUI
    }

    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();



int incrementFreqShift(int incr_factor) {
  myState.cur_shift_bins = freqShift.setShift_bins(freqShift.getShift_bins() + incr_factor);
  return myState.cur_shift_bins;
}


float incrementFormantShift(float incr_factor) {
  return setFormantShift(formantShift.setScaleFactor(formantShift.getScaleFactor()*incr_factor));
}
float setFormantShift(float new_value) {
  myState.cur_scale_factor = formantShift.setScaleFactor(new_value); //set the scale factor and save the state
  return myState.cur_scale_factor; 
}
