/*
   DigMicProbeTest
   
   Created: Chip Audette, OpenHearing, Feb 2024
   Purpose: Enable MPU to test the initial earprobe prototype using 4 PDM mics
      * Typmpan with Earpiece Shield
      * Generate a logarithmic sweep
      * Record audio from all four digital mics to SD card
      * Access SD card via "MTP Disk" mode

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

//set the sample rate and block size
const float sample_rate_Hz = 96000.0f ;  //Desired sample rate
const int audio_block_samples = 128;     //Number of samples per audio block (do not make bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects...they are configured later

// define classes to control the Tympan and the Earpiece shield
Tympan                        myTympan(TympanRev::E);   //do TympanRev::D or TympanRev::E
EarpieceShield   earpieceShield(TympanRev::E, AICShieldRev::A);  //Note that EarpieceShield is defined in the Tympan_Libarary in AICShield.h 

//create audio library objects for handling the audio
AudioInputI2SQuad_F32      i2s_in(audio_settings);         //Bring audio in
//AudioSynthWaveform_F32    sineWave(audio_settings);   //from the Tympan_Library (synth_waveform_f32.h>
AudioSynthToneSweepExp_F32 chirp(audio_settings);       //from the Tympan_Library (synth_tonesweep_F32.h)
AudioSynthWaveform_F32     sineWave(audio_settings);     //from the Tympan_Library
AudioOutputI2SQuad_F32     i2s_out(audio_settings);        //Send audio out
AudioSDWriter_F32_UI       audioSDWriter(audio_settings);  //Write audio to the SD card (if activated)

//Connect inputs to outputs...let's just connect the first two mics to the Tympan's black headphone jack
AudioConnection_F32           patchcord1(i2s_in, 1, i2s_out, 0);    //Left input to left output
AudioConnection_F32           patchcord2(i2s_in, 0, i2s_out, 1);    //Right input to right output

//Connect chirp to the earpieces and to the black jack on the Earpiece shield
AudioConnection_F32           patchcord11(chirp, 0, i2s_out, 2);  //connect chirp to left output on earpiece shield
AudioConnection_F32           patchcord12(chirp, 0, i2s_out, 3);  //connect chirp to riht output on earpiece shield

//Connect all four mics to SD logging
AudioConnection_F32           patchcord21(i2s_in, 1, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord22(i2s_in, 0, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer
AudioConnection_F32           patchcord23(i2s_in, 3, audioSDWriter, 2);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord24(i2s_in, 2, audioSDWriter, 3);   //connect Raw audio to right channel of SD writer


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
#include      "SerialManager.h"
#include      "State.h"                
BLE_UI        ble(&myTympan);           //create bluetooth BLE class
SerialManager serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

/* Create the MTP servicing stuff so that one can access the SD card via USB */
/* If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental) */
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated


//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  //serialManager.add_UI_element(&myState);
  //serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&audioSDWriter);
}


// /////////// Functions for configuring the system
float setInputGain_dB(float val_dB) {
  myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
  return earpieceShield.setInputGain_dB(myState.input_gain_dB);
}

void setConfiguration(int config) {
  myState.input_source = config;
  const float default_mic_gain_dB = 0.0f;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); earpieceShield.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      earpieceShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      setInputGain_dB(default_mic_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); earpieceShield.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the on-board microphones
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      earpieceShield.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      setInputGain_dB(default_mic_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); earpieceShield.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      setInputGain_dB(0.0);
      break;

    case State::INPUT_PDM_MICS:
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      myTympan.enableDigitalMicInputs(true);
      earpieceShield.enableDigitalMicInputs(true);
      setInputGain_dB(0.0);
      break;
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1500);
  Serial.println("DigMicProbeTest: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); earpieceShield.enable();

  //Select the input that we will use 
  setConfiguration(myState.input_source);  //the initial value is set in State.h
  
  //Select the gain for the output amplifiers
  myTympan.volume_dB(myState.output_gain_dB);
  earpieceShield.volume_dB(myState.output_gain_dB);

  //setup BLE
  delay(500); ble.setupBLE(myTympan.getBTFirmwareRev()); delay(500); //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  setupSerialManager();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  Serial.println("SD configured for " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //Set the state of the LEDs
  myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW);

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  #if 0
  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  }
  #endif

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info

  //service the chirp
  serviceChirpSdStartStop();
 
  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!

    #if 0
    //service the BLE advertising state
    ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
    #endif 
     
    //service the LEDs...blink slow normally, blink fast if recording
    myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 
  
  }
 
  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)


} //end loop()

