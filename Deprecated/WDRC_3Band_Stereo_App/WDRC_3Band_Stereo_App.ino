/*
  WDRC_3BandIIR_wBT_wAFC

  Created: Chip Audette (OpenAudio), 2018
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 3-band WDRC compressor with adaptive feedback cancelation (AFC)
      based on the work of BTNRH.

  Filters: The BTNRH filterbank was implemented in the frequency-domain, whereas
	  I implemented them in the time-domain via IIR filters.  Furthermore, I delay
	  the individual IIR filters to try to line up their impulse response so that
	  the overall frequency response is smoother because the phases are better aligned
	  in the cross-over region between neighboring filters.

  Compressor: The BTNRH WDRC compresssor did not include an expansion stage at low SPL.
	  I added an expansion stage to better manage noise.

  Feedback Management: Implemented the BTNHRH adaptivev feedback cancelation algorithm
    from their CHAPRO repository: https://github.com/BoysTownorg/chapro

  Connectivity: Communicates via USB Serial and via Bluetooth Serial

  User Controls:
    Potentiometer on Tympan controls the algorithm gain.

  MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

// Define the overall setup
String overall_name = String("Tympan: Stereo, 3-Band IIR WDRC, App Version");
const int N_CHAN_MAX = 3;  //number of frequency bands (channels)
int N_CHAN = N_CHAN_MAX;  //will be changed to user-selected number of channels later
const float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0; //will be overridden by volume knob
int USE_VOLUME_KNOB = 0;  //set to 1 to use volume knob to override the default vol_knob_gain_dB set a few lines below
const int LEFT = 0, RIGHT = 1;
const int LEFTRIGHT_NORMAL=0, LEFTRIGHT_MONO=1, LEFTRIGHT_MUTE=2;

//local files
#include "AudioEffectFeedbackCancel_F32.h"
#include "AudioEffectAFC_BTNRH_F32.h"
#include "SerialManager.h"

// define how to use Serial / Bluetooth messaging
#define BOTH_SERIAL audioHardware

const float sample_rate_Hz = 22050.0f ; //16000, 24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 16;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
TympanPins                    tympPins(TYMPAN_REV_D3);        //TYMPAN_REV_C or TYMPAN_REV_D
TympanBase                    audioHardware(tympPins);
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioMixer4_F32               leftRightMixer[2];        //left-right mixer for the left output and for the right output
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //keep this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
AudioFilterBiquad_F32      preFilter(audio_settings), preFilterR(audio_settings);  //remove low frequencies near DC
AudioEffectAFC_BTNRH_F32   feedbackCancel(audio_settings), feedbackCancelR(audio_settings);  //original adaptive feedback cancelation from BTNRH
AudioFilterBiquad_F32      bpFilt[2][N_CHAN_MAX];         //here are the filters to break up the audio into multiple bands
AudioEffectDelay_F32       postFiltDelay[2][N_CHAN_MAX];  //Here are the delay modules that we'll use to time-align the output of the filters
AudioEffectCompWDRC_F32    expCompLim[2][N_CHAN_MAX];     //here are the per-band compressors
AudioMixer8_F32            mixerFilterBank[2];                     //mixer to reconstruct the broadband audio
AudioEffectCompWDRC_F32    compBroadband[2];              //broad band compressor
AudioEffectFeedbackCancel_LoopBack_F32 feedbackLoopBack(audio_settings), feedbackLoopBackR(audio_settings);
AudioOutputI2S_F32          i2s_out(audio_settings);    //Digital audio output to the DAC.  Should be last.

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32  audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_filterbank(audio_settings);
AudioControlTestAmpSweep_F32    ampSweepTester(audio_settings, audioTestGenerator, audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings, audioTestGenerator, audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_FIR(audio_settings, audioTestGenerator, audioTestMeasurement_filterbank);

//make the audio connections
#define N_MAX_CONNECTIONS 150  //some large number greater than the number of connections that we'll make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count = 0;

  //connect input
  configureLeftRightMixer(LEFTRIGHT_NORMAL);
  patchCord[count++] = new AudioConnection_F32(i2s_in, LEFT, leftRightMixer[LEFT], LEFT); // (i2s_in,0) is left input
  patchCord[count++] = new AudioConnection_F32(i2s_in, RIGHT, leftRightMixer[LEFT], RIGHT); // (i2s_in,0) is left input
  patchCord[count++] = new AudioConnection_F32(i2s_in, LEFT, leftRightMixer[RIGHT], LEFT); // (i2s_in,0) is left input
  patchCord[count++] = new AudioConnection_F32(i2s_in, RIGHT, leftRightMixer[RIGHT], RIGHT); // (i2s_in,0) is left input

  patchCord[count++] = new AudioConnection_F32(leftRightMixer[LEFT], 0, audioTestGenerator, 0); // (i2s_in,0) is left input
  //patchCord[count++] = new AudioConnection_F32(i2s_in, RIGHT, feedbackCancelR, 0); // (i2s_in,1) is right input

  //make the connection for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_filterbank, 0);

  //start the algorithms with the feedback cancallation block
  //patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, feedbackCancel, 0);

  //make per-channel connections: filterbank -> delay -> WDRC Compressor -> mixer (synthesis)
  for (int Iear = LEFT; Iear <= RIGHT; Iear++) { //loop over channels
    for (int Iband = 0; Iband < N_CHAN_MAX; Iband++) {
      if (Iear == LEFT) {
        //patchCord[count++] = new AudioConnection_F32(feedbackCancel, 0, bpFilt[Iear][Iband], 0); //connect to Feedback canceler
        patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, bpFilt[Iear][Iband], 0); //input is coming from the audio test generator
      } else {
        //patchCord[count++] = new AudioConnection_F32(feedbackCancelR, 0, bpFilt[Iear][Iband], 0); //connect to Feedback canceler
        patchCord[count++] = new AudioConnection_F32(leftRightMixer[RIGHT], 0, bpFilt[Iear][Iband], 0); //input is coming directly from i2s_in
      }
      patchCord[count++] = new AudioConnection_F32(bpFilt[Iear][Iband], 0, postFiltDelay[Iear][Iband], 0);  //connect to delay
      patchCord[count++] = new AudioConnection_F32(postFiltDelay[Iear][Iband], 0, expCompLim[Iear][Iband], 0); //connect to compressor
      patchCord[count++] = new AudioConnection_F32(expCompLim[Iear][Iband], 0, mixerFilterBank[Iear], Iband); //connect to mixer

      //make the connection for the audio test measurements
      if (Iear == LEFT) {
        patchCord[count++] = new AudioConnection_F32(bpFilt[Iear][Iband], 0, audioTestMeasurement_filterbank, 1 + Iband);
      }
    }

    //connect the output of the mixers to the final broadband compressor
    patchCord[count++] = new AudioConnection_F32(mixerFilterBank[Iear], 0, compBroadband[Iear], 0);  //connect to final limiter
  }

  //connect the loop back to the adaptive feedback canceller
  feedbackLoopBack.setTargetAFC(&feedbackCancel);   //left ear
  feedbackLoopBackR.setTargetAFC(&feedbackCancelR); //right ear
  //patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, feedbackLoopBack, 0); //loopback to the adaptive feedback canceler
  //patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, feedbackLoopBackR, 0); //loopback to the adaptive feedback canceler

  //send the audio out
  for (int Iear=0; Iear <= RIGHT; Iear++) {
    patchCord[count++] = new AudioConnection_F32(compBroadband[Iear], 0, i2s_out, Iear);  //left output
  }

  //make the connections for the audio test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, audioTestMeasurement, 1);

  return count;
}


//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) {
  enable_printCPUandMemory = !enable_printCPUandMemory;
}; //"extern" let's be it accessible outside
bool enable_printAveSignalLevels = false, printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) {
  enable_printAveSignalLevels = !enable_printAveSignalLevels;
  printAveSignalLevels_as_dBSPL = as_dBSPL;
};
SerialManager serialManager(audioHardware, N_CHAN_MAX,
      expCompLim[LEFT], expCompLim[RIGHT], 
      ampSweepTester, freqSweepTester, freqSweepTester_FIR,
      feedbackCancel, feedbackCancelR);

//routine to setup the hardware
void setupTympanHardware(void) {
  BOTH_SERIAL.println("Setting up Tympan Audio Board...");
  audioHardware.enable(); // activate AIC

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  audioHardware.enableMicDetect(true);

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 70.0;  //set the default cutoff frequency for the highpass filter
  audioHardware.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disble
  //BOTH_SERIAL.print("Setting HP Filter in hardware at "); BOTH_SERIAL.print(audioHardware.getHPCutoff_Hz()); BOTH_SERIAL.println(" Hz.");

  //Choose the desired audio input on the Typman...this will be overridden by the serviceMicDetect() in loop()
  //audioHardware.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board micropphones
  audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); audioHardware.setEnableStereoExtMicBias(true); // use the microphone jack - defaults to mic bias 2.5V
  //audioHardware.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //set volumes
  audioHardware.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  audioHardware.setInputGain_dB(input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}


// /////////// setup the audio processing
//define functions to setup the audio processing parameters
#include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults
#include "GHA_Alternates.h"  //this sets alternate dsl and gha, which can be switched in via commands
#include "filter_coeff_sos.h"  //IIR filter coefficients for our filterbank
#define DSL_PRESET_A 0
#define DSL_PRESET_B 1
int current_dsl_config = DSL_PRESET_B; //used to select one of the configurations above for startup
float overall_cal_dBSPL_at0dBFS; //will be set later

void setupAudioProcessing(void) {
  //make all of the audio connections
  makeAudioConnections();

  //set the DC-blocking higpass filter cutoff
  preFilter.setHighpass(0, 40.0);  preFilterR.setHighpass(0, 40.0);

  //setup processing based on the DSL and GHA prescriptions
  setDSLConfiguration(current_dsl_config);
  //if (current_dsl_config == DSL_PRESET_A) {
  //  setupFromDSLandGHAandAFC(dsl, gha, afc, N_CHAN_MAX, audio_settings);
  //} else if (current_dsl_config == DSL_PRESET_B) {
  //  setupFromDSLandGHAandAFC(dsl_fullon, gha_fullon, afc_fullon, N_CHAN_MAX, audio_settings);
  //}
}

void setupFromDSLandGHAandAFC(const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
                              const BTNRH_WDRC::CHA_AFC &this_afc, const int n_chan_max, const AudioSettings_F32 &settings)
{
  //int n_chan = n_chan_max;  //maybe change this to be the value in the DSL itself.  other logic would need to change, too.
  N_CHAN = max(1, min(n_chan_max, this_dsl.nchannel));

  // //compute the per-channel filter coefficients
  //AudioConfigFIRFilterBank_F32 makeFIRcoeffs(n_chan, n_fir, settings.sample_rate_Hz, (float *)this_dsl.cross_freq, (float *)firCoeff);

  // Loop over each ear
  for (int Iear = LEFT; Iear <= RIGHT; Iear++) {

    //give the pre-computed coefficients to the IIR filters
    for (int Iband = 0; Iband < n_chan_max; Iband++) {
      if (Iband < N_CHAN) {
        bpFilt[Iear][Iband].setFilterCoeff_Matlab_sos(&(all_matlab_sos[Iband][0]), SOS_N_BIQUADS_PER_FILTER);  //from filter_coeff_sos.h.  Also calls begin().
      } else {
        bpFilt[Iear][Iband].end();
      }
    }

    //setup the per-channel delays
    for (int Iband = 0; Iband < n_chan_max; Iband++) {
      postFiltDelay[Iear][Iband].setSampleRate_Hz(audio_settings.sample_rate_Hz);
      if (Iband < N_CHAN) {
        postFiltDelay[Iear][Iband].delay(0, all_matlab_sos_delay_msec[Iband]); //from filter_coeff_sos.h.  milliseconds!!!
      } else {
        postFiltDelay[Iear][Iband].delay(0, 0); //from filter_coeff_sos.h.  milliseconds!!!
      }
    }

    //setup the AFC
    if (Iear == LEFT) {
      feedbackCancel.setParams(this_afc);
    } else {
      feedbackCancelR.setParams(this_afc);
    }

    //setup all of the per-channel compressors
    configurePerBandWDRCs(N_CHAN, settings.sample_rate_Hz, this_dsl, this_gha, expCompLim[Iear]);

    //setup the broad band compressor (limiter)
    configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, vol_knob_gain_dB, compBroadband[Iear]);
  }

  //overwrite the one-point calibration based on the dsl data structure
  overall_cal_dBSPL_at0dBFS = this_dsl.maxdB;

}


void setDSLConfiguration(int preset_ind) {
  switch (preset_ind) {
    case (DSL_PRESET_A):
      current_dsl_config = preset_ind;
      //BOTH_SERIAL.println("setDSLConfiguration: Using DSL Preset A");
      setupFromDSLandGHAandAFC(dsl, gha, afc, N_CHAN_MAX, audio_settings);
      audioHardware.setRedLED(true);  audioHardware.setAmberLED(false);
      break;
    case (DSL_PRESET_B):
      current_dsl_config = preset_ind;
      //BOTH_SERIAL.println("setDSLConfiguration: Using DSL Preset B");
      setupFromDSLandGHAandAFC(dsl_fullon, gha_fullon, afc_fullon, N_CHAN_MAX, audio_settings);
      audioHardware.setRedLED(false);  audioHardware.setAmberLED(true);
      break;
  }
  configureLeftRightMixer(LEFTRIGHT_NORMAL);
}

void configureBroadbandWDRCs(float fs_Hz, const BTNRH_WDRC::CHA_WDRC &this_gha,
                             float vol_knob_gain_dB, AudioEffectCompWDRC_F32 &WDRC)
{
  //assume all broadband compressors are the same
  //for (int i=0; i< ncompressors; i++) {
  //logic and values are extracted from from CHAPRO repo agc_prepare.c...the part setting CHA_DVAR

  //extract the parameters
  float atk = (float)this_gha.attack;  //milliseconds!
  float rel = (float)this_gha.release; //milliseconds!
  //float fs = this_gha.fs;
  float fs = (float)fs_Hz; // WEA override...not taken from gha
  float maxdB = (float) this_gha.maxdB;
  float exp_cr = (float) this_gha.exp_cr;
  float exp_end_knee = (float) this_gha.exp_end_knee;
  float tk = (float) this_gha.tk;
  float comp_ratio = (float) this_gha.cr;
  float tkgain = (float) this_gha.tkgain;
  float bolt = (float) this_gha.bolt;

  //set the compressor's parameters
  //WDRCs[i].setSampleRate_Hz(fs);
  //WDRCs[i].setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
  WDRC.setSampleRate_Hz(fs);
  WDRC.setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain + vol_knob_gain_dB, comp_ratio, tk, bolt);
  // }
}

void configurePerBandWDRCs(int nchan, float fs_Hz,
                           const BTNRH_WDRC::CHA_DSL &this_dsl, const BTNRH_WDRC::CHA_WDRC &this_gha,
                           AudioEffectCompWDRC_F32 *WDRCs)
{
  if (nchan > this_dsl.nchannel) {
    BOTH_SERIAL.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    BOTH_SERIAL.print(F("    : nchan = ")); BOTH_SERIAL.println(nchan);
    BOTH_SERIAL.print(F("    : dsl.nchannel = ")); BOTH_SERIAL.println(dsl.nchannel);
  }

  //now, loop over each channel
  for (int i = 0; i < nchan; i++) {

    //logic and values are extracted from from CHAPRO repo agc_prepare.c
    float atk = (float)this_dsl.attack;   //milliseconds!
    float rel = (float)this_dsl.release;  //milliseconds!
    //float fs = gha->fs;
    float fs = (float)fs_Hz; // WEA override
    float maxdB = (float) this_dsl.maxdB;
    float exp_cr = (float)this_dsl.exp_cr[i];
    float exp_end_knee = (float)this_dsl.exp_end_knee[i];
    float tk = (float) this_dsl.tk[i];
    float comp_ratio = (float) this_dsl.cr[i];
    float tkgain = (float) this_dsl.tkgain[i];
    float bolt = (float) this_dsl.bolt[i];

    // adjust BOLT
    float cltk = (float)this_gha.tk;
    if (bolt > cltk) bolt = cltk;
    if (tkgain < 0) bolt = bolt + tkgain;

    //set the compressor's parameters
    WDRCs[i].setSampleRate_Hz(fs);
    WDRCs[i].setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
  }
}

void configureLeftRightMixer(int LEFTRIGHT_setting) {
  switch (LEFTRIGHT_setting) {
    case LEFTRIGHT_NORMAL:
      leftRightMixer[LEFT].gain(LEFT,1.0);leftRightMixer[LEFT].gain(RIGHT,0.0);
      leftRightMixer[RIGHT].gain(LEFT,0.0);leftRightMixer[RIGHT].gain(RIGHT,1.0);
      break;
    case LEFTRIGHT_MONO:
      leftRightMixer[LEFT].gain(LEFT,0.5);leftRightMixer[LEFT].gain(RIGHT,0.5);
      leftRightMixer[RIGHT].gain(LEFT,0.5);leftRightMixer[RIGHT].gain(RIGHT,0.5);
      break;
    case LEFTRIGHT_MUTE:
      leftRightMixer[LEFT].gain(LEFT,0.0);leftRightMixer[LEFT].gain(RIGHT,0.0);
      leftRightMixer[RIGHT].gain(LEFT,0.0);leftRightMixer[RIGHT].gain(RIGHT,0.0);
      break;
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms
  audioHardware.beginBothSerial(); delay(1000);
  BOTH_SERIAL.print(overall_name); BOTH_SERIAL.println(": setup():...");
  BOTH_SERIAL.print("Sample Rate (Hz): "); BOTH_SERIAL.println(audio_settings.sample_rate_Hz);
  BOTH_SERIAL.print("Audio Block Size (samples): "); BOTH_SERIAL.println(audio_settings.audio_block_samples);
#if (USE_BT_SERIAL)
  BT_SERIAL.print(overall_name); BT_SERIAL.println(": setup():...");
  BT_SERIAL.print("Sample Rate (Hz): "); BT_SERIAL.println(audio_settings.sample_rate_Hz);
  BT_SERIAL.print("Audio Block Size (samples): "); BT_SERIAL.println(audio_settings.audio_block_samples);
#endif

  // Audio connections require memory
  AudioMemory_F32_wSettings(200, audio_settings); //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  setupTympanHardware();

  //setup filters and mixers
  setupAudioProcessing();

  //update the potentiometer settings
  if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //End of setup
  printGainSettings();
  BOTH_SERIAL.print("Setup complete:"); BOTH_SERIAL.println(overall_name);
  serialManager.printHelp();

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the potentiometer...if enough time has passed
  if (USE_VOLUME_KNOB) servicePotentiometer(millis());

  //check the mic_detect signal
  serviceMicDetect(millis(), 500);

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) printCPUandMemory(millis());

  //print info about the signal processing
  updateAveSignalLevels(millis());
  if (enable_printAveSignalLevels) printAveSignalLevels(millis(), printAveSignalLevels_as_dBSPL);

} //end loop()


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(audioHardware.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0 / 9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //send the potentiometer value to your algorithm as a control parameter
    //float scaled_val = val / 3.0; scaled_val = scaled_val * scaled_val;
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      setVolKnobGain_dB(val * 45.0f - 10.0f - input_gain_dB);
    }
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printGainSettings(void) {
  BOTH_SERIAL.print("Gain (dB): ");
  BOTH_SERIAL.print("Knob = "); BOTH_SERIAL.print(vol_knob_gain_dB, 1);
  BOTH_SERIAL.print(", PGA = "); BOTH_SERIAL.print(input_gain_dB, 1);
  BOTH_SERIAL.print(", Chan = ");
  for (int i = 0; i < N_CHAN; i++) {
    BOTH_SERIAL.print(expCompLim[LEFT][i].getGain_dB() - vol_knob_gain_dB, 1);
    BOTH_SERIAL.print(", ");
  }
  BOTH_SERIAL.println();
}

extern void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB + increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
  float prev_vol_knob_gain_dB = vol_knob_gain_dB;
  vol_knob_gain_dB = gain_dB;
  float linear_gain_dB;
  for (int i = 0; i < N_CHAN_MAX; i++) {
    for (int Iear=LEFT; Iear <= RIGHT; Iear++) {
      linear_gain_dB = vol_knob_gain_dB + (expCompLim[Iear][i].getGain_dB() - prev_vol_knob_gain_dB);
      expCompLim[Iear][i].setGain_dB(linear_gain_dB);
    }
  }
  printGainSettings();
}


void serviceMicDetect(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static unsigned int prev_val = 1111; //some sort of weird value
  unsigned int cur_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    cur_val = audioHardware.updateInputBasedOnMicDetect(); //if mic is plugged in, defaults to TYMPAN_INPUT_JACK_AS_MIC
    if (cur_val != prev_val) {
      if (cur_val) {
        BOTH_SERIAL.println("serviceMicDetect: detected plug-in microphone!  External mic now active.");
      } else {
        BOTH_SERIAL.println("serviceMicDetect: detected removal of plug-in microphone. On-board PCB mics now active.");
      }
    }
    prev_val = cur_val;
    lastUpdate_millis = curTime_millis;
  }
}

void printCPUandMemory(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printCPUandMemoryMessage();
#if (USE_BT_SERIAL)
    printCPUandMemoryMessage(&BT_SERIAL); //Bluetooth Serial
#endif
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printCPUandMemoryMessage(void) {
  BOTH_SERIAL.print("CPU Cur/Peak: ");
  BOTH_SERIAL.print(audio_settings.processorUsage());
  //BOTH_SERIAL.print(AudioProcessorUsage());
  BOTH_SERIAL.print("%/");
  BOTH_SERIAL.print(audio_settings.processorUsageMax());
  //BOTH_SERIAL.print(AudioProcessorUsageMax());
  BOTH_SERIAL.print("%,   ");
  BOTH_SERIAL.print("Dyn MEM Float32 Cur/Peak: ");
  BOTH_SERIAL.print(AudioMemoryUsage_F32());
  BOTH_SERIAL.print("/");
  BOTH_SERIAL.print(AudioMemoryUsageMax_F32());
  BOTH_SERIAL.println();
}

float aveSignalLevels_dBFS[2][N_CHAN_MAX]; //left ear and right ear, one for each frequency band
void updateAveSignalLevels(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how often to perform the averaging
  static unsigned long lastUpdate_millis = 0;
  float update_coeff = 0.2;

  //is it time to update the calculations
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    for (int Iear=LEFT; Iear <= RIGHT; Iear++) {
      for (int i = 0; i < N_CHAN_MAX; i++) { //loop over each band
        aveSignalLevels_dBFS[Iear][i] = (1.0 - update_coeff) * aveSignalLevels_dBFS[Iear][i] + update_coeff * expCompLim[Iear][i].getCurrentLevel_dB(); //running average
      }
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printAveSignalLevels(unsigned long curTime_millis, bool as_dBSPL) {
  static unsigned long updatePeriod_millis = 3000; //how often to print the levels to the screen
  static unsigned long lastUpdate_millis = 0;

  //is it time to print to the screen
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printAveSignalLevelsMessage(&Serial, as_dBSPL);
#if (USE_BT_SERIAL)
    printAveSignalLevelsMessage(&BT_SERIAL, as_dBSPL);
#endif
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printAveSignalLevelsMessage(Stream *s, bool as_dBSPL) {
  float offset_dB = 0.0f;
  String units_txt = String("dBFS");
  if (as_dBSPL) {
    offset_dB = overall_cal_dBSPL_at0dBFS;
    units_txt = String("est dBSPL");
  }
  BOTH_SERIAL.print("Ave Input Level ("); BOTH_SERIAL.print(units_txt); BOTH_SERIAL.print("), Per-Band = ");
  for (int i = 0; i < N_CHAN; i++) {
    BOTH_SERIAL.print(aveSignalLevels_dBFS[LEFT][i] + offset_dB, 1);
    BOTH_SERIAL.print(", ");
  }
  BOTH_SERIAL.println();
}
