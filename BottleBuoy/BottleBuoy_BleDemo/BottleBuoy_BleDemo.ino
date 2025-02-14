/*
*   BottleBuoy_BleDemo
*
*   Created: Chip Audette, Feb 2025
*   Purpose: Test BLE connection to BottleBuoy app from a Tympan Rev F
*   
*   For Tympan RevF, which requires you to set the Arduino IDE to compile for Teensy 4.1
*   
*   See the comments at the top of SerialManager.h and State.h for more description.
*
*/

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library
#include "SerialManager.h"
#include "BottleBuoy_BLE.h"

//set the sample rate and block size
const float sample_rate_Hz = 44100.0f ; //24000 or 44100 (or 44117, or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                    myTympan(TympanRev::F,audio_settings);          //only TympanRev::F has the nRF52480
AudioInputI2S_F32         i2s_in(audio_settings);                         //Digital audio in *from* the Tympan's audio codec
AudioOutputI2S_F32        i2s_out(audio_settings);                        //Digital audio out *to* the Tympan's audio codec

//Make all of the audio connections
AudioConnection_F32       patchCord1(i2s_in, 0, i2s_out, 0);     //connect left input to left output
AudioConnection_F32       patchCord2(i2s_in, 1, i2s_out, 1);     //connect right input to right output

// Create classes for controlling the system
#include      "SerialManager.h"                        
BLE_nRF52&    ble_nRF52 = myTympan.getBLE_nRF52(); //get BLE object for the nRF52840 (RevF only!)
SerialManager serialManager(&ble_nRF52);                 //create the serial manager for real-time control (via USB or App)


// define the setup() function, the function that is called once when the device is booting
void setup() {
  //begin the serial comms (for debugging)
  //Serial.begin(115200);  //USB Serial.  This begin() isn't really needed on Teensy. 
  myTympan.beginBluetoothSerial(); //should use the correct Serial port and the correct baud rate
  delay(1000);
  Serial.println("Tympan_Test_BLE_nRF52: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0.0);           // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(0.0);     // set input volume, 0-47.5dB in 0.5dB setps

  //setup BLE
  Serial.println("Setup ble_nRF52...");
  manuallySetupAndBeginBLE();  // <--------------------- You must have this

  //setup complete
  serialManager.printHelp();
} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device

void loop() {

  //look for in-coming serial messages via USB
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to in coming serial messages via BLE
  if (ble_nRF52.available() > 0) {
    String msgFromBle; ble_nRF52.recvBLE(&msgFromBle);    //get BLE messages (removing non-payload messages)
    for (unsigned int i=0; i < msgFromBle.length(); i++) serialManager.respondToByte(msgFromBle[i]); //interpet each character of the message
  }
  
  //send updated data to the phone app
  serviceSendDataToPhoneApp(millis(),5000);  // <--------------- This sends data to the App very 5000 msec!

} //end loop();

float32_t air_volume_mL = 0.0f;
float32_t temperature_C = 10.0f;
void serviceSendDataToPhoneApp(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0UL;

  if (curTime_millis > lastUpdate_millis+updatePeriod_millis) {
    air_volume_mL += 20.0f;   //increment by 20 mL
    if (air_volume_mL > 700.0) air_volume_mL = 100.0f;
    Serial.println("serviceSendDataToPhoneApp: sending air volume value = " + String(air_volume_mL));
    sendFloat32ToBLE(bottle_ble_service_id, bottle_ble_volume_char_id, air_volume_mL);

    temperature_C += 1.0;    //increment by 1 deg C
    if (temperature_C > 35.0) temperature_C = 10.0;
    Serial.println("serviceSendDataToPhoneApp: sending temperature value = " + String(temperature_C));
    sendFloat32ToBLE(bottle_ble_service_id, bottle_ble_temperature_char_id, temperature_C);   

    lastUpdate_millis = curTime_millis;
  }
}

