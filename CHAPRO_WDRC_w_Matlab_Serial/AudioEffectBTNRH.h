/*
   AudioEffectBTNRH_F32

   Created: Ira Ray Jenkins, Creare, June 2021
            Chip Audette, OpenAudio, August 2021

   Based on MyAudioAlgorithm by Chip Audette, OpenAudio April 2017
   Purpose: Demonstrate a port of CHAPRO algorithm to Tympan

   MIT License.  use at your own risk.
*/

#ifndef _AudioEffectBTNRH_F32_h
#define _AudioEffectBTNRH_F32_h

// Libraries that all Tympan Audio Effects should include
#include <Arduino.h>
#include "AudioStream_F32.h"

// Algorithm-specific include files
#include <chapro.h>
#include "test_gha.h"            ////////////////////////////////////////// Update this for your CHAPRO Algorithm!!!!


class AudioEffectBTNRH_F32 : public AudioStream_F32
{
  public:
    //constructor
    AudioEffectBTNRH_F32(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32){ };

    //CHAPRO usually uses a bunch of global data structures/arrays to hold parameters and states.  Let's make copies
    //here within this class so that we can run multiple instances of the algorithm (such as left and right) without
    //worrying about the left and right overwriting each other's settings or states.
    //
    //CHAPRO-relevant data members...each instance of this algorithm gets its own copy of these data structures
    void *cp[NPTR] = {0};        // NPTR is set in chapro.h???
    I_O io;                
    int local_prepared = 0;
    CHA_AFC local_afc;     ////////////////////////////////////////// Add or remove based on *your* CHAPRO Algorithm!!!!
    CHA_DSL local_dsl;     ////////////////////////////////////////// Add or remove based on *your* CHAPRO Algorithm!!!!
    CHA_WDRC local_agc;    ////////////////////////////////////////// Add or remove based on *your* CHAPRO Algorithm!!!!

    //methods to access the CHA_DVAR and CHA_IVAR values
    double get_cha_dvar(int ind) { return ((double *)cp[_dvar])[ind]; }; 
    double set_cha_dvar(int ind, double val) { return ((double *)cp[_dvar])[ind] = val; };
    int get_cha_ivar(int ind) { return ((int *)cp[_ivar])[ind]; }; 
    int set_cha_ivar(int ind, int val) { return ((int *)cp[_ivar])[ind] = val; };

    //print out some AFC stuff to the USB serial or to the Bluetooth serial (see the code much later in this file)
    void print_dsl_params(void);
    void print_agc_params(void);
    void print_afc_params(void);
    bool servicePrintingFeedbackModel(unsigned long curTime_millis, unsigned long updatePeriod_millis);
    bool servicePrintingFeedbackModel_toApp(unsigned long curTime_millis, unsigned long updatePeriod_millis, BLE_UI &ble);
    
    //methods to reset some AFC state arrays?  [is this complete?  I think that it's missing stuff?]
    void reset_feedback_model(void) {
      int n_coeff = get_cha_ivar(_afl);
      float *efbp = (float *)cp[_efbp];
      for (int i=0; i<n_coeff;i++) efbp[i]=0.0f;
    }

    //enable different parts of the algorithm
    bool setEnabled(bool val = true) { return enabled = val; }  //overall enabled or not
    bool setAfcEnabled(bool _enable);
    bool getAfcEnabled(void) { int cur_mxl = get_cha_ivar(_mxl); if (cur_mxl > 0) { return true; } else { return false; } };
    int baselineVal_mxl = -1;

    //setup methods
    bool setup_complete = false;
    void setup(void)  { 

      //copy local copies out to the global instances prior to setup?  NO.  Assume that the global functions (configure() and prepare()) will define the globals
      //memcpy(&afc_global, &local_afc, sizeof(CHA_AFC));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      //memcpy(&dsl_global, &local_dsl, sizeof(CHA_DSL));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      //memcpy(&agc_global, &local_agc, sizeof(CHA_WDRC)); //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      //prepared = local_prepared; //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!!  

      //run the configure() and prepare() functions in the global space
      configure(&io);               //in test_gha.h
      prepare(&io, cp);             //in test_gha.h

      //copy global instances back to local instances for safe-keeping (not really needed for monural operation, but some day this will be important for binaural
      memcpy(&local_afc, &afc_global, sizeof(CHA_AFC));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      memcpy(&local_dsl, &dsl_global, sizeof(CHA_DSL));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      memcpy(&local_agc, &agc_global, sizeof(CHA_WDRC)); //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      local_prepared = prepared; //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!!  
      
      setup_complete = true;
    }    
    

