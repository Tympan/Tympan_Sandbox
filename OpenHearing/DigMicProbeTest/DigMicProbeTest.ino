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

   For Tympan Rev D, program in Arduino IDE as a Teensy 3.6.
   For Tympan Rev E, program in Arduino IDE as a Teensy 4.1.

   MIT License.  use at your own risk.
*/

#define USE_BLE 0  //set to zero to disable bluetooth 

// Include all the of the needed libraries
#include <Tympan_Library.h>
#include "AudioSD_logging.h"
#include      "SerialManager.h"
#include      "State.h"       

//set the sample rate and block size
const float sample_rate_Hz = 96000.0f ;  //Desired sample rate
const int audio_block_samples = 128;     //Number of samples per audio block (do not make bigger than 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);


// /////////// Define audio objects...they are configured later
// define classes to control the Tympan and the Earpiece shield
Tympan          myTympan(TympanRev::E);                         //do TympanRev::D or TympanRev::E or TympanRev::F
EarpieceShield  earpieceShield(TympanRev::E, AICShieldRev::A);  //Note that EarpieceShield is defined in the Tympan_Libarary in AICShield.h 

// define classes to control serial communication and SD card 
SdFs            sd;                             //This is the SD card.  SdFs is part of the Teensy install
SdFileTransfer  sdFileTransfer(&sd, &Serial);   //Transfers raw bytes of files from Serial to the SD card.  Part of Tympan_Library

//create audio library objects for handling the audio
#include "AudioSetup.h"


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
         
BLE_UI&       ble = myTympan.getBLE_UI();  //myTympan owns the ble object, but we have a reference to it here
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


// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  //myTympan.beginBothSerial(); //only needed if using bluetooth
  delay(500);
  Serial.println("DigMicProbeTest: setup():...");
  Serial.println("Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  //allocate the dynamically re-allocatable audio memory
  AudioMemory_F32(100, audio_settings); 

  //Enable the Tympan and AIC shields to start the audio flowing!
  myTympan.enable(); earpieceShield.enable();

  //Select the input that we will use 
  setConfiguration(myState.input_source);  //the initial value is set in State.h
  
  //Configure the mixer for the audio sources
  audioMixerL.mute(); audioMixerR.mute();  //set all channels to zero (I think that this is defualt, but let's do this just to be sure!)
  audioMixerL.gain(0,1.0);  audioMixerL.gain(1,1.0);  //mix together the chirp and SD player (though prob only one will be playing at a time)
  audioMixerR.gain(0,1.0);  audioMixerR.gain(1,1.0);  //mix together the chirp and SD player (though prob only one will be playing at a time)

  //debug my software zeroing signals going to right channel
  if (0) {
    Serial.println("Zeroing mixer gains for right channel...");
    audioMixerR.gain(0,0.0);  audioMixerR.gain(1,0.0);  //mix together the chirp and SD player (though prob only one will be playing at a time)
  }

  //Select the gain for the output DAC
  setOutputGain_dB(myState.output_gain_dB);

  //debug by muting one side of the DAC to test for audio leakage
  if (0) {
    Serial.println("Muting Right DAC...");
    myTympan.muteDAC(RIGHT_CHAN); earpieceShield.muteDAC(RIGHT_CHAN);
  }

  //debug by muting one side of the Headphone Driver to test for audio leakage
  if (0) {
    delay(1000); //let stuff settle
    Serial.println("Muting Right Headphone Driver...");
    myTympan.muteHeadphone(RIGHT_CHAN); earpieceShield.muteHeadphone(RIGHT_CHAN);
  }

  //setup BLE
  if (USE_BLE) { delay(500); myTympan.setupBLE(); delay(500); } //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  setupSerialManager();

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(NUM_SD_CHAN);             //four channels for this quad recorder, but you could set it to 2
  Serial.println("setup: SD configured to write to " + String(audioSDWriter.getNumWriteChannels()) + " channels.");

  //let's allocate some extra RAM for the read and write buffers to reduce hiccups...make sure you don't use of all the RAM (RAM2)
  int ret_val;
  ret_val = audioSDPlayer.allocateBuffer((2*2*64)*512);  //set the RAM buffer (bytes) for playing (default is 32*512).  2*2*64*512 = 131072; 131072 / (2chan*2bytes*96000) = 341 msec
  Serial.println("setup: allocated RAM for play buffer.  Size after allocation = " + String(ret_val));
  ret_val = audioSDWriter.allocateBuffer(2*(2*2*64)*512);  //set the RAM buffer (bytes) for recording. default is 150000. 4*2*64*512 = 262/144; 262144 / (4chan*2bytes*96000) = 341 msec 
  Serial.println("setup: alloacted RAM for write buffer.  Allocation return val = " + String(ret_val));
  constexpr uint32_t bytes_per_second = 4*2*96000;
  constexpr uint32_t file_preAllocation_seconds = 60;
  audioSDWriter.setSDPreAllocationSizeBytes(bytes_per_second * file_preAllocation_seconds);

\
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

  if (USE_BLE) { 
    //respond to BLE
    if (ble.available() > 0) {
      String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
      for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
    }
  }
  
  //service the SD
  service_SD_read_write();
  
  //service the automatic SD starting and stopping based on the playing of the chirp / SD
  serviceAutoSdStartStop();
 
  // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!

    if (USE_BLE) ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
     
    //service the LEDs...blink slow normally, blink fast if recording
    myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  }
 
  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)


} //end loop()

