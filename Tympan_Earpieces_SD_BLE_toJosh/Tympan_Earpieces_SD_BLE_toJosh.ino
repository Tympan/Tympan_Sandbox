// Tympan_Earpieces
//
// Created: Chip Audette, Open Audio, Dec 2019
// Updated: Chip Audette, Apr 2021 for BLE
//
// This example code is in the public domain.
//
// Hardware: Assumes that you're using a Tympan RevD with an Earpiece_Shield
//    and two Tympan earpieces, each having its two PDM micrphones.
//
//  According to the schematic for the Tympan Earpiece_Shield, the two PDM
//    mics in the left earpiece go to the first AIC (ie, I2S channels 0 and 1)
//    while the two PDM mics in the right earpiece go to the second AIC (which
//    are I2S channels 2 and 3).  In this sketch, I will use a mixer to allow you
//    to mix the two mics from each earpiece, or just use the first mic in each
//    earpiece.
//
//  Also, according to the schematic, the speakers in the two earpieces are
//    driven by the second AIC, so they would be I2S channel 2 (left) and 3 (right).
//    In this sketch, I will copy the audio so that it goes to the headphone outputs
//    of both AICs, so you can listen through regular headphones on the first AIC
//    or through the Tympan earpieces (or headphones) on the second AIC.
//
// This sketch works with the TympanRemote App on the Play Store, BLE Version
//   https://play.google.com/store/apps/details?id=com.creare.tympanRemote&hl=en_US
//


#include <Tympan_Library.h>
#include "State.h"
#include "SerialManager.h"
#include <SD.h>  


#define USE_MTP true  //set to true to enable "MTP", which is USB access to the SD card when plugged into a computer
#if (USE_MTP)
  // Get the MTP library from: https://github.com/WMXZ-EU/MTP_t4
  // You do not need the USB2 stuff.
  // But, yes, you should do the installation instructions on that page for USB_MTP_SERIAL, such as:
  //     * For Teensy3.6 (aka Tympan RevD), change C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy3\usb_desc.h
  //     * For Teensy4.x (aka Tympan RevE), change C:\Program Files (x86)\Arduino\hardware\teensy\avr\cores\teensy4\usb_desc.h
  //     * For either Teensy, change C:\Program Files (x86)\Arduino\hardware\teensy\avr
  // Finally, after installing all that you need to tell the Arduino IDE that you'll be
  // using the USB for MTP.  Do this via the Arduino IDE menu: Tools -> USB Type -> MTP Disk SERIAL (Experimental)
  #include <TimeLib.h>   //Used by MTP
  #include <MTP.h>
  #include "MTP_funcs.h" //local to this sketch
#endif

//set the sample rate and block size
const float sample_rate_Hz    = 44100.0f ;  //Allowed values: 8000, 11025, 16000, 22050, 24000, 32000, 44100, 44118, or 48000 Hz
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//setup the  Tympan using the default settings
Tympan          myTympan(TympanRev::E);    //note: Rev C is not compatible with the AIC shield
EarpieceShield  earpieceShield(TympanRev::E, AICShieldRev::A);
SDClass sdx; // define SD card parameters (will get passed to AudioSDWriter and to MTP stuff

// Create the audio objects and audio connections
const int LEFT = 0, RIGHT = (LEFT+1);
#include "AudioConnections.h"


//Creare BLE
BLE ble = BLE(&Serial1);
String msgFromBle = String(""); // Message from ble
int msgLen;

//control display and serial interaction
SerialManager                 serialManager(&ble);
State                         myState(&audio_settings, &myTympan);
String overall_name = String("Tympan: Earpieces with PDM Mics, SD, BLE");


// ////////////// Setup the hardware
void setupAudioHardwareAndProcessing(void) {
  myTympan.enable(); // activate AIC
  earpieceShield.enable();

  earpieceMixer.setTympanAndShield(&myTympan, &earpieceShield); //the earpiece mixer must interact with the hardware, so point it to the hardware
  connectClassesToOverallState();

  //enable the Tympman to detect whether something was plugged inot the pink mic jack
  //myTympan.enableMicDetect(true);

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  earpieceShield.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable

  //Choose the default input
  if (1) {
    //default to the digital PDM mics
    earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop(), if micDetect is enabled
    earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_PDM); // ****but*** then activate the PDM mics
    Serial.println("setupTympanHardware: PDM Earpiece is the active input.");
  } else {
    //default to an analog input
    earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_MIC);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop(), if micDetect is enabled
    Serial.println("setupTympanHardware: analog input is the active input.");
  }
  
  //Set the Bluetooth audio to go straight to the headphone amp, not through the Tympan software
  myTympan.mixBTAudioWithOutput(true);

  //set volumes
  myTympan.volume_dB(0.f);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(myState.earpieceMixer->inputGain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
}

void connectClassesToOverallState(void) {
  myState.earpieceMixer = &earpieceMixer.state;
}

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here
  serialManager.add_UI_element(&earpieceMixer);
  serialManager.add_UI_element(&audioSDWriter);
  serialManager.add_UI_element(&myState);
  
  serialManager.createTympanRemoteLayout();
  
}

