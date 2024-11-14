/*
   DigMicProbeTest
   
   Created: Chip Audette, OpenHearing, Feb 2024
   Purpose: Enable MPU to test the initial earprobe prototype using 4 PDM mics
      * Typmpan with Earpiece Shield
      * Play test signals:
          * Option 1: Generate a logarithmic sweep
          * Opiton 2: Play back an arbitrary signal from the SD card
      * Record audio from all four digital mics to SD card
      * Access SD card via "MTP Disk" mode

  Switches:
      * To enable 4-channel audio, #define EN_4_SD_CHAN 1.  For 2-channel audio, comment this line out.
        - In 2-channel mode, the inputs are recorded to SD, and audio mixer (SD and Chirp) is sent to the output
      * To enable BLE (untested), #define EN_BLE 1.  Othewise, comment out.  May affect robust recordings. 

  Serial Commands
      * See help by sending 'h'.  Note that new hybrid input ('o') sets INPUT_MIC_JACK_WTIH_PDM_MIC
        - This sets the tympan to Mic Jack with Bias, and the shield to PDM Mics.

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>
#include "SerialManager.h"
#include "State.h"       

/* Create the MTP servicing stuff so that one can access the SD card via USB */
/* If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental) */
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated

// To enable 4-channel audio recordings, #define EN_4_SD_CHAN 1; otherwise for 2-channel, comment out.
#define EN_4_SD_CHAN 1

// To enable bluetooth, uncomment:
#define EN_BLE 1

//set the sample rate and block size
const float sample_rate_Hz = 96000.0f ;  //Desired sample rate
const int audio_block_samples = 128;     //Number of samples per audio block (do not make bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects...they are configured later
// define classes to control the Tympan and the Earpiece shield
Tympan myTympan(TympanRev::F, audio_settings);  //do TympanRev::D or TympanRev::E or TympanRev::F
#ifdef EN_4_SD_CHAN
  EarpieceShield earpieceShield(TympanRev::F, AICShieldRev::A, audio_settings);  //Note that EarpieceShield is defined in the Tympan_Libarary in AICShield.h 
#endif

// define classes to control serial communication and SD card 
SdFs                        sd;                             //This is the SD card.  SdFs is part of the Teensy install
SdFileTransfer              sdFileTransfer(&sd, &Serial);   //Transfers raw bytes of files from Serial to the SD card.  Part of Tympan_Library

//create audio library objects for handling the audio
AudioInputI2SQuad_F32      i2s_in(audio_settings);         //Bring audio in
AudioSynthToneSweepExp_F32 chirp(audio_settings);          //from the Tympan_Library (synth_tonesweep_F32.h)
AudioSDPlayer_F32          sdPlayer(&sd, audio_settings);  //from the Tympan_Library
AudioMixer4_F32            audioMixerL(audio_settings);    //for mixing outputs from SD player and Chirp
AudioMixer4_F32            audioMixerR(audio_settings);    //for mixing outputs from SD player and Chirp
AudioCalcLeq_F32           calcInputLevel_L(audio_settings); //use this to measure the input signal level
AudioCalcLeq_F32           calcInputLevel_R(audio_settings); //use this to measure the input signal level
AudioOutputI2SQuad_F32     i2s_out(audio_settings);        //Send audio out
AudioSDWriter_F32_UI       audioSDWriter(&sd, audio_settings);  //Write audio to the SD card (if activated)

//Connect the signal sources to the audio mixer
AudioConnection_F32        patchcord1(chirp,    0, audioMixerL, 0);   //chirp is mono, so simply send to left channel mixer
AudioConnection_F32        patchcord2(sdPlayer, 0, audioMixerL, 1);   //left channel of SD to left channel mixer
AudioConnection_F32        patchcord3(chirp,    0, audioMixerR, 0);   //chirp is mono, so simply send it to right channel mixer too
AudioConnection_F32        patchcord4(sdPlayer, 1, audioMixerR, 1);   //right channel of SD to right channel mixer

//If 4 channel audio... 
#ifdef EN_4_SD_CHAN
//Connect Tympan Inputs to Tympan outputs (black headphone jack)
AudioConnection_F32        patchcord11(i2s_in, 1, i2s_out, 0);    //Left input to left output
AudioConnection_F32        patchcord12(i2s_in, 0, i2s_out, 1);    //Right input to right output

