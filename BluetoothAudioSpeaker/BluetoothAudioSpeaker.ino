/*
 * BluetoothAudio_PassThru
 *
 * Created: Chip Audette, OpenAudio, Oct 2019
 * Purpose: Demonstrate basic audio pass-thru from the Bluetooth module
 *
 * Uses Teensy Audio default sample rate of 44100 Hz with Block Size of 128
 * Bluetooth Audio Requires Tympan Rev D
 *
 * License: MIT License, Use At Your Own Risk
 *
 */

//here are the libraries that we need
#include <Tympan_Library.h>  //include the Tympan Library

// define audio objects and connections
Tympan                        myTympan(TympanRev::D);  //do TympanRev::D or TympanRev::C
AudioInputI2S_F32         	  i2s_in;        //Digital audio *from* the Tympan AIC.
AudioMixer4_F32               mixerL,mixerR;
AudioEffectGain_F32           gainL,gainR;
AudioOutputI2S_F32        	  i2s_out;       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency

#if 0
AudioConnection_F32           patchCord1(i2s_in, 0, i2s_out, 0); //connect left input to left output
AudioConnection_F32           patchCord2(i2s_in, 1, i2s_out, 1); //connect right input to right output
#else
AudioConnection_F32           patchCord1(i2s_in,0,mixerL,0);
AudioConnection_F32           patchCord2(i2s_in,1,mixerL,1);
AudioConnection_F32           patchCord5(mixerL,0,gainL,0);
AudioConnection_F32           patchCord6(mixerR,0,gainR,0);
AudioConnection_F32           patchCord11(gainL,0,i2s_out,0);
AudioConnection_F32           patchCord12(gainR,0,i2s_out,1);
#endif


#define MONO 0
#define LEFT 1
#define RIGHT 2
#define STEREO 3

float gain_dB = 6.0f;
int mixerState = MONO;

// define the setup() function, the function that is called once when the device is booting
void setup(void)
{
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial();  delay(500);
  myTympan.setEchoAllPrintToBT(true);
  Serial.println("BluetoothAudio_PassThru: Starting setup()...");

  //allocate the dynamic memory for audio processing blocks
  AudioMemory_F32(10); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  if (1) {
        
    //Configure the Tympan to digitize the analog audio produced by
    //the Bluetooth module 
    myTympan.inputSelect(TYMPAN_INPUT_BT_AUDIO); 
    myTympan.setInputGain_dB(10.0); // keep the BT audio at 0 dB gain
    setMono();
    
    
  } else {
    
    //Or, digitize audio from one of the other sources and simply mix
    //the bluetooth audio via the analog output mixer built into system.

    //select which channel to digitize
    myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
    //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
    //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

    //set the input gain for this channel
    myTympan.setInputGain_dB(15.0);                      // Set input gain.  0-47.5dB in 0.5dB setps

    //now, on the output, mix in the analog audio from the BT module
    myTympan.mixBTAudioWithOutput(true);                 // this tells the tympan to mix the BTAudio into the output
  }

  //Set the desired output volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.

  setGain_dB(gain_dB);

  printHelp();
  
  myTympan.println("Setup complete.");
}

unsigned long last_update_millis=0;
float prev_pot_val = -1.0;
float pot_thresh = 0.75f;
void loop(void)
{
  //asm(" WFI");

  while (Serial.available()) respondToSerial((char)Serial.read());
  while (Serial1.available()) respondToSerial((char)Serial1.read());
  
  
  // Nothing to do - just looping input to output
  //delay(2000);
  //myTympan.println("Running...");
  if (mixerState != STEREO) {
    if (millis() < last_update_millis) last_update_millis = 0;
    if ( (millis() - last_update_millis) > 1000 ) {
      float val = ((float)myTympan.readPotentiometer()) / 1023.0f;
      if (val > pot_thresh) {
        myTympan.println("Potentiometer: setting to stereo...");
        setStereo();
      }
    } else {
      delay(20);
    }
  }
  
}

float setGain_dB(float g_dB) {
  return gainR.setGain_dB(gainL.setGain_dB(gain_dB = g_dB));
}
float increaseGain_dB(float add_dB) {
  return setGain_dB(gain_dB + add_dB);
}

void setMono(void) {
  mixerState = MONO;
  mixerL.gain(0,0.5);  mixerL.gain(1,0.5);
  mixerR.gain(0,0.5);  mixerR.gain(1,0.5);
}
void setLeft(void) {
  mixerState = LEFT;
  mixerL.gain(0,1.0);  mixerL.gain(1,0.0);
  mixerR.gain(0,1.0);  mixerR.gain(1,0.0);
}
void setRight(void) {
  mixerState = RIGHT;
  mixerL.gain(0,0.0);  mixerL.gain(1,1.0);
  mixerR.gain(0,0.0);  mixerR.gain(1,1.0);
}
void setStereo(void) {
  mixerState = STEREO;
  mixerL.gain(0,1.0);  mixerL.gain(1,0.0);
  mixerR.gain(0,0.0);  mixerR.gain(1,1.0);
}


void printHelp(void) {
  myTympan.println("Bluetooth Audio Speaker: Help...");
  myTympan.println("  h: print this help");
  myTympan.println("  m: mono");
  myTympan.println("  l: left");
  myTympan.println("  r: right");
  myTympan.println("  s: stereo");
  myTympan.println("  k/K: increase or decrease gain.");
  myTympan.println();
}

void respondToSerial(char c) {
  switch (c){
    case 'h': case '?':
      printHelp();
      break;
    case 'm':
      setMono();
      myTympan.println("Mono: Mixing both channels at 0.5");
      break;
    case 'l':
      setLeft();
      myTympan.println("Left: Left channel at 1.0");
      break;
    case 'r':
      setRight();
      myTympan.println("Right: Right channel at 1.0");
      break;
    case 's':
      setStereo();
      myTympan.println("Stereo: passing each channel at 1.0");
      break;
    case 'k':
      myTympan.print("Gain: setting gain (dB) to ");
      myTympan.println(increaseGain_dB(3.0));
      break;
    case 'K':
      myTympan.print("Gain: setting gain (dB) to ");
      myTympan.println(increaseGain_dB(-3.0));
      break;
    
  }
}
