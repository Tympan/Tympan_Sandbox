/*
  Play tone with adjustable frequency

  Chip Audette, OpenAudio, Nov 2021

  MIT License, Use at your own risk.
*/

#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 96000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Create the audio library objects that we'll use
Tympan                    myTympan(TympanRev::E, audio_settings);   //use TympanRev::D or TympanRev::E
AudioInputI2S_F32         audioInput(audio_settings);    //from the Tympan_Library
AudioFilterBiquad_F32     filter_L(audio_settings), filter_R(audio_settings); //from the Tympan Library
AudioCalcEnvelope_F32     measureEnvelope_L(audio_settings); //from the Tympan_Library
AudioCalcEnvelope_F32     measureEnvelope_R(audio_settings); //from the Tympan_Library
AudioSynthWaveform_F32    sineWave(audio_settings);     //from the Tympan_Library
AudioOutputI2S_F32        audioOutput(audio_settings);  //from the Tympan_Library
AudioSDWriter_F32_UI      audioSDWriter(audio_settings); //from the Tympan_Library

// Create the audio connections from the sineWave object to the audio output object
AudioConnection_F32 patchCord1(audioInput, 0, filter_L, 0);
AudioConnection_F32 patchCord2(audioInput, 1, filter_R, 0);
AudioConnection_F32 patchCord5(filter_L, 0, measureEnvelope_L, 0);
AudioConnection_F32 patchCord6(filter_R, 0, measureEnvelope_R, 0);
AudioConnection_F32 patchCord11(sineWave, 0, audioOutput, 0);  //connect to left output
AudioConnection_F32 patchCord12(sineWave, 0, audioOutput, 1);  //connect to right output
AudioConnection_F32 patchCord21(audioInput, 0, audioSDWriter, 0);
AudioConnection_F32 patchCord22(audioInput, 1, audioSDWriter, 1);

// /////////// Define the two operating states: MANUAL and AUTOMATIC
#define STATE_OFF 0
#define STATE_MANUAL 1
#define STATE_AUTOMATIC 2
int current_state = STATE_OFF;

// Define the parameters of the test tone
float tone_freq_Hz = 1000.0;   //frequency of the tone
float tone_amp_dB = -6.0;      //for some reason, it distorts above -6.0 dB

// Create classes to execute the calibration
#include "CalibrationClasses.h"
float start_Hz = 1000.0, end_Hz = 48000.0;
int n_steps = (int)(((end_Hz - start_Hz) / 1000.0)+0.5) + 1;
float step_dur_sec = 1.0;
CalibrationSteppedSine calSteppedSine(&sineWave, &filter_L, &filter_R, &measureEnvelope_L, &measureEnvelope_R);

// Define other parameters
float input_gain_dB = 15.0;

// /////////// Create classes for controlling the system, espcially via USB Serial
#include      "SerialManager.h"
SerialManager serialManager;     //create the serial manager for real-time control (via USB)

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&audioSDWriter);
}


// define setup()...this is run once when the hardware starts up
void setup(void)
{ 
  //set the sine wave parameters
  sineWave.amplitude(0.0); //silence
  setFrequency(tone_freq_Hz);
    
  //Open serial link for debugging
  myTympan.beginBothSerial(); delay(1000); //both Serial (USB) and Serial1 (BT) are started here
  myTympan.println("PlayTone: starting setup()...");

  //allocate the audio memory
  AudioMemory_F32(20,audio_settings); //I can only seem to allocate 400 blocks
  
  //start the audio hardware
  myTympan.enable();

   //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 1000.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble


  //choose input
  //myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  myTympan.inputSelect(TYMPAN_LEFT_JACK_AS_LINEIN_RIGHT_ON_BOARD_MIC); //the left input is from the pink jack, the right input is the right PCB mic
  myTympan.setInputGain_dB(input_gain_dB);
  Serial.println("setup(): Analog input gain set to " + String(input_gain_dB) + " dB for both left and right.");

  //Set the baseline volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  //setup the envelope measurement
  measureEnvelope_L.setAttackRelease_msec(100.0,100.0);
  measureEnvelope_R.setAttackRelease_msec(100.0,100.0);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);         //the library will print any error info to this serial stream (note that myTympan is also a serial stream)
  audioSDWriter.setNumWriteChannels(2);       //this is also the built-in defaullt, but you could change it to 4 (maybe?), if you wanted 4 channels.
  Serial.println("Setup: SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //setup the serial manager
  setupSerialManager();

  //switch state to the specified startup state
  switchState(current_state);

  myTympan.println("Setup complete.");
  serialManager.printHelp();
}