//Connect audio mixer (containing SD play and chirp) to the output on the Earpiece Shield (both earpiece ports and black headphone jack)
AudioConnection_F32        patchcord21(audioMixerL, 0, i2s_out, 2);  //connect chirp to left output on earpiece shield
AudioConnection_F32        patchcord22(audioMixerR, 0, i2s_out, 3);  //connect chirp to right output on earpiece shield

//ELSE 2-channel audio
#else
//Connect audio mixer (containing SD play and chirp) to the output on the Tympan
  AudioConnection_F32       patchcord21(audioMixerL, 0, i2s_out, 0);
 AudioConnection_F32        patchcord22(audioMixerR, 0, i2s_out, 1);
#endif

//Connect Tympan Inputs to SD recording
AudioConnection_F32         patchcord31(i2s_in, 1, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32         patchcord32(i2s_in, 0, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer

//IF 4-Channel Audio, connect Earpiece Shield inputs to SD recorder
#ifdef EN_4_SD_CHAN
  AudioConnection_F32       patchcord33(i2s_in, 3, audioSDWriter, 2);   //connect Raw audio to left channel of SD writer
  AudioConnection_F32       patchcord34(i2s_in, 2, audioSDWriter, 3);   //connect Raw audio to right channel of SD writer
#endif

//Connect Tympan Left and Right Input to level meter
AudioConnection_F32        patchcord41(i2s_in, 0, calcInputLevel_L, 0);    //Left input to the level monitor
AudioConnection_F32        patchcord42(i2s_in, 1, calcInputLevel_R, 0);    //Left input to the level monitor


// create BLE object, if enabled
#ifdef EN_BLE
BLE_UI          &ble = myTympan.getBLE_UI();                         //myTympan owns the ble object, but we have a reference to it here
#endif
SerialManager   serialManager(&ble);                          //create the serial manager for real-time control
State           myState(&audio_settings, &myTympan, &serialManager);  //keeping one's state is useful for the App's GUI


//~~~~Main setup()~~~~ 
void setup() {

  // Begin Bluetooth and serial comms
  #ifdef EN_BLE
  myTympan.beginBothSerial(); //should use the correct Serial port and the correct baud rate
  #endif
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
  Serial.print(CrashReport);
  Serial.println("DigMicProbeTest: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); 
  #ifdef EN_4_SD_CHAN
    earpieceShield.enable();
  #endif

  //Select the input that we will use 
  setConfiguration(myState.input_source);  //the initial value is set in State.h
  
  //Configure the mixer for the audio sources
  audioMixerL.mute(); audioMixerR.mute();  //set all channels to zero (I think that this is defualt, but let's do this just to be sure!)
  audioMixerL.gain(0,1.0);  audioMixerL.gain(1,1.0);  //mix together the chirp and SD player (though prob only one will be playing at a time)
  audioMixerR.gain(0,1.0);  audioMixerR.gain(1,1.0);  //mix together the chirp and SD player (though prob only one will be playing at a time)

  //Set window for calculating sound level
  calcInputLevel_L.setTimeWindow_sec(0.125);  //average over 0.125 seconds
  calcInputLevel_R.setTimeWindow_sec(0.125);  //average over 0.125 seconds

  //Select the gain for the output DAC
  setOutputGain_dB(myState.output_gain_dB);

// IF BLE enabled, set it up
#ifdef EN_BLE
  myTympan.setupBLE(); delay(500);

  // Add preset UI elements
  serialManager.add_UI_element(&audioSDWriter);
#endif

  //FOR DEBUGGING ONLY 
  #if 0
  //Debug my software zeroing signals going to right channel
  Serial.println("Zeroing mixer gains for right channel...");
  audioMixerR.gain(0,0.0);  audioMixerR.gain(1,0.0);  //mix together the chirp and SD player (though prob only one will be playing at a time)

  //debug by muting one side of the DAC to test for audio leakage
  Serial.println("Muting Right DAC...");
  myTympan.muteDAC(RIGHT_CHAN); 
  #ifdef EN_4_SD_CHAN
    earpieceShield.muteDAC(RIGHT_CHAN); 
  #endif

  //debug by muting one side of the Headphone Driver to test for audio leakage
  delay(1000); //let stuff settle
  Serial.println("Muting Right Headphone Driver...");
  myTympan.muteHeadphone(RIGHT_CHAN); 
  #ifdef EN_4_SD_CHAN
    earpieceShield.muteHeadphone(RIGHT_CHAN);
  #endif //EN_4_SD_CHAN
  #endif

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  
  // Increase the RAM buffer allocated to audio SD writer.
  if (audioSDWriter.allocateBuffer(300000) < 512) {       //Must be greater than 512 bytes to trigger SD writing
    Serial.println("setup: *** ERROR ***: SD Could Not Allocate RAM Buffer!");  
  }

  // Set # of recording channels
  #ifdef EN_4_SD_CHAN
    audioSDWriter.setNumWriteChannels(4);
  #else
    audioSDWriter.setNumWriteChannels(2);       
  #endif
  Serial.println("SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

//Set the state of the LEDs
  myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW);

  Serial.println("Setup: complete."); 
  serialManager.printHelp();
} //end setup()



//~~~~Main loop()~~~~ 
void loop() {
  //respond to Serial commands
  while (Serial.available()) {
    serialManager.respondToByte( (char)Serial.read() ); 
  }

#if EN_BLE
  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
#endif

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //service the automatic SD starting and stopping based on the playing of the chirp / SD
  serviceAutoSdStartStop();
 
  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!
    //service the LEDs...blink slow normally, blink fast if recording
    myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

    //service the SD player to refill the play buffer
    sdPlayer.serviceSD();
  }
 
  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

  //periodically print the input signal levels
  if (myState.flag_PrintInputLevel) printInputSignalLevels(millis(),500);  //print every 500 msec

} //end loop()


// /////////// Functions for configuring the system
float setInputGain_dB(float val_dB) {
  myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
  #ifdef EN_4_SD_CHAN
    myState.input_gain_dB = earpieceShield.setInputGain_dB(myState.input_gain_dB);
  #endif
  return myState.input_gain_dB;
}

void setConfiguration(int config) {
  myState.input_source = config;
  const float default_mic_gain_dB = 0.0f;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); 
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      #ifdef EN_4_SD_CHAN
        earpieceShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      #endif
      setInputGain_dB(default_mic_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); 
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels

      #ifdef EN_4_SD_CHAN
        earpieceShield.enableDigitalMicInputs(false);
        earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the on-board 
        earpieceShield.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      #endif
      setInputGain_dB(default_mic_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); 
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes

      #ifdef EN_4_SD_CHAN
        earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
        setInputGain_dB(0.0);
      #endif
      break;

    case State::INPUT_PDM_MICS:
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      myTympan.enableDigitalMicInputs(true);

      #ifdef EN_4_SD_CHAN
        earpieceShield.enableDigitalMicInputs(true);
      #endif
      setInputGain_dB(0.0);
      break;

    // Use MIC_JACK on Tympan main board, and digital mics on Earpiece Shield
    case State::INPUT_MIC_JACK_WTIH_PDM_MIC:
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the line-input through holes
      #ifdef EN_4_SD_CHAN
        earpieceShield.enableDigitalMicInputs(true);
      #endif
      setInputGain_dB(0.0);
      break;
  }
}