// //////////////////////////////////// Servicing routines not otherwise embedded in other classes
void serviceChirpSdStartStop(void) {
  static unsigned long nextUpdate_millis = 0UL;
  static unsigned long updatePeriod_millis = 1000UL;
  static bool has_chirp_been_playing = false;
  
  // Is it time to start a chirp?
  if ((myState.chirp_start_armed==true) || (myState.auto_chirpsd_state == State::WAIT_START_CHIRP)) {
    if (millis() >= myState.start_chirp_at_millis) {
      Serial.println("serviceChirpStartStop: Starting chirp...");
      chirp.play( sqrtf(powf(10.0f, 0.1f*myState.chirp_amp_dBFS)),
                  myState.chirp_start_Hz,
                  myState.chirp_end_Hz,
                  myState.chirp_dur_sec
                );
      myState.chirp_start_armed = false;
      if (myState.auto_chirpsd_state == State::WAIT_START_CHIRP) {
        myState.auto_chirpsd_state = State::WAIT_END_CHIRP;
      }
      has_chirp_been_playing = true;     //set this flag
      nextUpdate_millis = millis()+updatePeriod_millis; //set this static to enable future timed messages
    }
  }

  //Is the chirp playing
  if (chirp.isPlaying()){
    
    //is it time to print a status message to the serial monitor?
    if (millis() >= nextUpdate_millis) {
      Serial.println("serviceChirpStartStop: Chirp is still playing...");
      nextUpdate_millis += updatePeriod_millis;
    }
    
  } else { //or has the chirp stopped
    //chirp is not playing
    if (has_chirp_been_playing == true) {
      //chirp just stopped playing
      Serial.println("serviceChirpStartStop: Chirp has finished.");

      if (myState.auto_chirpsd_state == State::WAIT_END_CHIRP) {
        myState.auto_chirpsd_state = State::WAIT_STOP_SD;
        Serial.println("Auto-stopping SD in " + String(1000*myState.auto_SD_start_stop_delay_sec,0) + " msec");
        myState.stop_SD_at_millis = millis()+ (unsigned int)round(max(0.0,1000.0*myState.auto_SD_start_stop_delay_sec));
      }
    }
    has_chirp_been_playing = false; //clear this flag
  }

  //is it time to stop the SD?
  if (myState.auto_chirpsd_state == State::WAIT_STOP_SD) {
    if (millis() >= myState.stop_SD_at_millis) {
      Serial.println("Auto-stopping SD recording of " + audioSDWriter.getCurrentFilename());
      audioSDWriter.stopRecording();
      myState.auto_chirpsd_state = State::DISABLED;
    }
  }
}

// //////////////////////////////////// Control the audio processing from the SerialManager

//here's a function to change the volume settings.   We'll also invoke it from our serialManager
float incrementInputGain(float increment_dB) {
  return setInputGain_dB(myState.input_gain_dB + increment_dB);
}

float incrementChirpLoudness(float incr_dB) {
  return myState.chirp_amp_dBFS = min(0.0, myState.chirp_amp_dBFS+incr_dB);
}
float incrementChirpDuration(float incr_sec) {
  return myState.chirp_dur_sec = max(1.0, myState.chirp_dur_sec+incr_sec);
}
void startChirpWithDelay(float delay_sec) {
  myState.chirp_start_armed = true;
  myState.start_chirp_at_millis = millis()+ (unsigned int)round(max(0.0,1000.0*delay_sec));
}
