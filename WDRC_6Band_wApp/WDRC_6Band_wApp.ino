/*
  WDRC_6Band_wApp

  Created: Chip Audette (OpenAudio), 2020
    Primarly built upon CHAPRO "Generic Hearing Aid" from
    Boys Town National Research Hospital (BTNRH): https://github.com/BTNRH/chapro

  Purpose: Implements 6-band WDRC compressor with adaptive feedback cancelation (AFC)
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
     Use the TympanRemote App from the Android Play Store: https://play.google.com/store/apps/details?id=com.creare.tympanRemote&hl=en_US

  User Controls:
    Potentiometer on Tympan controls the algorithm gain.

  MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

// Define parameters relating to the overall setup
String overall_name = String("Tympan: 6-Band IIR WDRC (Left only), with App Control");
const int N_CHAN_MAX = 6;  //number of frequency bands (channels)
int N_CHAN = N_CHAN_MAX;  //will be changed to user-selected number of channels later
const float input_gain_dB = 15.0f; //gain on the analog microphones...does not affect the PDM microphone
float vol_knob_gain_dB = 0.0; //will be overridden by volume knob
int USE_VOLUME_KNOB = 0;  //set to 1 to use volume knob to override the default vol_knob_gain_dB set a few lines below

//define some constants for use later
#define RUN_STEREO (false)
#if (RUN_STEREO)
#define N_EARPIECES 2
#else
#define N_EARPIECES 1
#endif
const int LEFT = 0, RIGHT = (LEFT+1);
const int FRONT = 0, REAR = 1;
//const int PDM_LEFT_FRONT = 3, PDM_LEFT_REAR = 2, PDM_RIGHT_FRONT = 1, PDM_RIGHT_REAR = 0;  //notice the weird order
const int PDM_RIGHT_FRONT = 3, PDM_RIGHT_REAR = 2, PDM_LEFT_FRONT = 1, PDM_LEFT_REAR = 0;  //Front/Rear is weird.  Left/Right matches the enclosure labeling.

const int LEFT_ANALOG = 0, RIGHT_ANALOG = 1, LEFT_EARPIECE = 2, RIGHT_EARPIECE = 3;
const int  OUTPUT_LEFT_EARPIECE = 3, OUTPUT_RIGHT_EARPIECE = 2;  //Left/Right matches the enclosure...but is backwards from ideal
const int  OUTPUT_LEFT_TYMPAN = 0, OUTPUT_RIGHT_TYMPAN = 1;  //left/right for headphone jack on main tympan board
const int ANALOG_IN = 0, PDM_IN = 1;

//local files
#include "AudioEffectFeedbackCancel_F32.h"
#include "AudioEffectAFC_BTNRH_F32.h"
#include "SerialManager.h"

//define the sample rate and audio block size
const float sample_rate_Hz = 22050.0f ; //16000, 24000 or 44117.64706f (or other frequencies in the table in AudioOutputI2S_F32
const int audio_block_samples = 16;  //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32   audio_settings(sample_rate_Hz, audio_block_samples);

// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
Tympan                        myTympan(TympanRev::D);   //use TympanRev::D or TympanRev::C
AICShield                     earpieceShield(TympanRev::D, AICShieldRev::A);
AudioInputI2SQuad_F32         i2s_in(audio_settings);   //Digital audio input from the ADC
AudioMixer4_F32               earpieceMixer[2];         //mixes front-back earpiece mics
AudioSummer4_F32              analogVsDigitalSwitch[2];  //switches between analog and PDM (a summer is cpu cheaper than a mixer, and we don't need to mix here)
AudioMixer4_F32               leftRightMixer[2];        //mixers to control mono versus stereo
AudioTestSignalGenerator_F32  audioTestGenerator(audio_settings); //keep this to be *after* the creation of the i2s_in object

//create audio objects for the algorithm
AudioFilterBiquad_F32      preFilter(audio_settings), preFilterR(audio_settings);  //remove low frequencies near DC
AudioEffectAFC_BTNRH_F32   feedbackCancel(audio_settings), feedbackCancelR(audio_settings);  //original adaptive feedback cancelation from BTNRH
AudioFilterBiquad_F32      bpFilt[2][N_CHAN_MAX];         //here are the filters to break up the audio into multiple bands
AudioConfigIIRFilterBank_F32 filterBankCalculator(audio_settings);  //this computes the filter coefficients
//AudioFilterIIR_F32         bpFilt[2][N_CHAN_MAX];           //here are the filters to break up the audio into multiple bands
AudioEffectDelay_F32       postFiltDelay[2][N_CHAN_MAX];  //Here are the delay modules that we'll use to time-align the output of the filters
AudioEffectCompWDRC_F32    expCompLim[2][N_CHAN_MAX];     //here are the per-band compressors
AudioMixer8_F32            mixerFilterBank[2];                     //mixer to reconstruct the broadband audio
AudioEffectCompWDRC_F32    compBroadband[2];              //broad band compressor
AudioEffectFeedbackCancel_LoopBack_Local_F32 feedbackLoopBack(audio_settings), feedbackLoopBackR(audio_settings);
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2SQuad_F32      i2s_out(audio_settings);    //Digital audio output to the DAC.  Should be last.

//complete the creation of the tester objects
AudioTestSignalMeasurement_F32  audioTestMeasurement(audio_settings);
AudioTestSignalMeasurementMulti_F32  audioTestMeasurement_filterbank(audio_settings);
AudioControlTestAmpSweep_F32    ampSweepTester(audio_settings, audioTestGenerator, audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester(audio_settings, audioTestGenerator, audioTestMeasurement);
AudioControlTestFreqSweep_F32    freqSweepTester_perBand(audio_settings, audioTestGenerator, audioTestMeasurement_filterbank);

//make the audio connections
#define N_MAX_CONNECTIONS 150  //some large number greater than the number of connections that we'll make
AudioConnection_F32 *patchCord[N_MAX_CONNECTIONS];
int makeAudioConnections(void) { //call this in setup() or somewhere like that
  int count = 0;

  //connect input to earpiece mixer
  patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_LEFT_FRONT, earpieceMixer[LEFT], FRONT); 
  patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_LEFT_REAR, earpieceMixer[LEFT], REAR); 
  //if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_RIGHT_FRONT, earpieceMixer[RIGHT], FRONT); 
    patchCord[count++] = new AudioConnection_F32(i2s_in, PDM_RIGHT_REAR, earpieceMixer[RIGHT], REAR); 
  //}

  //analog versus earpiece switching
  patchCord[count++] = new AudioConnection_F32(i2s_in, LEFT, analogVsDigitalSwitch[LEFT], ANALOG_IN); 
  patchCord[count++] = new AudioConnection_F32(earpieceMixer[LEFT], 0, analogVsDigitalSwitch[LEFT], PDM_IN); 
  //if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(i2s_in, RIGHT, analogVsDigitalSwitch[RIGHT], ANALOG_IN); 
    patchCord[count++] = new AudioConnection_F32(earpieceMixer[RIGHT], 0, analogVsDigitalSwitch[RIGHT], PDM_IN); 
  //}
 
  //connect analog and digital inputs for left side and then for the right side
  patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[LEFT], 0, leftRightMixer[LEFT], LEFT);      //all possible connections intended for LEFT output
  //if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[RIGHT], 0, leftRightMixer[LEFT], RIGHT);    //all possible connections intended for LEFT output
  if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[LEFT], 0, leftRightMixer[RIGHT], LEFT);     //all possible connections intended for RIGHT output
    patchCord[count++] = new AudioConnection_F32(analogVsDigitalSwitch[RIGHT], 0, leftRightMixer[RIGHT], RIGHT);   //all possible connections intended for RIGHT output
  }

  //add prefilters
  patchCord[count++] = new AudioConnection_F32(leftRightMixer[LEFT],0,preFilter,0);
  if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(leftRightMixer[RIGHT],0,preFilterR,0);
  }
  
  
  //configure the mixer's default state until set later
  configureEarpieceMixer(State::MIC_FRONT);
  configureLeftRightMixer(State::INPUTMIX_MUTE); //set left mixer to only listen to left, set right mixer to only listen to right


  //make the connection for the audio test measurements...only relevant when the audio test functions are being invoked by the engineer/user
  patchCord[count++] = new AudioConnection_F32(preFilter, 0, audioTestGenerator, 0); // set output of left mixer to the audioTestGenerator (which is will then pass through)
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0); // connect test generator to test measurements
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement_filterbank, 0); //connect test generator to the test measurement filterbank

  //start the algorithms with the feedback cancallation block
  #if 1  //set to zero to discable the adaptive feedback cancelation
    patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, feedbackCancel, 0); //remember, even the normal audio is coming through the audioTestGenerator
  #endif

  //make per-channel connections: filterbank -> delay -> WDRC Compressor -> mixer (synthesis)
  for (int Iear = 0; Iear < N_EARPIECES; Iear++) { //loop over channels
    for (int Iband = 0; Iband < N_CHAN_MAX; Iband++) {
      if (Iear == LEFT) {
        #if 1  //set to zero to discable the adaptive feedback cancelation
          patchCord[count++] = new AudioConnection_F32(feedbackCancel, 0, bpFilt[Iear][Iband], 0); //connect to Feedback canceler //perhaps 
        #else
          patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, bpFilt[Iear][Iband], 0); //input is coming from the audio test generator
        #endif
      } else {
        #if 1  //set to zero to discable the adaptive feedback cancelation
          patchCord[count++] = new AudioConnection_F32(feedbackCancelR, 0, bpFilt[Iear][Iband], 0); //connect to Feedback canceler
        #else
          patchCord[count++] = new AudioConnection_F32(preFilterR, 0, bpFilt[Iear][Iband], 0); //input is coming directly from i2s_in
        #endif
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
  if (RUN_STEREO) feedbackLoopBackR.setTargetAFC(&feedbackCancelR); //right ear
  
  #if 1  // set to zero to disable the adaptive feedback canceler
    patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, feedbackLoopBack, 0); //loopback to the adaptive feedback canceler
    patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, feedbackLoopBackR, 0); //loopback to the adaptive feedback canceler
  #endif

  //send the audio out
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_LEFT_EARPIECE); 
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_LEFT_TYMPAN); 
  if (RUN_STEREO) {
    patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, i2s_out, OUTPUT_RIGHT_EARPIECE); 
    patchCord[count++] = new AudioConnection_F32(compBroadband[RIGHT], 0, i2s_out, OUTPUT_RIGHT_TYMPAN); 
  } else {
    //copy mono audio to other channel to present in both ears
    patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_RIGHT_EARPIECE); 
    patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, i2s_out, OUTPUT_RIGHT_TYMPAN); 
  }
  
  //make the last connections for the audio test measurements and SD writer
  patchCord[count++] = new AudioConnection_F32(audioTestGenerator, 0, audioTestMeasurement, 0);
  patchCord[count++] = new AudioConnection_F32(compBroadband[LEFT], 0, audioTestMeasurement, 1);
  patchCord[count++] = new AudioConnection_F32(leftRightMixer[LEFT], 0, audioSDWriter, 0);
  patchCord[count++] = new AudioConnection_F32(i2s_out, OUTPUT_LEFT_EARPIECE, audioSDWriter, 1);
  

  return count;
}


//control display and serial interaction
State myState(&audio_settings, &myTympan);  //for keeping track of state (especially useful for sync'ing to mobile App)
bool enable_printAveSignalLevels = false, printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) {
  enable_printAveSignalLevels = !enable_printAveSignalLevels;
  printAveSignalLevels_as_dBSPL = as_dBSPL;
};
SerialManager serialManager(N_CHAN_MAX, 
      ampSweepTester, freqSweepTester, freqSweepTester_perBand,
      feedbackCancel, feedbackCancelR);

//Setup the hardware
void setupTympanHardware(void) {
  myTympan.enable(); // activate AIC
  earpieceShield.enable();

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  myTympan.enableMicDetect(true);

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  earpieceShield.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable

  //Choose the default input
  if (1) {
    setAnalogInputSource(State::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop()
    setInputAnalogVsPDM(State::INPUT_PDM); // ****but*** then activate the PDM mics
    Serial.println("setupTympanHardware: PDM Earpiece is the active input.");
  } else {
    //setAnalogInputSource(State::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop()
    setAnalogInputSource(State::INPUT_MICJACK_MIC);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop()
    Serial.println("setupTympanHardware: analog input is the active input.");
  }
  
  //Set the Bluetooth audio to go straight to the headphone amp, not through the Tympan software
  myTympan.mixBTAudioWithOutput(true);

  //set volumes
  myTympan.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(input_gain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}

int setInputAnalogVsPDM(int input) {
  int prev_val = myState.input_mixer_config; 
  configureLeftRightMixer(State::INPUTMIX_MUTE); //mute the system
  switch (input) {
    case State::INPUT_PDM:
      myTympan.enableDigitalMicInputs(true);  //two of the earpiece digital mics are routed here
      earpieceShield.enableDigitalMicInputs(true);  //the other two of the earpiece digital mics are routed here
      analogVsDigitalSwitch[LEFT].switchChannel(PDM_IN);
      analogVsDigitalSwitch[RIGHT].switchChannel(PDM_IN);
      myState.input_analogVsPDM = input;
      break;
    case State::INPUT_ANALOG:
      myTympan.enableDigitalMicInputs(false);  //two of the earpiece digital mics are routed here
      earpieceShield.enableDigitalMicInputs(false);  //the other two of the earpiece digital mics are routed here
      analogVsDigitalSwitch[LEFT].switchChannel(ANALOG_IN);
      analogVsDigitalSwitch[RIGHT].switchChannel(ANALOG_IN);
      myState.input_analogVsPDM = input;
      break;
  }
  configureLeftRightMixer(prev_val); //unmute the system
  return myState.input_analogVsPDM;
}

//Choose the input to use
//set the desired input source 
int setAnalogInputSource(int input_config) { 
  int prev_val = myState.input_mixer_config; 
  configureLeftRightMixer(State::INPUTMIX_MUTE); //mute the system
    
  switch (input_config) {
    case State::INPUT_PCBMICS:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      //earpieceShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);  //this doesn't really exist

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      myState.input_analog_config = input_config;
      break;
      
    case State::INPUT_MICJACK_MIC:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);

      //Set input gain in dB
      setInputGain_dB(15.0);

      //Store configuration
      myState.input_analog_config = input_config;
      break;
      
  case State::INPUT_MICJACK_LINEIN:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the mic jack
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      myState.input_analog_config = input_config;
      break;     
    case State::INPUT_LINEIN_SE:      
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes
      earpieceShield.inputSelect(TYMPAN_INPUT_LINE_IN);

      //Set input gain in dB
      setInputGain_dB(0.0);

      //Store configuration
      myState.input_analog_config = input_config;
      break;
  }
  configureLeftRightMixer(prev_val); //unmute the system
  return myState.input_analog_config;
}


// /////////// setup the audio processing
//define functions to setup the audio processing parameters
#include "GHA_Constants.h"  //this sets dsl and gha settings, which will be the defaults
#include "GHA_FullOn.h"     //this sets alternate dsl and gha, which can be switched in via commands
#include "GHA_RTS.h"        //this sets a third dsl and gha, which can be switched in via commands
float overall_cal_dBSPL_at0dBFS; //will be set later

void setupAudioProcessing(void) {
  //make all of the audio connections
  makeAudioConnections();
  Serial.println("setupAudioProcessing: makeAudioConnections() is complete.");
  
  //set the DC-blocking higpass filter cutoff...this is the filter done here in software, not the one done in the AIC DSP hardware
  preFilter.setHighpass(0, 60.0);  preFilterR.setHighpass(0, 60.0); 
  
  //setup processing based on the DSL and GHA prescriptions
  Serial.println("setupAudioProcessing: starting setAlgorithmPreset()");
  setAlgorithmPreset(myState.current_alg_config); //sets the Per Band, the Broad Band, and the AFC parameters using a preset
}


// define filter parameters
#define MAX_IIR_FILT_ORDER 6                        //filter order (note: in Matlab, an "N=3" bandpass is actually a 6th-order filter
#define N_BIQUAD_PER_FILT (MAX_IIR_FILT_ORDER/2)    //how many biquads per filter?  If the filter order is 6, there will be 3 biquads
#define COEFF_PER_BIQUAD  6                         //3 "b" coefficients and 3 "a" coefficients per biquad
float  filter_sos[N_CHAN_MAX][N_BIQUAD_PER_FILT * COEFF_PER_BIQUAD];   //this holds all the biquad filter coefficients
int    filter_delay[N_CHAN_MAX];                    //added delay (samples) for each filter (int[8])

// setup the per-band processing
void setupFromDSL(BTNRH_WDRC::CHA_DSL &this_dsl, float gha_tk, const int n_chan_max, const AudioSettings_F32 &settings) {
  
  //int n_chan = n_chan_max;  //maybe change this to be the value in the DSL itself.  other logic would need to change, too.
  N_CHAN = max(1, min(n_chan_max, this_dsl.nchannel));

  // filterbank parameters
  int n_chan = N_CHAN;          //number of channels
  int n_iir = MAX_IIR_FILT_ORDER;  //filter order
  float sample_rate_Hz = audio_settings.sample_rate_Hz; //sample rate Hz)
  float td_msec = 2.5f;          //allowed time delay (msec)
  float *crossover_freq = this_dsl.cross_freq;  //crossover frequencies (Hz)
 
  // //compute the per-channel filter coefficients
  Serial.println("setupFromDSL: computing SOS filter coefficients...");
  filterBankCalculator.createFilterCoeff_SOS(n_chan, n_iir, sample_rate_Hz, td_msec, crossover_freq,  // these are the inputs
        (float *)filter_sos, filter_delay);  //these are the outputs

  #if 0
  //plot coefficients (for debugging)
  for (int Iband = 0; Iband < n_chan_max; Iband++) {
    Serial.print("setupFromDSL: Band ");Serial.println(Iband);
    for (int Ibiquad = 0; Ibiquad < N_BIQUAD_PER_FILT; Ibiquad++) {
      Serial.print(": Biquad "); Serial.print(Ibiquad); 
      for (int i=0; i<COEFF_PER_BIQUAD; i++){ Serial.print(filter_sos[Iband][(Ibiquad*COEFF_PER_BIQUAD)+i],7);Serial.print(", ");};
      Serial.println();
    } 
  }
  #endif
  
  // Loop over each ear
  Serial.println("setupFromDSL: deploying SOS filter coefficients to the filter objects...");
  for (int Iear = 0; Iear <= N_EARPIECES; Iear++) {
    //give the pre-computed coefficients to the IIR filters
    for (int Iband = 0; Iband < n_chan_max; Iband++) {
      if (Iband < N_CHAN) {
        bpFilt[Iear][Iband].setFilterCoeff_Matlab_sos(&(filter_sos[Iband][0]), N_BIQUAD_PER_FILT);  //from filter_coeff_sos.h.  Also calls begin().
      } else {
        bpFilt[Iear][Iband].end();
      }
    }
    
    //setup the per-channel delays
    for (int Iband = 0; Iband < n_chan_max; Iband++) {
      postFiltDelay[Iear][Iband].setSampleRate_Hz(audio_settings.sample_rate_Hz);
      if (Iband < N_CHAN) {
        //postFiltDelay[Iear][Iband].delay(0, all_matlab_sos_delay_msec[Iband]); //from filter_coeff_sos.h.  milliseconds!!!
        postFiltDelay[Iear][Iband].delay(0, ((float)filter_delay[Iband]/sample_rate_Hz)*1000.0f); //from filter_coeff_sos.h.  milliseconds!!!
      } else {
        postFiltDelay[Iear][Iband].delay(0, 0); //from filter_coeff_sos.h.  milliseconds!!!
      }
    }
  
    //setup all of the per-channel compressors
    configurePerBandWDRCs(N_CHAN, settings.sample_rate_Hz, this_dsl, gha_tk, expCompLim[Iear]);
  }
  //overwrite the one-point calibration based on the dsl data structure
  overall_cal_dBSPL_at0dBFS = this_dsl.maxdB;
  
  //save the state
  myState.wdrc_perBand = this_dsl;  //shallow copy the contents of this_dsl into wdrc_perBand
  //myState.printPerBandSettings();  //debugging!

  //active the correct earpiece
  if (this_dsl.ear == 1) {
    configureLeftRightMixer(State::INPUTMIX_BOTHRIGHT); //listen to the right mic(s)
  } else {
    configureLeftRightMixer(State::INPUTMIX_BOTHLEFT); //listen to the left mic(s)
  }

}

void setupFromDSLandGHAandAFC(BTNRH_WDRC::CHA_DSL &this_dsl, BTNRH_WDRC::CHA_WDRC &this_gha,
                              BTNRH_WDRC::CHA_AFC &this_afc, const int n_chan_max, const AudioSettings_F32 &settings)
{
  //setup all the DSL-based parameters (ie, the processing per frequency band)
  setupFromDSL(this_dsl, (float)this_gha.tk, n_chan_max, settings);  //this also sets the current dsl state to the given DSL
  
  for (int Iear = 0; Iear <= N_EARPIECES; Iear++) {
    //setup the AFC
    if (Iear == LEFT) {
      feedbackCancel.setParams(this_afc);
    } else {
      feedbackCancelR.setParams(this_afc);
    }
    
      //setup the broad band compressor (limiter)
      configureBroadbandWDRCs(settings.sample_rate_Hz, this_gha, vol_knob_gain_dB, compBroadband[Iear]);
  }
  
  
  //save the state
  myState.wdrc_broadBand = this_gha; //shallow copy into wdrc_broadBand
  myState.afc = this_afc;   //shallow copy into AFC
  //myState.printBroadbandSettings();
}


void setAlgorithmPreset(int preset_ind) {
  switch (preset_ind) {
    case (State::ALG_PRESET_A):
      myState.current_alg_config = preset_ind;
      //myTympan.println("setAlgorithmPreset: Using DSL Preset A");
      setupFromDSLandGHAandAFC(dsl, gha, afc, N_CHAN_MAX, audio_settings);
      //myTympan.setRedLED(true);  myTympan.setAmberLED(false);
      break;
    case (State::ALG_PRESET_B):
      myState.current_alg_config = preset_ind;
      //myTympan.println("setAlgorithmPreset: Using DSL Preset B");
      setupFromDSLandGHAandAFC(dsl_fullon, gha_fullon, afc_fullon, N_CHAN_MAX, audio_settings);
      //myTympan.setRedLED(false);  myTympan.setAmberLED(true);
      break;
    case (State::ALG_PRESET_C):
      myState.current_alg_config = preset_ind;
      //myTympan.println("setAlgorithmPreset: Using DSL Preset C");
      setupFromDSLandGHAandAFC(dsl_rts, gha_rts, afc_rts, N_CHAN_MAX, audio_settings);
      //myTympan.setRedLED(false);  myTympan.setAmberLED(true);
      break;      
  }
  //configureLeftRightMixer(State::INPUTMIX_STEREO);
  
}

void updateDSL(BTNRH_WDRC::CHA_DSL &this_dsl) {
  //setupFromDSLandGHAandAFC(this_dsl, myState.wdrc_broadBand, myState.afc, N_CHAN_MAX, audio_settings);
  setupFromDSL(this_dsl, myState.wdrc_broadBand.tk, N_CHAN_MAX, audio_settings);
}

void updateGHA(BTNRH_WDRC::CHA_WDRC &this_gha) {
  setupFromDSLandGHAandAFC(myState.wdrc_perBand, this_gha, myState.afc, N_CHAN_MAX, audio_settings);
}

void updateAFC(BTNRH_WDRC::CHA_AFC &this_afc) {
  setupFromDSLandGHAandAFC(myState.wdrc_perBand, myState.wdrc_broadBand, this_afc, N_CHAN_MAX, audio_settings);
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
                           const BTNRH_WDRC::CHA_DSL &this_dsl, float gha_tk,
                           AudioEffectCompWDRC_F32 *WDRCs)
{
  if (nchan > this_dsl.nchannel) {
    myTympan.println(F("configureWDRC.configure: *** ERROR ***: nchan > dsl.nchannel"));
    myTympan.print(F("    : nchan = ")); myTympan.println(nchan);
    myTympan.print(F("    : dsl.nchannel = ")); myTympan.println(dsl.nchannel);
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
    //float cltk = (float)this_gha.tk;
    //float cltk = gha_tk; //this is enabled in the original BTNRH code.  *Temporarily* disabled by WEA 7/31/2020
    //if (bolt > cltk) bolt = cltk;  //this is enabled in the original BTNRH code.  *Temporarily* disabled by WEA 7/31/2020
    if (tkgain < 0) bolt = bolt + tkgain;

    //set the compressor's parameters
    WDRCs[i].setSampleRate_Hz(fs);
    WDRCs[i].setParams(atk, rel, maxdB, exp_cr, exp_end_knee, tkgain, comp_ratio, tk, bolt);
  }
}

int configureEarpieceMixer(int val) {
  switch (val) {
    case State::MIC_FRONT:
      earpieceMixer[LEFT].gain(FRONT, 1.0);earpieceMixer[LEFT].gain(REAR, 0.0);
      earpieceMixer[RIGHT].gain(FRONT, 1.0);earpieceMixer[RIGHT].gain(REAR, 0.0);
      myState.input_earpiece_config = val;
      break;
    case State::MIC_REAR:
      earpieceMixer[LEFT].gain(FRONT, 0.0);earpieceMixer[LEFT].gain(REAR, 1.0);
      earpieceMixer[RIGHT].gain(FRONT, 0.0);earpieceMixer[RIGHT].gain(REAR, 1.0);
      myState.input_earpiece_config = val;
      break;
    case State::MIC_BOTH_INPHASE:
      earpieceMixer[LEFT].gain(FRONT, 0.5);earpieceMixer[LEFT].gain(REAR, 0.5);
      earpieceMixer[RIGHT].gain(FRONT, 0.5);earpieceMixer[RIGHT].gain(REAR, 0.5);
      myState.input_earpiece_config = val;
      break;  
    case State::MIC_BOTH_INVERTED:
      earpieceMixer[LEFT].gain(FRONT, 1.0);earpieceMixer[LEFT].gain(REAR, -1.0);
      earpieceMixer[RIGHT].gain(FRONT, 1.0);earpieceMixer[RIGHT].gain(REAR, -1.0);
      myState.input_earpiece_config = val;
      break;           
  }
  return myState.input_earpiece_config;
}

//int configureLeftRightMixer(void) { return configureLeftRightMixer(myState.input_mixer_config); } 
int configureLeftRightMixer(int val) {  //call this if you want to change left-right mixing (or if the input source was changed)
  switch (val) {  // now configure that mix for the analog inputs  
    case State::INPUTMIX_STEREO:
      myState.input_mixer_config = val;
      leftRightMixer[LEFT].gain(LEFT,1.0);leftRightMixer[LEFT].gain(RIGHT,0.0);   //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT,0.0);leftRightMixer[RIGHT].gain(RIGHT,1.0); //set what is sent right
      break;
    case State::INPUTMIX_MONO:
      myState.input_mixer_config = val;
      leftRightMixer[LEFT].gain(LEFT,0.5);leftRightMixer[LEFT].gain(RIGHT,0.5);   //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT,0.5);leftRightMixer[RIGHT].gain(RIGHT,0.5); //set what is sent right
      break;
    case State::INPUTMIX_MUTE:
      myState.input_mixer_config = val;
      leftRightMixer[LEFT].gain(LEFT,0.0);leftRightMixer[LEFT].gain(RIGHT,0.0);     //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT,0.0);leftRightMixer[RIGHT].gain(RIGHT,0.0);   //set what is sent right
      break;
    case State::INPUTMIX_BOTHLEFT:
      myState.input_mixer_config = val;
      leftRightMixer[LEFT].gain(LEFT,1.0);leftRightMixer[LEFT].gain(RIGHT,0.0);     //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT,1.0);leftRightMixer[RIGHT].gain(RIGHT,0.0);   //set what is sent right
      break;
    case State::INPUTMIX_BOTHRIGHT:
      myState.input_mixer_config = val;
      leftRightMixer[LEFT].gain(LEFT,0.0);leftRightMixer[LEFT].gain(RIGHT,1.0);     //set what is sent left
      leftRightMixer[RIGHT].gain(LEFT,0.0);leftRightMixer[RIGHT].gain(RIGHT,1.0);   //set what is sent right
      break;   
  }
  return myState.input_mixer_config;
}

bool enableAFC(bool enable) {
  myState.afc.default_to_active = enable;
  feedbackCancel.setEnable(enable);
  feedbackCancelR.setEnable(enable);
  return myState.afc.default_to_active ;
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms
  myTympan.beginBothSerial(); delay(1500);
  myTympan.print(overall_name); myTympan.println(": setup():...");
  myTympan.print("Sample Rate (Hz): "); myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("Audio Block Size (samples): "); myTympan.println(audio_settings.audio_block_samples);

  // Audio connections require memory
  AudioMemory_F32_wSettings(70, audio_settings); //allocate Float32 audio data blocks (primary memory used for audio processing)

  // Enable the audio shield, select input, and enable output
  Serial.println("setup: starting setupTympanHardware()");
  setupTympanHardware();

  //setup filters and mixers
  Serial.println("setup: starting setupAudioProcessing()");
  setupAudioProcessing();

  //update the potentiometer settings
  if (USE_VOLUME_KNOB) servicePotentiometer(millis(),0);  //the "0" forces it to read the pot now

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(2);             //four channels for this quad recorder, but you could set it to 2
  int ret_val = -999;
  ret_val = audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  Serial.print("setup: setWriteDataType() yielded return code: "); 
  Serial.print(ret_val); 
  if (ret_val < 0) { 
    Serial.println(": ERROR!"); 
  } else { 
    Serial.println(": OK"); 
  }
  #if 1
  ret_val = audioSDWriter.allocateBuffer(75000);  //I believe that 150000 is the default, but we might not have that much available RAM in this sketch
  Serial.print("setup: allocating SD buffer yields return code: ");Serial.print(ret_val); Serial.print(": "); 
  if (ret_val == -2) {
    Serial.println("BUFF_WRITER NOT CREATED YET.");
  } else if (ret_val < 0) { 
    Serial.println("ERROR"); 
  } else if (ret_val > 0) {
    Serial.println("SUCCESS!!");
  } else { 
    Serial.println("OK"); 
  };
  #endif
  myTympan.print("Configured for "); myTympan.print(audioSDWriter.getNumWriteChannels()); myTympan.println(" channels to SD.");


  //End of setup
  printGainSettings();
  myTympan.print("Setup complete: "); myTympan.println(overall_name);
  serialManager.printHelp();

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //choose to sleep ("wait for interrupt") instead of spinning our wheels doing nothing but consuming power
  //asm(" WFI");  //ARM-specific.  Will wake on next interrupt.  The audio library issues tons of interrupts, so we wake up often.

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  while (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the potentiometer...if enough time has passed
  if (USE_VOLUME_KNOB) servicePotentiometer(millis(),100); //service the pot every 100 msec

  //service the SD recording
  serviceSD();

  //service the LEDs
  serviceLEDs();   //Remember that the LEDs on the Tympan board are not visible with CCP shield in-place.

  //check the mic_detect signal
  serviceMicDetect(millis(), 500);  //service the MicDetect every 500 msec

  //update the memory and CPU usage...if enough time has passed
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(),3000); //print CPU and mem every 3000 msec
  if (myState.flag_printCPUtoGUI) serialManager.printCPUtoGUI(millis(),3000);

  //print info about the signal processing
  updateAveSignalLevels(millis());
  if (enable_printAveSignalLevels) printAveSignalLevels(millis(), printAveSignalLevels_as_dBSPL);

  //print plottable data
  //if (myState.flag_printPlottableData) printPlottableData(millis(), 250);  //print values every 250msec
  if (myState.flag_printPlottableData) printPlottableData(millis(), 1000);  //print values every 250msec


} //end loop()


// ///////////////// Servicing routines


//serviceLEDs: This toggles the big blue LED on the CCP shield differently based on state.
void serviceLEDs(void) {
  static int loop_count = 0;
  loop_count++;
  
  if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
    if (loop_count > 200000) {  //slow toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {

    //let's flicker the LEDs while writing
    loop_count++;
    if (loop_count > 20000) { //fast toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } else {
    //myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW); //Go Red
    if (loop_count > 200000) { //slow toggle
      loop_count = 0;
      toggleLEDs(false,true); //just blink the red
    }
  }
}

void toggleLEDs(void) {
  toggleLEDs(true,true);  //toggle both
}
void toggleLEDs(const bool &useAmber, const bool &useRed) {
  static bool LED = false;
  LED = !LED;
  if (LED) {
    if (useAmber) myTympan.setAmberLED(true);
    if (useRed) myTympan.setRedLED(false);
  } else {
    if (useAmber) myTympan.setAmberLED(false);
    if (useRed) myTympan.setRedLED(true);
  }

  if (!useAmber) myTympan.setAmberLED(false);
  if (!useRed) myTympan.setRedLED(false);
}



#define PRINT_OVERRUN_WARNING 1   //set to 1 to print a warning that the there's been a hiccup in the writing to the SD.
void serviceSD(void) {
  static int max_max_bytes_written = 0; //for timing diagnotstics
  static int max_bytes_written = 0; //for timing diagnotstics
  static int max_dT_micros = 0; //for timing diagnotstics
  static int max_max_dT_micros = 0; //for timing diagnotstics

  unsigned long dT_micros = micros();  //for timing diagnotstics
  int bytes_written = audioSDWriter.serviceSD();
  dT_micros = micros() - dT_micros;  //timing calculation

  if ( bytes_written > 0 ) {

    max_bytes_written = max(max_bytes_written, bytes_written);
    max_dT_micros = max((int)max_dT_micros, (int)dT_micros);

    if (dT_micros > 10000) {  //if the write took a while, print some diagnostic info

      max_max_bytes_written = max(max_bytes_written, max_max_bytes_written);
      max_max_dT_micros = max(max_dT_micros, max_max_dT_micros);

      Serial.print("serviceSD: bytes written = ");
      Serial.print(bytes_written); Serial.print(", ");
      Serial.print(max_bytes_written); Serial.print(", ");
      Serial.print(max_max_bytes_written); Serial.print(", ");
      Serial.print("dT millis = ");
      Serial.print((float)dT_micros / 1000.0, 1); Serial.print(", ");
      Serial.print((float)max_dT_micros / 1000.0, 1); Serial.print(", ");
      Serial.print((float)max_max_dT_micros / 1000.0, 1); Serial.print(", ");
      Serial.println();
      max_bytes_written = 0;
      max_dT_micros = 0;
    }

    //print a warning if there has been an SD writing hiccup
    if (PRINT_OVERRUN_WARNING) {
      //if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
      if (i2s_in.get_isOutOfMemory()) {
        float approx_time_sec = ((float)(millis() - audioSDWriter.getStartTimeMillis())) / 1000.0;
        if (approx_time_sec > 0.1) {
          myTympan.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          myTympan.println(approx_time_sec );
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}


//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis,unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
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


float getChannelLinearGain_dB(int chan) { //chan starts counting from zero
  return getChannelLinearGain_dB(LEFT, chan);
}
float getChannelLinearGain_dB(int left_right,  int chan) { //chan starts counting from zero
  left_right = min(max(left_right,LEFT), RIGHT);
  chan = min(max(chan,0),N_CHAN-1);
  return expCompLim[left_right][chan].getGain_dB();
}
void printGainSettings(void) {
  myTympan.print("Gain (dB): ");
  myTympan.print("Knob = "); myTympan.print(vol_knob_gain_dB, 1);
  myTympan.print(", PGA = "); myTympan.print(input_gain_dB, 1);
  myTympan.print(", Chan = ");
  for (int i = 0; i < N_CHAN; i++) {
    myTympan.print(expCompLim[LEFT][i].getGain_dB() - vol_knob_gain_dB, 1);
    myTympan.print(", ");
  }
  myTympan.println();
}


// ////////////// Change settings of system

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.inputGain_dB + increment_dB);
}
float setInputGain_dB(float gain_dB) { 
  myState.inputGain_dB = myTympan.setInputGain_dB(gain_dB);   //set the AIC on the main Tympan board
  earpieceShield.setInputGain_dB(gain_dB);  //set the AIC on the Earpiece Sheild
  return myState.inputGain_dB;
}

extern void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB + increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
  float prev_vol_knob_gain_dB = vol_knob_gain_dB;
  vol_knob_gain_dB = gain_dB;
  float linear_gain_dB;
  for (int i = 0; i < N_CHAN_MAX; i++) {
    for (int Iear=0; Iear <= N_EARPIECES; Iear++) {
      linear_gain_dB = vol_knob_gain_dB + (expCompLim[Iear][i].getGain_dB() - prev_vol_knob_gain_dB);
      //expCompLim[Iear][i].setGain_dB(linear_gain_dB); // simple and direct but doesn't seem to maintain state correctly
      myState.wdrc_perBand.tkgain[i] = linear_gain_dB;  //but, we need to maintain the state
      expCompLim[Iear][i].setGain_dB(myState.wdrc_perBand.tkgain[i]);  //but, we need to maintain the state
    }
  }
  //myState.printPerBandSettings(); //debugging!
  printGainSettings();
}


void serviceMicDetect(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  static unsigned int prev_val = 1111; //some sort of weird value
  unsigned int cur_val = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    cur_val = myTympan.updateInputBasedOnMicDetect(); //if mic is plugged in, defaults to TYMPAN_INPUT_JACK_AS_MIC
    if (cur_val != prev_val) {
      if (cur_val) {
        myTympan.println("serviceMicDetect: detected plug-in microphone!  External mic now active.");
      } else {
        myTympan.println("serviceMicDetect: detected removal of plug-in microphone. On-board PCB mics now active.");
      }
    }
    prev_val = cur_val;
    lastUpdate_millis = curTime_millis;
  }
}

float aveSignalLevels_dBFS[2][N_CHAN_MAX]; //left ear and right ear, one for each frequency band
void updateAveSignalLevels(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 100; //how often to perform the averaging
  static unsigned long lastUpdate_millis = 0;
  float update_coeff = 0.2;

  //is it time to update the calculations
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    for (int Iear=0; Iear <= N_EARPIECES; Iear++) {
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
    printAveSignalLevelsMessage(as_dBSPL);
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}
void printAveSignalLevelsMessage(bool as_dBSPL) {
  float offset_dB = 0.0f;
  String units_txt = String("dBFS");
  if (as_dBSPL) {
    offset_dB = overall_cal_dBSPL_at0dBFS;
    units_txt = String("est dBSPL");
  }
  myTympan.print("Ave Input Level ("); myTympan.print(units_txt); myTympan.print("), Per-Band = ");
  for (int i = 0; i < N_CHAN; i++) {
    myTympan.print(aveSignalLevels_dBFS[LEFT][i] + offset_dB, 1);
    myTympan.print(", ");
  }
  myTympan.println();
}

void printPlottableData(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static int counter=0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    counter++;
    if (counter > 100) counter=0; //let's have the counter stay between 0 and 100

    if (0) {
      printData(&myTympan,true,counter);
    } else {
      printData(&Serial,false,counter); //print to USB without leading char (for Arduino Serial Plotter)
      printData(myTympan.getBTSerial(),true,counter); //print to USB without leading char (for Arduino Serial Plotter)
    }

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  } // end if
} //end printPlottableData

void printData(Print *s, bool printLeadingChar, int counter) {
  if (printLeadingChar)  s->print("P");          //Let's assume that all plottable data starts with a "P"
  #if 0
    s->print(counter);      //Let's plot a counter
    s->print(",");
    s->print(audio_settings.processorUsage(), 1);  //let's plot the CPU being used (should be 0-100)
    s->print(",");
    s->print(aveSignalLevels_dBFS[LEFT][0] + overall_cal_dBSPL_at0dBFS, 1); //let's plot the ave signal level for the first channel
    s->println();
  #else
    s->println(-2);s->println(2); //put a blip in the data so that you know when new data is being sent
    feedbackCancel.printEstimatedFeedbackImpulseResponse(s,true);  //the true puts a line feed after each datapoint
  #endif
}

void printCompressorSettings(void) {
  for (int Iear = 0; Iear < N_EARPIECES; Iear++) { //loop over channels
    Serial.print("Per-Band 0, Ear = "); Serial.println(Iear);
    Serial.print("  Scale factor (max_dB): "); Serial.println(expCompLim[Iear][0].getMaxdB());
    Serial.print("  Expansion Knee (dB): "); Serial.println(expCompLim[Iear][0].getKneeExpansion_dBSPL());
    Serial.print("  Expansion Ratio: "); Serial.println(expCompLim[Iear][0].getExpansionCompRatio());
    Serial.print("  Linear Gain (dB): "); Serial.println(expCompLim[Iear][0].getGain_dB());
    Serial.print("  Comp Knee (dB): "); Serial.println(expCompLim[Iear][0].getKneeCompressor_dBSPL());
    Serial.print("  Comp Ratio: "); Serial.println(expCompLim[Iear][0].getCompRatio());
    Serial.print("  Limiter Knee (dB): "); Serial.println(expCompLim[Iear][0].getKneeLimiter_dBSPL());
    
    Serial.print("Broadband, Ear = "); Serial.println(Iear);
    Serial.print("  Scale factor (max_dB): "); Serial.println(compBroadband[Iear].getMaxdB());
    Serial.print("  Expansion Knee (dB): "); Serial.println(compBroadband[Iear].getKneeExpansion_dBSPL());
    Serial.print("  Expansion Ratio: "); Serial.println(compBroadband[Iear].getExpansionCompRatio());
    Serial.print("  Linear Gain (dB): "); Serial.println(compBroadband[Iear].getGain_dB());
    Serial.print("  Comp Knee (dB): "); Serial.println(compBroadband[Iear].getKneeCompressor_dBSPL());
    Serial.print("  Comp Ratio: "); Serial.println(compBroadband[Iear].getCompRatio());
    Serial.print("  Limiter Knee (dB): "); Serial.println(compBroadband[Iear].getKneeLimiter_dBSPL());
  }
}

// // I don't think that this is really needed to receive audio from the BT Module...WEA 2019-11-27
//
// #include <SparkFunbc127.h>  //from https://github.com/sparkfun/SparkFun_BC127_Bluetooth_Module_Arduino_Library
// BC127 BTModu(&Serial1);  //Bluetooth module
//
//void serviceBTAudio(void) {
//    // resetFlag tracks the state of the module. When true, the module is reset and
//  //  ready to receive a connection. When false, the module either has an active
//  //  connection or has lost its connection; checking connectionState() will verify
//  //  which of those conditions we're in.
//  static boolean resetFlag = false;
//
//  // This is where we determine the state of the module. If resetFlag is false, and
//  //  we have a CONNECT_ERROR, we need to restart the module to clear its pairing
//  //  list so it can accept another connection.
//  if (BTModu.connectionState() == BC127::CONNECT_ERROR  && !resetFlag)
//  {  
//    Serial.println("Connection lost! Resetting...");
//    // Blast the existing settings of the BC127 module, so I know that the module is
//    //  set to factory defaults...
//    BTModu.restore();
//
//    // ...but, before I restart, I need to set the device to be a SINK, so it will
//    //  enable its audio output and dump the audio to the output.
//    BTModu.setClassicSink();
//
//    // Write, reset, to commit and effect the change to a source.
//    BTModu.writeConfig();
//    // Repeat the connection process if we've lost our connection.
//    BTModu.reset();
//    // Change the resetFlag, so we know we've restored the module to a usable state.
//    resetFlag = true;
//  }
//  // If we ARE connected, we'll issue the "PLAY" command. Note that issuing this when
//  //  we are already playing doesn't hurt anything.
//  else {
//    //BTModu.musicCommands(BC127::PLAY);
//  }
//
//  // We want to look for a connection to be made to the module; once a connection
//  //  has been made, we can clear the resetFlag.
//  if (BTModu.connectionState() == BC127::SUCCESS) resetFlag = false;
//
//}