    // ////////////////////////////////////////// Here is the call into the CHAPRO that actually does the signal processing
    // Here is where you can add your algorithm.  This function gets called block-wise
    void applyMyAlgorithm(audio_block_f32_t *audio_block)
    {
      float *x = audio_block->data;  //This is used input audio.  And, the output is written back in here, too
      int cs = audio_block->length;  //How many audio samples to process?
      
      
      //copy local copies to global instances before calling functions in the global scope
      memcpy(&afc_global, &local_afc, sizeof(CHA_AFC));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      memcpy(&dsl_global, &local_dsl, sizeof(CHA_DSL));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      memcpy(&agc_global, &local_agc, sizeof(CHA_WDRC)); //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      prepared = local_prepared; //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!!  
       
      //hopefully, this one line is all that needs to change to reflect what CHAPRO code you want to use
      process_chunk(cp, x, x, cs); //see test_gha.h  (or whatever test_xxxx.h is #included at the top)

      //copy global instances back to local copies after completing the functions that are in the global scope
      memcpy(&local_afc, &afc_global, sizeof(CHA_AFC));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      memcpy(&local_dsl, &dsl_global, sizeof(CHA_DSL));  //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      memcpy(&local_agc, &agc_global, sizeof(CHA_WDRC)); //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      local_prepared = prepared; //////////////////// Add or remove items based on *your* CHAPRO Algorithm!!!! 
      
    } //end of applyMyAlgorithms
    // /////////// End of the signal processing code that references CHAPRO

    
   

    // ////////////////////////////////////////// Here is some housekeeping stuff that (hopefully) never needs to be touched

    //here's the method that is called automatically by the Teensy Audio Library for every audio block that needs to be processed
    void update(void)
    {
        if (!enabled) return;
      
        //Serial.println("AudioEffectMine_F32: doing update()");  //for debugging.
        audio_block_f32_t *audio_block;
        audio_block = AudioStream_F32::receiveWritable_f32();
        if (!audio_block) return;

        //do your work
        applyMyAlgorithm(audio_block); //this is the method defined earlier that you can touch as you see fit

        ///transmit the block and release memory
        AudioStream_F32::transmit(audio_block);
        AudioStream_F32::release(audio_block);
    }

    
  private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enabled = false;

}; //end class definition for AudioEffectBTNRH

//methods to print AFC parameters
void AudioEffectBTNRH_F32::print_dsl_params(void) {
  Serial.println("local_dsl: attack = " + String(local_dsl.attack));
  Serial.println("local_dsl: release = "+ String(local_dsl.release));
  Serial.println("local_dsl: maxdB = " + String(local_dsl.maxdB));
  Serial.println("local_dsl: ear = " + String(local_dsl.ear));
  Serial.println("local_dsl: nchannel = " + String(local_dsl.nchannel));
  int nchannel = local_dsl.nchannel;
  Serial.print("local_dsl: cross_freq = "); for (int i=0; i<nchannel;i++) { Serial.print(String(local_dsl.cross_freq[i]) + ", "); } Serial.println();
  Serial.print("local_dsl: tkgain = "); for (int i=0; i<nchannel;i++) { Serial.print(String(local_dsl.tkgain[i]) + ", "); } Serial.println();
  Serial.print("local_dsl: cr = "); for (int i=0; i<nchannel;i++) { Serial.print(String(local_dsl.cr[i]) + ", "); } Serial.println();
  Serial.print("local_dsl: tk = "); for (int i=0; i<nchannel;i++) { Serial.print(String(local_dsl.tk[i]) + ", "); } Serial.println();
  Serial.print("local_dsl: bolt = "); for (int i=0; i<nchannel;i++) { Serial.print(String(local_dsl.bolt[i]) + ", "); } Serial.println();    
}
void AudioEffectBTNRH_F32::print_agc_params(void) {
  Serial.println("AGC: alfa = " + String(get_cha_dvar(_alfa),6));
  Serial.println("AGC: beta = " + String(get_cha_dvar(_beta),6));
  Serial.println("AGC: maxdB = " + String(get_cha_dvar(_mxdb)));
  Serial.println("AGC: tkgn = " + String(get_cha_dvar(_tkgn)));
  Serial.println("AGC: tk = " + String(get_cha_dvar(_tk)));
  Serial.println("AGC: cr = " + String(get_cha_dvar(_cr)));
  Serial.println("AGC: bolt = " + String(get_cha_dvar(_bolt)));
}
void AudioEffectBTNRH_F32::print_afc_params(void) {
  Serial.println("AFC: afl = " + String(get_cha_ivar(_afl)));
  Serial.println("AFC: wfl= " + String(get_cha_ivar(_wfl)));
  Serial.println("AFC: pfl = " + String(get_cha_ivar(_pfl)));
  Serial.println("AFC: fbl = " + String(get_cha_ivar(_fbl)));
  Serial.println("AFC: hdel = " + String(get_cha_ivar(_hdel)));
  Serial.println("AFC: mu = " + String(get_cha_dvar(_mu),8));
  Serial.println("AFC: rho = " + String(get_cha_dvar(_rho),8));
  Serial.println("AFC: eps = " + String(get_cha_dvar(_eps),8));
  Serial.println("AFC: alf = " + String(get_cha_dvar(_alf),8));
  Serial.println("AFC: fbm = " + String(get_cha_dvar(_fbm),8));      
}

