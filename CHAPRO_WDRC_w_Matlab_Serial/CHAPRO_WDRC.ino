/*
   CHARPRO_WDRC

   Created: Chip Audette, OpenAudio, November 2021

   Purpose: Demonstrate a port of tst_gha.c from ChaPro to Tympan.
            Includes ability to control some of the AFC algorithm via the SerialMonitor or App
            and to plot some of the AFC states via the SerialPlotter.
            Ingests the prescription via an old file format from Daniel's "Controller" windows app

   MIT License.  use at your own risk.
*/


//Include Arduino/Tympan related libraries
#include <Arduino.h>
#include <Tympan_Library.h>
#include "implementStdio.h"   //alows for printf

//Include algorithm-specific files
#include "test_gha.h"          //see the tab "test_gha.h"..........be sure to update the name if you change the filename!
#include "AudioEffectBTNRH.h"  //see the tab "AudioEffectBTNRH.h"
 
// ///////////////////////////////////////// setup the audio processing classes and connections

//set the sample rate and block size
const float sample_rate_Hz = (int)srate;  //Set in test_gha.h.  Must be one of the values in the table in AudioOutputI2S_F32.h
const int audio_block_samples = chunk;    //Set in test_gha.h.  Must be less than or equal to 128 
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Create the Tympan
Tympan myTympan(TympanRev::E, audio_settings);
EarpieceShield   earpieceShield(TympanRev::E, AICShieldRev::A);  //Note that EarpieceShield is defined in the Tympan_Libarary in AICShield.h

// Create the audio connections
#include "AudioConnections.h"

// Create classes for controlling the system
#include      "SerialManager.h"
#include      "State.h"                            
BLE_UI&       ble = myTympan.getBLE_UI();        //myTympan owns the ble object, but we have a reference to it here                                    //create bluetooth BLE
SerialManager serialManager(&ble);               //create the serial manager for real-time control (via USB or App)
State         myState(&audio_settings, &myTympan, &serialManager); //keeping one's state is useful for the App's GUI

void connectClassesToOverallState(void) {
  myState.earpieceMixer = &earpieceMixer.state;
}

//set up the serial manager
void setupSerialManager(void) {
  //register all the UI elements here  
  serialManager.add_UI_element(&myState);
  serialManager.add_UI_element(&ble);
  serialManager.add_UI_element(&earpieceMixer); 
  serialManager.add_UI_element(&audioSDWriter);
}

float setDigitalGain_dB(float val_dB) {
    return myState.digital_gain_dB = gain1.setGain_dB(val_dB);
}

float setOutputGain_dB(float gain_dB) {  
  earpieceShield.volume_dB(gain_dB);
  return myState.output_gain_dB = myTympan.volume_dB(gain_dB);
}

// /////////////////////////  Start the Arduino-standard functions: setup() and loop()

void setup() { //this runs once at startup  
  Serial1.begin(9600); //for talking to the Bluetooth unit (9600 is the right choice for for RevE)
  while ((!Serial) && (millis() < 1000));  //stall for a bit to let the USB get up and running.
  //myTympan.beginBothSerial(); delay(500);
      
  if (Serial) Serial.print(CrashReport);  //if it crashes and restarts, this will give some info
  myTympan.println("CHAPRO for Tympan: Test GHA w/Earpieces w/App: setup():...");
  myTympan.println("  Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
  myTympan.println("  Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

  // Audio connections require memory to work.
  AudioMemory_F32(50, audio_settings);

  // /////////////////////////////////////////////  do any setup of the algorithms

  BTNRH_alg1.setup();           //in AudioEffectBTNRH.h
  BTNRH_alg1.setEnabled(true);  //see AudioEffectBTNRH.h.  This could be done later in setup()
 
  // //////////////////////////////////////////// End setup of the algorithms


  // Start the Tympan
  Serial.println("setup: Tympan enable...");
  myTympan.enable();
  earpieceShield.enable();

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble
  earpieceShield.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

  //make some other softare connections
  earpieceMixer.setTympanAndShield(&myTympan, &earpieceShield); //the earpiece mixer must interact with the hardware, so point it to the hardware
  connectClassesToOverallState();
  setupSerialManager();

  //Choose the desired input
  if (1) {
    //default to the digital PDM mics within the Tympan earpieces
    earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman...this will be overridden by the serviceMicDetect() in loop(), if micDetect is enabled
    earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_PDM);       // ****but*** then activate the PDM mics
    Serial.println("setup: PDM Earpiece is the active input.");
  } else {
    //default to an analog input
    earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);  //Choose the desired audio analog input on the Typman
    //earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_MIC);  //Choose the desired audio analog input on the Typman
    earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);       // ****but*** then activate the PDM mics
    Serial.println("setup: analog input is the active input.");
  }
    
  //Set the desired volume levels
  setOutputGain_dB(myState.output_gain_dB);  // -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
  myTympan.setInputGain_dB(myState.earpieceMixer->inputGain_dB); // set MICPGA volume, 0-47.5dB in 0.5dB setps
  
  //set the default gain
  Serial.println("setup(): setting overall gain to " + String(myState.digital_gain_dB));
  setDigitalGain_dB(myState.digital_gain_dB);

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  //audioSDWriter.setNumWriteChannels(4);     // Default is 2 channels.  Can record 4 channel, if asked
  Serial.println("Setup: SD configured for writing " + String(audioSDWriter.getNumWriteChannels()) + " audio channels.");
  
  //setup BLE
  ble.setUseFasterBaudRateUponBegin(true); //speeds up baudrate to 115200.  ONLY WORKS FOR ANDROID.  If iOS, you must set to false.
	delay(500); myTympan.setupBLE(); delay(500); //Assumes the default Bluetooth firmware. You can override!

  //finish setup
  Serial.println("Setup complete.");
  serialManager.printHelp();
}

void loop() {  //this runs forever in a loop

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB...respondToByte is in SerialManagerBase...it then calls SerialManager.processCharacter(c)

  //respond to BLE
  if (ble.available() > 0) {
    String msgFromBle; int msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++) serialManager.respondToByte(msgFromBle[i]);  //respondToByte is in SerialManagerBase...it then calls SerialManager.processCharacter(c)
  }

  //service the BLE advertising state
  ble.updateAdvertising(millis(),5000); //check every 5000 msec to ensure it is advertising (if not connected)

  //service the SD recording
  audioSDWriter.serviceSD_withWarnings(audio_in); //needs some info from i2s_in to provide some of the warnings

  //service the LEDs...blink slow normally, blink fast if recording
  myTympan.serviceLEDs(millis(),audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING); 

  //periodically print the CPU and Memory Usage
  if (myState.flag_printCPUandMemory) myState.printCPUandMemory(millis(), 3000); //print every 3000msec  (method is built into TympanStateBase.h, which myState inherits from)
  if (myState.flag_printCPUandMemory) myState.printCPUtoApp(millis(), 3000);     //send to App every 3000msec (method is built into TympanStateBase.h, which myState inherits from)

  //periodically print the AFC model coefficients
  if (myState.flag_printLeftFeedbackModel) BTNRH_alg1.servicePrintingFeedbackModel(millis(), 1000);
  if (myState.flag_printLeftFeedbackModel_toApp) {
   BTNRH_alg1.servicePrintingFeedbackModel_toApp(millis(), 1000, ble); //BLE transfer is slow, this this ends up spacing transmissions as starting a new one after 1000msec has passed since end of previous one
  }
}
