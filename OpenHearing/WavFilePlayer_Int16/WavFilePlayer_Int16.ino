// Simple WAV file player example


#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Tympan_Library.h> 

// ************************************************************************************
// NOTE: We're using the **TEENSY** (not Tympan) audio library for the audo objects.
//       The Teensy audio library differs in that the data type is Int16 (not Float32)
//       and the sample rate and block size are fixed at 44.1kHz and 128 points
// ************************************************************************************

//set the sample rate and block size
const float sample_rate_Hz = AUDIO_SAMPLE_RATE ;       // If using the Teensy audio library, this must be Teeny's definition of "AUDIO_SAMPLE_RATE"
const int audio_block_samples = AUDIO_BLOCK_SAMPLES;   // If using the Teensy audio library, this must be Teeny's definition of "AUDIO_BLOCK_SIZE"
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples); //this is mainly ignored


// /////////// Define audio objects

// define classes to control the Tympan and the Earpiece shield
Tympan                   myTympan(TympanRev::D);      //do TympanRev::D or TympanRev::E

//create audio library objects for handling the audio
AudioPlaySdWav           playWav1;
AudioOutputI2S           audioOutput;

//Make the Audio connections
AudioConnection          patchCord1(playWav1, 0, audioOutput, 0);
AudioConnection          patchCord2(playWav1, 1, audioOutput, 1);

// Use these with the Teensy 3.5 & 3.6 & 4.1 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used


// /////////// Create classes for controlling the system, espcially via USB Serial and via the App
#include      "SerialManager.h"
#include      "State.h"                
BLE_UI        ble(&myTympan);           //create bluetooth BLE class
SerialManager serialManager(&ble);     //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

// Create the MTP servicing stuff so that one can access the SD card via USB
// If you want this, be sure to set the USB mode via the Arduino IDE,  Tools Menu -> USB Type -> Serial + MTP (experimental)
#include "setup_MTP.h"  //put this line sometime after the audioSDWriter has been instantiated

// //connect some software modules to the serial manager for automatic GUI generation for the phone
// void setupSerialManager(void) {
//   //register all the UI elements here
//   //serialManager.add_UI_element(&myState);
//   //serialManager.add_UI_element(&ble);
//   serialManager.add_UI_element(&audioSDWriter);
// }


void setup() {
  myTympan.beginBothSerial(); delay(1500);
  Serial.println("WavFilePlayer_Int16: setup():...");
  Serial.println("Sample Rate (Hz): " + String(sample_rate_Hz));
  Serial.println("Audio Block Size (samples): " + String(audio_block_samples));

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(32);

  //start the audio hardware
  myTympan.enable();
  myTympan.volume_dB(0.0);

  //start the SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    // stop here, but print a message repetitively
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }

  //setup BLE
  //delay(250); ble.setupBLE(myTympan.getBTFirmwareRev()); delay(250); //Assumes the default Bluetooth firmware. You can override!
  
  //setup the serial manager
  //setupSerialManager();

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();
}

void loop() {
  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  // //respond to BLE
  // if (ble.available() > 0) {
  //   String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
  //   for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);
  // }


 // Did the user activate MTP mode?  If so, service the MTP and nothing else
  if (use_MTP) {   //service MTP (ie, the SD card appearing as a drive on your PC/Mac
     
     service_MTP();  //Find in Setup_MTP.h 
  
  } else { //do everything else!

    // //service the BLE advertising state
    // if (audioSDWriter.getState() != AudioSDWriter::STATE::RECORDING) {
    //   ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)
    // }

    //service the LEDs...blink slow normally, blink fast if recording
    bool blinkFaster = playWav1.isPlaying();
    myTympan.serviceLEDs(millis(),blinkFaster); 

    //be nice?
    delay(1);
  }
}



// //////////////////////////////////// Control the audio from the SerialManager

bool playFile(const char *filename) {

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playWav1.play(filename);

  // A brief delay for the library read WAV info
  delay(25);

  // Simply wait for the file to finish playing.
  bool success = playWav1.isPlaying();
  bool failure = !success;
  return failure;
}

void playSdWavFile(int index) {
  String fname = String(""); //ensure an empty String

  switch (index) {
    case 1:
      fname = String("TONE_125Hz_-20dBFS.WAV");
      break;
    case 2:
      fname = String("TONE_250Hz_-37dBFS.WAV");
      break;
    case 3:
      fname = String("TONE_500Hz_-45dBFS.WAV");
      break;
    case 4:
      fname = String("TONE_1000Hz_-48dBFS.WAV");
      break;
    case 5:
      fname = String("TONE_2000Hz_-44dBFS.WAV");
      break;
  }

  if (fname.length() > 0) {
    Serial.println("playSdWavFile: attempting to play " + fname);
    bool failure = playFile(fname.c_str());
    if (failure) {
      Serial.println("playSdWavFile: *** ERROR ***: failed to play " + fname);
    }
  } else {
    Serial.println("playSdWavFile: *** ERROR ***: did not recognize command " + String(index));
    Serial.println("    : command must be 1-5.");
  }
}

void stopSdWav(void) {
  Serial.println("stopSdWav: stopping WAV...");
  playWav1.stop();
}