int playChirp(void) {
  chirp.play( sqrtf(powf(10.0f, 0.1f*myState.chirp_amp_dBFS)),
              myState.chirp_start_Hz,
              myState.chirp_end_Hz,
              myState.chirp_dur_sec
            );
  return 0;
}

int playSD(int file_ind) {
  //check the inputs
  if (file_ind < 0) {
    Serial.println("playSD: *** ERROR ***: file_ind is " + String(file_ind) + ", which cannot be negative.");
    Serial.println("    : ignoring and returning.");
    return -1;
  }

  //compose the filename
  String fname = "PLAY";
  fname += String(file_ind);
  fname += ".WAV";

  //confirm that it exists
  // if (!sd.exists(fname)) {
  //   Serial.println("playSD: *** ERROR ***: file " + fname + " does not exist on SD.");
  //   Serial.println("    : ignoring and returning...");
  //   return -1;
  // }

  //play the file
  sdPlayer.play(fname);
  return 0;
}

void serviceAutoSdStartStop(void) {
  static unsigned long nextUpdate_millis = 0UL;
  static unsigned long updatePeriod_millis = 1000UL;

  // Is it time to start a signal?
  if ((myState.signal_start_armed==true) || (myState.auto_sd_state == State::WAIT_START_SIGNAL)) {
    if (millis() >= myState.start_signal_at_millis) {
      int signal_type = myState.autoPlay_signal_type;
      switch(signal_type) {
        case State::PLAY_CHIRP:
          Serial.println("serviceAudioSdStartStop: Starting chirp...");
          playChirp();
          break;
        default:
          Serial.println("serviceAudioSdStartStop: Starting SD " + String(signal_type) + " playback...");
          playSD(signal_type);
          break;
      }

      //regardless of which signal type, finish off the actions
      myState.signal_start_armed = false;
      if (myState.auto_sd_state == State::WAIT_START_SIGNAL)  myState.auto_sd_state = State::WAIT_END_SIGNAL;
      myState.has_signal_been_playing = true;     //set this flag
      nextUpdate_millis = millis()+updatePeriod_millis; //set this static to enable future timed messages
    }
  }

  //Is the signal playing
  if (chirp.isPlaying() || sdPlayer.isPlaying()) {

    //is it time to print a status message to the serial monitor?
    if (millis() >= nextUpdate_millis) {
      Serial.println("serviceChirpStartStop: Chirp or SD is still playing...");
      nextUpdate_millis += updatePeriod_millis;
    }

  } else { //or has the chirp stopped
    //chirp is not playing
    if (myState.has_signal_been_playing == true) {
      //signal just stopped playing
      Serial.println("serviceChirpStartStop: Chirp or SD has finished.");

      if (myState.auto_sd_state == State::WAIT_END_SIGNAL) {
        myState.auto_sd_state = State::WAIT_STOP_SD;
        Serial.println("Auto-stopping SD in " + String(1000*myState.auto_SD_start_stop_delay_sec,0) + " msec");
        myState.stop_SD_at_millis = millis()+ (unsigned int)round(max(0.0,1000.0*myState.auto_SD_start_stop_delay_sec));
      }
    }
    myState.has_signal_been_playing = false; //clear this flag
  }

  //is it time to stop the SD?
  if (myState.auto_sd_state == State::WAIT_STOP_SD) {
    if (millis() >= myState.stop_SD_at_millis) {
      Serial.println("Auto-stopping SD recording of " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      myState.auto_sd_state = State::DISABLED;
    }
  }
}

// Print Tympan right and level input level in dBFS
void printInputSignalLevels(unsigned long cur_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0UL;

  if ( (cur_millis < lastUpdate_millis) || (cur_millis >= lastUpdate_millis + updatePeriod_millis) ) {
    Serial.print( "Input level: (" + String(calcInputLevel_L.getCurrentLevel_dB(),2) + ", " );
    Serial.println( String(calcInputLevel_R.getCurrentLevel_dB(),2) + ") dBFS");
		//setButtonText("cpuValue", String(getCPUUsage(),1));
    
    //reset timer
    lastUpdate_millis = cur_millis;    
  }
}


// //////////////////////////////////// Control the audio processing from the SerialManager

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}
float incrementOutputGain_dB(float increment_dB) {
  return setOutputGain_dB(myState.output_gain_dB + increment_dB);
}
float setOutputGain_dB(float gain_dB) {
  myState.output_gain_dB = myTympan.volume_dB(gain_dB);
  #ifdef EN_4_SD_CHAN
    myState.output_gain_dB = earpieceShield.volume_dB(gain_dB);
  #endif
  return myState.output_gain_dB;
}

float incrementChirpLoudness(float incr_dB) {
  return myState.chirp_amp_dBFS = min(0.0, myState.chirp_amp_dBFS+incr_dB);
}
float incrementChirpDuration(float incr_sec) {
  return myState.chirp_dur_sec = max(1.0, myState.chirp_dur_sec+incr_sec);
}
// void startChirpWithDelay(float delay_sec) {
//   myState.autoPlay_signal_type = State::PLAY_CHIRP;
//   startSignalWithDelay(delay_sec);
// }
void startSignalWithDelay(float delay_sec) {
  myState.signal_start_armed = true;
  myState.start_signal_at_millis = millis()+ (unsigned int)round(max(0.0,1000.0*delay_sec));
}
void forceStopSDPlay(void) {
  sdPlayer.stop();
}