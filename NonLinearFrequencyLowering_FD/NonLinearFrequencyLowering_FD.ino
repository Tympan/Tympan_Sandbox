// NonLinearFrequencyLowering_FD
//
// Demonstrate non-linear frequency lowering via frequency domain processing.
//   By "non-linear", you can shift some of frequency range, but you don't have
//   to shift *all* of the frequency range as in our other frequency-domain
//   example code.
//
// Created: Chip Audette (OpenAudio) Oct 2020-Jun 2021
//
// Approach: This processing is performed in the frequency domain.
//    Frequencies can only be shifted by an integer number of bins,
//    so small frequency shifts are not possible.  It depends upon
//    the sample rate divided by the N_FFT.  For example:
//
//    fs = 24 kHz, N=128 results in shift being multiples of 24000/256 = 188 Hz
//    fs = 24 kHz, N=256 results in shift being multiples of 24000/256 = 93.8 Hz
//    fs = 44 kHz, N=256 results in shift being multiples of 44100/256 = 172 Hz
//
//    This processing is performed in the frequency domain where
//    we take the FFT, shift the bins upward or downward, take
//    the IFFT, and listen to the results.  In effect, this is
//    single sideband modulation, which will sound very unnatural
//    (like robot voices!).  Maybe you'll like it, or maybe not.
//    Probably not, unless you like weird.  ;)
//
//    You can shift frequencies upward or downward with this algorithm.
//
//    To change the length of the FFT, you need to change the audio processing
//    block size "audio_block_samples" and the FFT "overlap_factor".  The FFT
//    ("N") is simply audio_block_samples*overlap_factor.  For example:
//
//     64 sample blocks * 2 overlap_factor is an N_FFT of 128 points (with 50% overlap)
//     64 sample blocks * 4 overlap_factor is an N_FFT of 256 points (with 75% overlap)
//     128 sample blocks * 2 overlap_factor is an N_FFT of 256 points (with 50% overlap)
//     128 sample blocks * 4 overlap_factor is an N_FFT of 512 points (with 75% overlap)
//
//    Be aware of being limited by how much computational power that is available.
//    For Tympan RevD (ie, Teensy 3.6) running in stereo, here is the self-reported 
//    CPU usage:
//
//      fs = 24kHz, stereo (analog inputs, -500 Hz shift):
//        32 sample blocks, 4 overlap_fac (N=128, 75% overlap -> 188 Hz resolution): CPU = 46%  (9.6 msec latency?)
//        64 sample blocks, 2 overlap_fac (N=128, 50% overlap -> 188 Hz resolution): CPU = 23%  (12.3 msec latency?)
//        64 sample blocks, 4 overlap_fac (N=256, 75% overlap -> 94 Hz resolution):  CPU = 37%  (17.6 msec latency?)
//        128 sample blocks, 2 overlap_fac (N=256, 50% overlap -> 94 Hz resolution): CPU = 18%  (22.9 msec latency?)
//        128 sample blocks, 4 overlap_fac (N=512, 75% overlap -> 47 Hz resolution): CPU = 53%  (33.6 msec latency?)
//
//      fs = 44100 kHz, stereo (analog inputs, -500 Hz shift):
//        32 sample blocks, 4 overlap_fac (N=128, 75% overlap -> 345 Hz resolution):  CPU = 84% (5.2 msec latency?)
//        64 sample blocks, 2 overlap_fac (N=128, 50% overlap -> 345 Hz resolution):  CPU = 42% (6.7 msec latency?) 
//        64 sample blocks, 4 overlap_fac (N=256, 75% overlap -> 172 Hz resolution):  CPU = 66% (9.6 msec latency?)
//        128 sample blocks, 2 overlap_fac (N=256, 50% overlap -> 172 Hz resolution): CPU = 32% (12.5 msec latency?)
//        128 sample blocks, 4 overlap_fac (N=512, 75% overlap -> 86 Hz resolution):  CPU = 98%+ (too slow!  try mono.)  (would be 18.3 msec latency?)
//
//    If you are using the Tympan RevE (ie, Teensy 4.1), however, it is much more efficient
//        in executing FFT and IFFT operations, so you can probably do 4-5x more calcluations
//        (ie CPU utilization will be 1/4 to 1/5 of the values shown above for the RevD.
//
//  Latency (sec) is [ (17+21 samples) + 2*block_length + N_FFT) / sample_rate_Hz ]
//
// Frequency Domain Processing:
//    * Take samples in the time domain
//    * Take FFT to convert to frequency domain
//    * Manipulate the frequency bins to do the freqyebct shifting
//    * Take IFFT to convert back to time domain
//    * Send samples back to the audio interface
//
// Built for the Tympan library for Teensy 3.6-based hardware
//
// MIT License.  Use at your own risk.
//