//setAfcEnabled: enable or disable the AFC portion of the BTNRH algorithm.
//  This is done by setting the "mxl" (max AFC filter length) to zero.
//  To re-enable, we set the mxl back to its proper (non-zero) value
bool AudioEffectBTNRH_F32::setAfcEnabled(bool _enable) {
  int cur_mxl = get_cha_ivar(_mxl);
  if (cur_mxl > 0) baselineVal_mxl = cur_mxl; //save the value for future use

  if (_enable) { //we are asking to enable the AFC
    if (cur_mxl <= 0) { //had it been disabled?
      //it is currently disabled, so go ahead and enable it
      if (baselineVal_mxl > 0) {  //is there a valid _mxl to enable it with?
        //yes, there is a valid value to use  
        set_cha_ivar(_mxl,baselineVal_mxl);
      } else {
        //no, there is not a valid value, so we cannot enable
      }
    } else { //it is already enabled, so nothing to do
    }
  } else { //we are asking to disable the afc
    set_cha_ivar(_mxl,0);  //set to zero to disable
  }

  return getAfcEnabled();
}

bool AudioEffectBTNRH_F32::servicePrintingFeedbackModel(unsigned long curTime_millis, unsigned long updatePeriod_millis) {
  static unsigned long lastUpdate_millis = 0;
  bool ret_val = false;
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) >= updatePeriod_millis) { //is it time to update the user interface?

    int n_coeff = get_cha_ivar(_afl);
    if (n_coeff > 0) {
      //Serial.println("servicePrintingFeedbackModel: printing feedback model for AFC...");
      float scale_fac = 100.0;  //choose whatever to make the plot prettier
      int n_decimals = 3;
      Serial.println("Model_" + String((int)(scale_fac + 0.5)) + ":");

      Serial.println(scale_fac,n_decimals);
      Serial.println(-scale_fac,n_decimals);
      //Serial.println(0.0);   
      float *efbp = (float *)(cp[_efbp]);  //get a simpler name for the array that we're going to print
      for (int i=0; i<n_coeff; i++) { 
        Serial.println(scale_fac*efbp[i],n_decimals); //print x decimal places
      }
      ret_val = true;
    }

    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
  return ret_val;
}

bool AudioEffectBTNRH_F32::servicePrintingFeedbackModel_toApp(unsigned long curTime_millis, unsigned long updatePeriod_millis, BLE_UI &ble) {
  static unsigned long lastUpdate_millis = 0;
  bool ret_val = false;
  
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) >= updatePeriod_millis) { //is it time to update the user interface?
    int n_coeff = get_cha_ivar(_afl);
    if (n_coeff > 0) {
      Serial.println("servicePrintingFeedbackModel: printing feedback model for AFC...");
      float scale_fac = 1.0;  //choose whatever to make the plot prettier
      int n_decimals = 3;
      String line_prefix = String("P");

      //build up the string with all the data
      ble.sendMessage(line_prefix + String(scale_fac,n_decimals) + String('\n'));
      ble.sendMessage(line_prefix + String(-scale_fac,n_decimals) + String('\n'));
      //ble.sendMessage(line_prefix + String(0.0,n_decimals) + String('\n'));      
      float *efbp = (float *)(cp[_efbp]); //get a simpler name for the array that we're going to print
      for (int i=0; i<n_coeff; i++) { 
        ble.sendMessage(line_prefix + String(scale_fac*efbp[i],n_decimals) + String('\n')); //print x decimal places
      }
      //Serial1.flush();
      //Serial.println("servicePrintingFeedbackModel_toApp: finished.");
      ret_val = true;
    }
    //lastUpdate_millis = curTime_millis; //we will use this value the next time around.
    lastUpdate_millis = millis(); //change the timing so that there are XXXX msec *between* the end of one transmission and the start of the next
  }
  return ret_val;
}

#endif
