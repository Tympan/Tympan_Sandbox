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
AudioInputI2S_F32         audioInput(audio_settings); //from the Tympan_Library
AudioCalcEnvelope_F32     measureEnvelope(audio_settings); //from the Tympan_Library
AudioSynthWaveform_F32    sineWave(audio_settings);   //from the Tympan_Library
AudioOutputI2S_F32        audioOutput(audio_settings);  //from the Tympan_Library
AudioSDWriter_F32         audioSDWriter(audio_settings) //from the Tympan_Library

// Create the audio connections from the sineWave object to the audio output object
AudioConnection_F32 patchCord9(audioInput, 0, measureEnvelope, 0);
AudioConnection_F32 patchCord10(sineWave, 0, audioOutput, 0);  //connect to left output
AudioConnection_F32 patchCord11(sineWave, 0, audioOutput, 1);  //connect to right output


// Define the parameters of the tone
float tone_freq_Hz = 1000.0;   //frequency of the tone
float tone_amp_dB = -6.0;      //for some reason, it distorts above -6.0 dB

// Define other parameters
float input_gain_dB = 15.0;

// define setup()...this is run once when the hardware starts up
void setup(void)
{
  //set the sine wave parameters
  sineWave.frequency(tone_freq_Hz);
  sineWave.amplitude(sqrtf(2.0)*sqrtf(powf(10.f,tone_amp_dB/10.0)));
  
  //Open serial link for debugging
  myTympan.beginBothSerial(); delay(1000); //both Serial (USB) and Serial1 (BT) are started here
  myTympan.println("PlayTone: starting setup()...");

  //allocate the audio memory
  AudioMemory_F32(20,audio_settings); //I can only seem to allocate 400 blocks
  
  //start the audio hardware
  myTympan.enable();

   //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble


  //choose input
  //myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
  myTympan.inputSelect(TYMPAN_LEFT_JACK_AS_LINEIN_RIGHT_ON_BOARD_MIC); //the left input is from the pink jack, the right input is the right PCB mic
  myTympan.setInputGain_dB(input_gain_dB);

  //Set the baseline volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  //setup the envelope measurement
  measureEnvelope.setAttackRelease_msec(100.0,100.0);
 
  myTympan.println("Setup complete.");
  printHelp();
}

void loop(void)
{
  while (Serial.available()) respondToByte((char)Serial.read());
  delay(500);

  float slm_cal_dB_SPL = 94.0 - (-57.65) - input_gain_dB - (101.3 - 94);
  float pcb_cal_dB_SPL = 94.0 - (-57.65) - input_gain_dB - 4.0;
  float level_dB = 20.0*log10f(measureEnvelope.getCurrentLevel());
  Serial.println(String("Current Level: ")
    + String(", Left Raw: ") + String(level_dB) + String(" dBFS")
    + String(", SPM = ") + String(level_dB + slm_cal_dB_SPL) + String(" dB SPL")
    + String(", PCB = ") + String(level_dB + pcb_cal_dB_SPL) + String(" dB SPL"));
}

void printHelp(void) {
  Serial.println("PlayTone: Help menu:");
  Serial.println("  h: print this help");
  Serial.println(" w/W: Switch between PCB Mics (w) and Line-in on Mic Jack (W)");
  Serial.println("  f/F: incr/decrease the frequency (currently: " + String(tone_freq_Hz/1000,1) + " kHz)");
  Serial.println("  a/A: incr/decrease the amplitude (currently: " + String(tone_amp_dB,1) + " dB)");
  Serial.println("  m/M: mute/unmute the tone");
}

float incrementFrequency(float incr_Hz) {
  tone_freq_Hz = max(1000.0,min(sample_rate_Hz/2.0, tone_freq_Hz+incr_Hz));
  sineWave.frequency(tone_freq_Hz);
  return tone_freq_Hz;
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

void respondToByte(char c) {
  float new_val;
  
  if ((c==' ') || (c=='\n') || (c=='\r')) return;
  switch (c) {
    case 'h':
      printHelp();
      break;
   case 'w':
      Serial.println("Switching to PCB mics");
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
      break;
   case 'W':
      Serial.println("Switching to line in on pink jack");
      //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
      break;
   case 'f':
      new_val = incrementFrequency(1000.0);
      Serial.println("Changing frequency to " + String(new_val) + " Hz");
      break;
   case 'F':
      new_val = incrementFrequency(-1000.0);
      Serial.println("Changing frequency to " + String(new_val) + " Hz");
      break;
   case 'a':
      new_val = incrementAmplitude(3.0);
      Serial.println("Changing amplitude to " + String(new_val));
      break;
   case 'A':
      new_val = incrementAmplitude(-3.0);
      Serial.println("Changing amplitude to " + String(new_val));
      break;
   case 'm':
      muteTone(true);
      Serial.println("Muting the tone");
      break;
   case 'M':
      muteTone(false);
      Serial.println("Unmuting the tone");
      break;    
  }
}