#include <Tympan_Library.h>
#include "SerialManager.h"
#include "AudioEffectFreqLoweringFD_F32.h"

//set the sample rate and block size
//const float sample_rate_Hz = 24000.f; ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const float sample_rate_Hz = 44100.f; ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 64;    //for freq domain processing choose a power of 2 (16, 32, 64, 128) but no higher than 128
const int FFT_overlap_factor = 4;       //2 is 50% overlap, 4 is 75% overlap
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan                       myTympan(TympanRev::E, audio_settings);   //use TympanRev::D or TympanRev::C
//AICShield                    earpieceShield(TympanRev::E, AICShieldRev::A);
AudioInputI2S_F32            i2s_in(audio_settings);          //Digital audio *from* the Tympan AIC.
AudioEffectFreqLoweringFD_F32    freqShift_L(audio_settings);     //create the frequency-domain processing block
AudioEffectFreqLoweringFD_F32    freqShift_R(audio_settings);     //create the frequency-domain processing block
AudioEffectGain_F32           gain_L(audio_settings);          //Applies digital gain to audio data.
AudioEffectGain_F32           gain_R(audio_settings);          //Applies digital gain to audio data.
AudioOutputI2S_F32            i2s_out(audio_settings);         //Digital audio out *to* the Tympan AIC.

//Make all of the audio connections
AudioConnection_F32       patchCord10(i2s_in, 0, freqShift_L, 0);  //use the Left input
AudioConnection_F32       patchCord11(i2s_in, 1, freqShift_R, 0);  //use the Right input
AudioConnection_F32       patchCord20(freqShift_L, 0, gain_L, 0);  //connect to gain
AudioConnection_F32       patchCord21(freqShift_R, 0, gain_R, 0);  //connect to gain
AudioConnection_F32       patchCord30(gain_L, 0, i2s_out, 0);      //connect to the left output
AudioConnection_F32       patchCord41(gain_R, 0, i2s_out, 1);      //connect to the right output

//Create BLE
BLE ble = BLE(&Serial1);
String msgFromBle = String(""); // allocate a string (and keep reusing it to minimize memory issues?)
int msgLen;

//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) { enable_printCPUandMemory = !enable_printCPUandMemory; };
SerialManager serialManager(&ble);