// //////////////////////////////////// Servicing routines not otherwise embedded in other classes

//For the SD cards, it needs to periodically read bytes from the SD card into the SDplayer's read buffer.
//Similarly, it needs to periodically write bytes from the SDwriter's write buffer to the SD card.
//In this function, we look to see which one is closest to running out of buffer, and we service its
//needs first.
int service_SD_read_write(void) {
  uint32_t writeBuffer_empty_msec = 9999;
  if (audioSDWriter.isFileOpen()) writeBuffer_empty_msec = audioSDWriter.getNumUnfilledSamplesInBuffer_msec();

  uint32_t playBuffer_full_msec = 9999;
  if (audioSDPlayer.isFileOpen()) playBuffer_full_msec = audioSDPlayer.getNumBytesInBuffer_msec();

  if (playBuffer_full_msec < writeBuffer_empty_msec) {
    //service the player first as its buffer is closest to being out
    audioSDPlayer.serviceSD();   
    audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
  } else { 
    //service the writer first, as its buffer is closest to being out
    audioSDWriter.serviceSD_withWarnings(i2s_in); //For the warnings, it asks the i2s_in class for some info
    audioSDPlayer.serviceSD();   
  }
  return 0;
}

int playChirp(void) {
  chirp.play( sqrtf(powf(10.0f, 0.1f*myState.chirp_amp_dBFS)),
              myState.chirp_start_Hz,
              myState.chirp_end_Hz,
              myState.chirp_dur_sec
            );
  return 0;
}

int openPlayFileOnSD(int file_ind) {
  Serial.println("openPlayFileOnSD: file_ind = " + String(file_ind));
  //check the inputs
  if (file_ind < 0) {
    Serial.println("openPlaySD: *** ERROR ***: file_ind is " + String(file_ind) + ", which cannot be negative.");
    Serial.println("    : ignoring and returning.");
    return -1;
  }

  Serial.println("openPlayFileOnSD: play_file_state = " + String(myState.play_file_state));
  if (myState.play_file_state != State::PLAY_FILE_CLOSED) {
    Serial.println("openPlayFileOnSD: stopping the player...");
    audioSDPlayer.stop();
    myState.play_file_state = State::PLAY_FILE_CLOSED;
  } 

  //compose the filename
  String fname = "PLAY";
  fname += String(file_ind);
  fname += ".WAV";
  Serial.println("openPlayFileOnSD: fname = " + String(fname));

  //confirm that it exists
  if (!sd.exists(fname)) {
    Serial.println("openPlayFileOnSD: *** ERROR ***: file " + fname + " does not exist on SD.");
    Serial.println("    : ignoring and returning...");
    return -1;
  }

  //open the file
  Serial.println("openPlayFileOnSD: opening " + fname);
  audioSDPlayer.resetLog();
  bool success = audioSDPlayer.open(fname);
  Serial.println("openPlayFileOnSD: success = " + String(success));
  if (success == myState.play_file_state) {
    myState.play_file_state = State::PLAY_FILE_OPEN;  
  }


  return success;
}

//play a file that has already been opened
int playSD(void) { return playSD(-1); } //assumes that the file has already been opened, manually

//open a file and play it
int playSD(int file_ind) {  

  if (myState.play_file_state == State::PLAY_FILE_CLOSED) {

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
    audioSDPlayer.resetLog();
    bool success = audioSDPlayer.play(fname);
    if (success == myState.play_file_state) myState.play_file_state = State::PLAY_FILE_OPEN;

  } else {
    //play file that has already been opened
    Serial.println("playSD: starting to play already-opened file...");
    audioSDPlayer.play();
  }
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
  if (chirp.isPlaying() || audioSDPlayer.isPlaying()) {

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
      myState.play_file_state = State::PLAY_FILE_CLOSED;
      Serial.println("serviceAudioSdStartStop: wrote log file to SD: " + audioSDPlayer.writeLogToSD()); 

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
      Serial.println("serviceAudioSdStartStop: wrote log file to SD: " + audioSDWriter.writeLogToSD()); 
      myState.auto_sd_state = State::DISABLED;
      myState.rec_file_state = State::REC_FILE_CLOSED;
    }
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
  return myState.output_gain_dB = earpieceShield.volume_dB(myTympan.volume_dB(gain_dB));
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
  audioSDPlayer.stop();
  myState.play_file_state = State::REC_FILE_CLOSED;
}