// Set up the BLE
void setupBLE(void)
{
  myTympan.forceBTtoDataMode(false); //for BLE, it needs to be in command mode, not data mode
  
  int ret_val = ble.begin();
  if (ret_val != 1) {  //via BC127.h, success is a value of 1
    Serial.print("setupBLE: ble did not begin correctly.  error = ");  Serial.println(ret_val);
    Serial.println("    : -1 = TIMEOUT ERROR");
    Serial.println("    :  0 = GENERIC MODULE ERROR");
  }

  //start the advertising for a connection (whcih will be maintained in serviceBLE())
  if (ble.isConnected() == false) ble.advertise(true);  //not connected, ensure that we are advertising
}


void serviceBLE(unsigned long curTime_millis, unsigned long updatePeriod_millis = 3000) {
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    if (ble.isConnected() == false) {
      if (ble.isAdvertising() == false) {
        Serial.println("serviceBLE: activating BLE advertising");
        ble.advertise(true);  //not connected, ensure that we are advertising
      }
    }
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

// ///////////////// Main setup() and loop() as required for all Arduino programs
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  myTympan.setEchoAllPrintToBT(false);  //if we're using BLE, don't echo the print statements over the BT link
  Serial.print(overall_name); Serial.println(": setup():...");
  Serial.print("Sample Rate (Hz): "); Serial.println(audio_settings.sample_rate_Hz);
  Serial.print("Audio Block Size (samples): "); Serial.println(audio_settings.audio_block_samples);

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(60,audio_settings); //I can only seem to allocate 400 blocks

  //Enable the Tympan and AIC shields to start the audio flowing!
  setupAudioHardwareAndProcessing();
 
  //Set the state of the LEDs
  myTympan.setRedLED(HIGH);  myTympan.setAmberLED(LOW);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setNumWriteChannels(4);             //four channels for this quad recorder, but you could set it to 2
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  Serial.print("Setup: SD configured for "); Serial.print(audioSDWriter.getNumWriteChannels()); Serial.println(" channels.");

  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer

  //setup BLE
  setupBLE();

  //setup serial manager
  setupSerialManager();

  // start the MTP stuff, if applicable
  #if (USE_MTP)
    setup_MTP();
  #endif

  //End of setup
  Serial.println("Setup: complete."); 
  serialManager.printHelp();
}

void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    msgFromBle = "";  msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++)  serialManager.respondToByte(msgFromBle[i]);
  }

  //service the SD recording
  serviceSD();
  
  //service the LEDs
  serviceLEDs(millis());

  //service the BLE (keep it alive to enabe connection)
  serviceBLE(millis(),5000);  //check every 5000 msec

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(),3000); //print every 3000 msec
  if (myState.flag_printCPUandMemory) myState.printCPUtoGUI(millis(),3000);

  //service MTP
  #if (USE_MTP)
    mtpd.loop();
  #endif
}



// ///////////////// Servicing routines


void serviceLEDs(unsigned long curTime_millis) {
  static unsigned long lastUpdate_millis = 0;
  if (lastUpdate_millis > curTime_millis) { lastUpdate_millis = 0; } //account for possible wrap-around
  unsigned long dT_millis = curTime_millis - lastUpdate_millis;
  
  if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
    if (dT_millis > 1000) {  //slow toggle
      toggleLEDs(true,true); //blink both
      lastUpdate_millis = curTime_millis;
    }
  } else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
    if (dT_millis > 50) {  //fast toggle
      toggleLEDs(true,true); //blink both
      lastUpdate_millis = curTime_millis;
    }
  } else {
    if (dT_millis > 1000) {  //slow toggle
      toggleLEDs(true,true); //blink both
      lastUpdate_millis = curTime_millis;
    }
  }
}

void toggleLEDs(void) {
  toggleLEDs(true,true);  //toggle both LEDs
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
      max_max_bytes_written = max(max_bytes_written,max_max_bytes_written);
      max_max_dT_micros = max(max_dT_micros, max_max_dT_micros);
      
      Serial.print("serviceSD: bytes written = ");
      Serial.print(bytes_written); Serial.print(", ");
      Serial.print(max_bytes_written); Serial.print(", ");
      Serial.print(max_max_bytes_written); Serial.print(", ");
      Serial.print("dT millis = "); 
      Serial.print((float)dT_micros/1000.0,1); Serial.print(", ");
      Serial.print((float)max_dT_micros/1000.0,1); Serial.print(", "); 
      Serial.print((float)max_max_dT_micros/1000.0,1);Serial.print(", ");      
      Serial.println();
      max_bytes_written = 0;
      max_dT_micros = 0;     
    }
      
    //print a warning if there has been an SD writing hiccup
    if (PRINT_OVERRUN_WARNING) {
      //if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
      if (i2s_in.get_isOutOfMemory()) {
        float approx_time_sec = ((float)(millis()-audioSDWriter.getStartTimeMillis()))/1000.0;
        if (approx_time_sec > 0.1) {
          Serial.print("SD Write Warning: there was a hiccup in the writing. ");//  Approx Time (sec): ");
          Serial.println(approx_time_sec);
        }
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}



// ////////////// Change settings of system

//Increment the input (PGA) gain for the analog inputs
float incrementInputGain(float increment_dB) { return earpieceMixer.setInputGain_dB(myState.earpieceMixer->inputGain_dB + increment_dB);}

//Increment and Set the Digital Gain for both channels
float incrementDigitalGain(float increment_dB) { return setDigitalGain_dB(myState.digitalGain_dB+increment_dB); }
float setDigitalGain_dB(float gain_dB) {
  gain[LEFT].setGain_dB(gain_dB);  
  return myState.digitalGain_dB = gain[RIGHT].setGain_dB(gain_dB);
}