//inputs and levels
float input_gain_dB = 15.0f; //gain on the microphone
float vol_knob_gain_dB = 0.0;      //will be overridden by volume knob
void switchToPCBMics(void) {
  myTympan.println("Switching to PCB Mics.");
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the microphone jack - defaults to mic bias OFF
  myTympan.setInputGain_dB(input_gain_dB);
}
void switchToLineInOnMicJack(void) {
  myTympan.println("Switching to Line-in on Mic Jack.");
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF  
  myTympan.setInputGain_dB(0.0);
}
void switchToMicInOnMicJack(void) {
  myTympan.println("Switching to Mic-In on Mic Jack.");
  myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias OFF   
  myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
  myTympan.setInputGain_dB(input_gain_dB);
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

// ////////////////////////////////////////////
      
// define the setup() function, the function that is called once when the device is booting
void setup() {
  myTympan.beginBothSerial(); delay(1000);
  myTympan.println("freqLowering: starting setup()...");
  myTympan.print("    : sample rate (Hz) = ");  myTympan.println(audio_settings.sample_rate_Hz);
  myTympan.print("    : block size (samples) = ");  myTympan.println(audio_settings.audio_block_samples);

  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory_F32(40, audio_settings);

  // Configure the FFT parameters algorithm
  int overlap_factor = FFT_overlap_factor;  //set to 2, 4 or 8...which yields 50%, 75%, or 87.5% overlap (8x)
  int N_FFT = audio_block_samples * overlap_factor;  
  Serial.print("    : N_FFT = "); Serial.println(N_FFT);
  freqShift_L.setup(audio_settings, N_FFT); //do after AudioMemory_F32();
  freqShift_R.setup(audio_settings, N_FFT); //do after AudioMemory_F32();

  //configure the frequency shifting
  float shiftFreq_Hz = 0.0; //shift audio downward a bit
  //float Hz_per_bin = audio_settings.sample_rate_Hz / ((float)N_FFT);
  //int shift_bins = (int)(shiftFreq_Hz / Hz_per_bin + 0.5);  //round to nearest bin
  //shiftFreq_Hz = shift_bins * Hz_per_bin;
  Serial.print("Setting shift to "); Serial.print(shiftFreq_Hz); Serial.print(" Hz");
  shiftFreq_Hz = freqShift_R.setShift_Hz(freqShift_L.setShift_Hz(shiftFreq_Hz)); //0 is no freq shifting.
  Serial.print("FFT resolution allowed for a shift of "); Serial.print(shiftFreq_Hz); Serial.print(" Hz");
  
  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC
  //earpieceShield.enable();

  //setup DC-blocking highpass filter running in the ADC hardware itself
  float cutoff_Hz = 60.0;  //set the default cutoff frequency for the highpass filter
  myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble
  //earpieceShield.setHPFonADC(true, cutoff_Hz, audio_settings.sample_rate_Hz); //set to false to disable
  

  //Choose the desired input
  switchToPCBMics();        //use PCB mics as input
  //switchToMicInOnMicJack(); //use Mic jack as mic input (ie, with mic bias)
  //switchToLineInOnMicJack();  //use Mic jack as line input (ie, no mic bias)
  
  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
   
  // configure the blue potentiometer
  servicePotentiometer(millis(),0); //update based on the knob setting the "0" is not relevant here.

  //setup BLE
  while (Serial1.available()) Serial1.read(); //clear the incoming Serial1 (BT) buffer
  setupBLE();


  //finish the setup by printing the help menu to the serial connections
  serialManager.printHelp();
}


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {

  //respond to Serial commands
  while (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial

  //respond to BLE
  if (ble.available() > 0) {
    msgFromBle = "";  msgLen = ble.recvBLE(&msgFromBle);
    for (int i=0; i < msgLen; i++)  serialManager.respondToByte(msgFromBle[i]);
  }

  //service the BLE (keep it alive to enabe connection)
  serviceBLE(millis(),5000);  //check every 5000 msec
  
  //check the potentiometer
  servicePotentiometer(millis(), 100); //service the potentiometer every 100 msec

  //check to see whether to print the CPU and Memory Usage
  if (enable_printCPUandMemory) myTympan.printCPUandMemory(millis(),3000); //print every 3000 mse

} //end loop();


// ///////////////// Servicing routines

//servicePotentiometer: listens to the blue potentiometer and sends the new pot value
//  to the audio processing algorithm as a control parameter
void servicePotentiometer(unsigned long curTime_millis, const unsigned long updatePeriod_millis) {
  //static unsigned long updatePeriod_millis = 100; //how many milliseconds between updating the potentiometer reading?
  static unsigned long lastUpdate_millis = 0;
  static float prev_val = -1.0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?

    //read potentiometer
    float val = float(myTympan.readPotentiometer()) / 1023.0; //0.0 to 1.0
    val = (1.0/9.0) * (float)((int)(9.0 * val + 0.5)); //quantize so that it doesn't chatter...0 to 1.0

    //use the potentiometer value to control something interesting
    if (abs(val - prev_val) > 0.05) { //is it different than befor?
      prev_val = val;  //save the value for comparison for the next time around

      //change the volume
      float vol_dB = 0.f + 30.0f * ((val - 0.5) * 2.0); //set volume as 0dB +/- 30 dB
      myTympan.print("Changing output volume to = "); myTympan.print(vol_dB); myTympan.println(" dB");
      myTympan.volume_dB(vol_dB);

    }

    
    lastUpdate_millis = curTime_millis;
  } // end if
} //end servicePotentiometer();


void printGainSettings(void) {
  myTympan.print("Gain (dB): ");
  myTympan.print("Vol Knob = "); myTympan.print(vol_knob_gain_dB,1);
  //myTympan.print(", Input PGA = "); myTympan.print(input_gain_dB,1);
  myTympan.println();
}


void incrementKnobGain(float increment_dB) { //"extern" to make it available to other files, such as SerialManager.h
  setVolKnobGain_dB(vol_knob_gain_dB+increment_dB);
}

void setVolKnobGain_dB(float gain_dB) {
  vol_knob_gain_dB = gain_dB;
  gain_L.setGain_dB(vol_knob_gain_dB);gain_R.setGain_dB(vol_knob_gain_dB);
  printGainSettings();
}

float incrementFreqKnee(float incr_factor) {
  float cur_val = freqShift_L.getStartFreq_Hz();
  return freqShift_R.setStartFreq_Hz(freqShift_L.setStartFreq_Hz(cur_val + incr_factor));
}

float incrementFreqCR(float incr_factor) {
  incr_factor = max(0.1,min(5.0,incr_factor));
  float cur_val = freqShift_L.getFreqCompRatio();
  return freqShift_R.setFreqCompRatio(freqShift_L.setFreqCompRatio(cur_val * incr_factor));
}

float incrementFreqShift(float incr_factor) {
  float cur_val = freqShift_L.getShift_Hz();
  return freqShift_R.setShift_Hz(freqShift_L.setShift_Hz(cur_val + incr_factor));
}