void loop(void)
{
  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(audioInput); //For the warnings, it asks the audioInput class for some info

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //act based on current state
  switch (current_state) {
    case STATE_MANUAL:
      update_manual_operation();
      break;
    case STATE_AUTOMATIC:
      update_automatic_operation();
      break;
  }
}

// //////////////////////// Updating routines
void update_manual_operation(void) {
  unsigned long cur_millis = millis();
  static unsigned long prev_update_millis = 0;
  const unsigned long update_period_millis = 1000.0;
  if (cur_millis < prev_update_millis) prev_update_millis = 0;  //wrapped around!
  if ( (cur_millis - prev_update_millis) >= update_period_millis ) { //has enough time passed?

    //yes, enough time has passed.  Do the work!
    float left_cal_dB_SPL = 94.0 - (-57.65) - input_gain_dB - (101.3 - 94);
    float right_cal_dB_SPL = 94.0 - (-57.65) - input_gain_dB - 4.0;
    float level_L_dB = 20.0*log10f(measureEnvelope_L.getCurrentLevel());
    float level_R_dB = 20.0*log10f(measureEnvelope_R.getCurrentLevel());
    Serial.println(String("Current Level: ")
      + String(", Raw (L/R): ") + String(level_L_dB) + String(", ") + String(level_R_dB) + String(" dBFS")
      + String(", Scaled (L/R) = ") + String(level_L_dB + left_cal_dB_SPL) + String(" dB SPL")
      + String(", = ") + String(level_R_dB + right_cal_dB_SPL) + String(" dB SPL"));

    prev_update_millis = cur_millis;
  } else {
    //do nothing
  }
}

void update_automatic_operation(void) {
  int ret_val = 0;
  ret_val = calSteppedSine.update();
  if (ret_val == 1) {
    Serial.println("Stepped Sine Complete.");
    switchState(STATE_OFF);
  }
}

// ///////////////////////////////// Interactive routines

float setFrequency(float freq_Hz) {
  tone_freq_Hz = freq_Hz;

  filter_L.setBandpass(0,freq_Hz);
  filter_R.setBandpass(0,freq_Hz);
  sineWave.frequency(tone_freq_Hz); //set the frequency
  
  return tone_freq_Hz;
}

float incrementFrequency(float incr_Hz) {
  float freq_Hz = max(1000.0,min(sample_rate_Hz/2.0, tone_freq_Hz+incr_Hz));
  return setFrequency(freq_Hz);
}

float incrementAmplitude(float incr_dB) {
  tone_amp_dB = max(-60.0, min(-3.0, tone_amp_dB + incr_dB));
  float tone_rms = sqrtf(powf(10.0,tone_amp_dB/10.0));
  float tone_amp = tone_rms * sqrt(2.0);
  //Serial.println("incrementAmplitude: setting amplitude to " + String(tone_amp));
  sineWave.amplitude(tone_amp);
  return tone_amp_dB;
}

bool muteTone(bool set_mute) {
  if (set_mute) {
    sineWave.amplitude(0.0);
  } else {
    incrementAmplitude(0.0);
  }

  return set_mute;
}

int switchState(int new_state) {

  //turn off previous state
  if (new_state != current_state) {
    switch (current_state) {
      case STATE_OFF:
        //nothing to turn off
        break;
      case STATE_MANUAL:
        //turn off tone
        Serial.println("switchState: stopping manual tone...");
        muteTone(true);
        break;
      case STATE_AUTOMATIC:
        //stop test
        Serial.println("switchState: stopping stepped sine test...");
        calSteppedSine.end();
        calSteppedSine.printAllResults();
        muteTone(true);
    }
 
    //turn on current state
    switch (new_state) {
      case STATE_OFF:
        //nothing to do
        break;
      case STATE_MANUAL:
        //turn on tone
        Serial.println("Staring manual test tone...");
        muteTone(false);
        break;
      case STATE_AUTOMATIC:
        //turn on stepped-sine test
        Serial.println("Starting Stepped Sine Calibration...");
        calSteppedSine.start(start_Hz, end_Hz, n_steps, step_dur_sec);  
        break;
    }
  }
  
  current_state = new_state;
  return current_state;
}
