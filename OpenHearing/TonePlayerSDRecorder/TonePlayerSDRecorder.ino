/*
   TonePlayerSDRecorder
   
   Created: Chip Audette, OpenHearing, Feb 2024
   Purpose: Enable testing of output of Tympan
      * Uses Tympan (no earpiece shield is necessary)
      * Generate steady tones
      * Vary frequency and loudness via serial commands
      * Record pink jack to SD card (stereo)
      * Access SD card from PC via MTP

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E/F, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 96000.0f ;  //Desired sample rate
const int audio_block_samples = 128;     //Number of samples per audio block (do not make bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects

// define classes to control the Tympan and the Earpiece shield
Tympan                     myTympan(TympanRev::D);      //do TympanRev::D or TympanRev::E

//create audio library objects for handling the audio
AudioInputI2S_F32          i2s_in(audio_settings);         //Bring audio in
AudioSynthWaveform_F32     sineTone(audio_settings);       //from the Tympan_Library (synth_waveform_f32.h)
AudioMixer4_F32            audioMixer(audio_settings);     //from the Tympan_Library
AudioSDWriter_F32_UI       audioSDWriter(audio_settings);  //Write audio to the SD card (if activated)
AudioOutputI2S_F32         i2s_out(audio_settings);        //Send audio out

//Connect inputs to outputs, if used
//AudioConnection_F32           patchcord1(i2s_in, 1, i2s_out, 0);    //Left input to left output
//AudioConnection_F32           patchcord2(i2s_in, 0, i2s_out, 1);    //Right input to right output

//Connect the synthetic signals together into an audio mixer.  Right now, there's only one, so this isn't really necessary
AudioConnection_F32           patchcord5(sineTone, 0, audioMixer, 0);  //connect to first channel of the mixer

//Connect the sounds to the black headphone jack (both left and right output)
AudioConnection_F32           patchcord11(audioMixer, 0, i2s_out, 0);   //left output
AudioConnection_F32           patchcord12(audioMixer, 0, i2s_out, 1);   //right output

//Connect the audio input to the SD recording
AudioConnection_F32           patchcord21(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord22(sineTone, 0, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer
//AudioConnection_F32         patchcord21(i2s_in, 1, audioSDWriter, 1);   //connect Raw audio to left channel of SD writer


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
#include      "SerialManager.h"
#include      "State.h"                
BLE_UI&       ble = myTympan.getBLE_UI();  //myTympan owns the ble object, but we have a reference to it here
SerialManager serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

// Create the MTP servicing stuff so that one can access the SD card via USB
// If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental)
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated

// Add functions to enable more setup or hardware configuration changes
#include "setInputConfig.h"  // let's hide it in this file...the code is messy looking!  Ick!

//connect some software modules to the serial manager for automatic GUI generation for the phone
void setupSerialManager(void) {
  //register all the UI elements here
  //serialManager.add_UI_element(&myState);
  //serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
}


// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1500);
  Serial.println("TonePlayerSDRecorder: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable();

  //Select the input that we will use 
  setInputConfiguration(myState.input_source);  //the initial value is set in State.h
  
  //Select the gain for the output amplifiers
  setDacOutputGain(myState.output_dacGain_dB);

  //initialize the tone
  audioMixer.mute();  //mute everything (will be set correctly by activateTone())
  setToneLoudness(myState.tone_dBFS);
  //setToneFrequency(myState.tone_Hz);
  incrementToneFrequency(0);  //set to current index
  activateTone(myState.is_tone_active);

  //setup BLE
  //delay(250); myTympan.setupBLE();  delay(250); //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  setupSerialManager();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(2);             //four channels for this quad recorder, but you could set it to 2
  Serial.println("SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  // //respond to BLE
  // if (ble.available() > 0) {
  //   String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
  //   for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  // }

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
 
  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!

    //service the BLE advertising state
    if (audioSDWriter.getState() != AudioSDWriter::STATE::RECORDING) {
      //ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
    }

    //service the LEDs...blink slow normally, blink fast if recording
    myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 
  
  }
  
  //periodically print the CPU and Memory Usage, if requested
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

} //end loop()

// //////////////////////////////////// Control the audio from the SerialManager

//here's a function to change the input gain
float setInputGain_dB(float val_dB) {
  myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
  //earpieceShield.setInputGain_dB(myState.input_gain_dB);
  return myState.input_gain_dB;
}

//here's a function to increment the input gain.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}

//set the loudness of the tone
float setToneLoudness(float amp_dB) {
  myState.tone_dBFS = min(amp_dB, 0.0);  //limit maximum volume to 0.0 dBFS to prevent clipping
  sineTone.amplitude(sqrtf(powf(10.0f, 0.1f*myState.tone_dBFS)));
  return myState.tone_dBFS; 
}

//increment the loudness of the tone
float incrementToneLoudness(float incr_dB) {
  return setToneLoudness(myState.tone_dBFS+incr_dB);
}

//set the frequency of the tone
float setToneFrequency(float targ_freq_Hz) {
  static const float min_freq_Hz = 125.0/2.0/2.0/2.0; //t
  static const float max_freq_Hz = min(32000.0,sample_rate_Hz/2.0);
  myState.tone_Hz = max(min_freq_Hz,min(max_freq_Hz,targ_freq_Hz));
  sineTone.frequency(myState.tone_Hz);
  return myState.tone_Hz;
}

//increment the frequency of the tone
//float incrementToneFrequency(float incr_fac) {
//  return setToneFrequency(myState.tone_Hz*incr_fac);
//}
float incrementToneFrequency(int ind) {
  int tone_table_ind = myState.cur_tone_table_ind+ind;
  tone_table_ind =  min(myState.n_tone_table-1, max(0, tone_table_ind));
  myState.cur_tone_table_ind = tone_table_ind;
  return setToneFrequency(myState.tone_table_Hz[myState.cur_tone_table_ind]);
}

// helper function
float32_t overallTonePlusDacLoudness(void) { return myState.tone_dBFS + myState.output_dacGain_dB; }

//set the amplifier gain
float setDacOutputGain(float gain_dB) {
  float new_gain = min(6.0, myState.tone_dBFS + gain_dB) - myState.tone_dBFS; //limit the gain to prevent excessive clipping (will allow some clipping...starts at -3dBFS)
  return myState.output_dacGain_dB = myTympan.volume_dB(new_gain); //yes, this sets the DAC gain, not the headphone volume
}

//increment the amplifier gain
float incrementDacOutputGain(float incr_dB) {
  return setDacOutputGain(myState.output_dacGain_dB + incr_dB);
}

//mute the tone
bool activateTone(bool enable) {
  if (enable) {
    audioMixer.gain(0,1.0); //set gain in mixer to 1.0.  Tone should be connected to channel 0
  } else {
    audioMixer.gain(0,0.0); //get gain in mixer to zeor.  Tone should be connected to channel 0
  }
  return enable;
}

#include <math.h>
float testSineAccuracy(float32_t freq_Hz) {
  //float32_t freq_Hz = 1000.0;
  int nsamp = int(1.0*sample_rate_Hz);

  //allocate memory and the set the constants
  double two_pi_d = 2.0d*acos(0.0d);
  double highResPhase = 0.0d;
  double highRes_dPhase = two_pi_d * double(freq_Hz)/double(sample_rate_Hz);
  double hiRes;
  float32_t two_pi_f = 2.0f*acosf(0.0f);
  float32_t lowResPhase = 0.0f;
  float32_t lowRes_dPhase = two_pi_f * (float32_t)freq_Hz /float32_t(sample_rate_Hz);
  float32_t lowRes;

  //Serial.println("testSineAccuracy: two_pi = " + String(two_pi_f,6) + ", " + String(two_pi_d,6));

  //loop through time and compute everything
  double sum_diff2 = 0.0d;
  for (int i=0; i < nsamp; i++) {
    //compute high res (double precision)
    hiRes = sin(highResPhase);  //should use double-precision
    highResPhase += highRes_dPhase; 
    if (highResPhase > two_pi_d) highResPhase -= two_pi_d;

    //compute lower res (single precision)
    if (0) {
      lowRes = sinf(lowResPhase); 
    } else {
      //use single-precision, as does the ARM math library, as used by Tympan_Library
      lowRes = arm_sin_f32(lowResPhase);
    }
    lowResPhase += lowRes_dPhase; 
    if (lowResPhase > two_pi_f) lowResPhase -= two_pi_f;

    //compute the average difference between samples
    double diff = hiRes - double(lowRes);
    sum_diff2 += (diff * diff);
  }


  for (int i=0; i<nsamp; i++) {

  }
  double rms_diff = sum_diff2 / double(nsamp);
  return rms_diff;
}
