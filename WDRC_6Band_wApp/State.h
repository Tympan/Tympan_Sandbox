
#ifndef State_h
#define State_h

#include <AudioEffectCompWDRC_F32.h>  //for CHA_DSL and CHA_WDRC data types

// define structure for tracking state of system (for GUI)
extern "C" char* sbrk(int incr);
class State {
  public:
    //State(void) {};
    State(AudioSettings_F32 *given_settings, Print *given_serial) {
      local_audio_settings = given_settings; 
      local_serial = given_serial;
    }

    enum ANALOG_VS_PDM {INPUT_ANALOG, INPUT_PDM};
    enum ANALOG_INPUTS {INPUT_PCBMICS, INPUT_MICJACK_MIC, INPUT_LINEIN_SE, INPUT_MICJACK_LINEIN};
    //enum GAIN_TYPE {INPUT_GAIN_ID, DSL_GAIN_ID, GHA_GAIN_ID, OUTPUT_GAIN_ID};
    enum EARPIECE_CONFIG_TYPE {MIC_FRONT, MIC_REAR, MIC_BOTH_INPHASE, MIC_BOTH_INVERTED};

    int input_analogVsPDM = INPUT_PDM;
    int input_earpiece_config = MIC_FRONT;
    int input_analog_config = INPUT_PCBMICS;
   
    //int input_source = NO_STATE;
    float inputGain_dB = 0.0;  //gain on the analog inputs (ie, the AIC ignores this when in PDM mode, right?)
    //float vol_knob_gain_dB = 0.0;  //will be overwritten once the potentiometer is read

    //set input streo/mono configuration
    enum INPUTMIX {INPUTMIX_STEREO, INPUTMIX_MONO, INPUTMIX_MUTE};
    int input_mixer_config = INPUTMIX_STEREO;
    
    //algorithm settings
    enum DSL_CONFIG {ALG_PRESET_A, ALG_PRESET_B, ALG_PRESET_C};
    int current_alg_config = ALG_PRESET_A; //default to whatever the first algorithm is
    BTNRH_WDRC::CHA_DSL wdrc_perBand;
    BTNRH_WDRC::CHA_WDRC wdrc_broadBand;
    BTNRH_WDRC::CHA_AFC afc;

    void printPerBandSettings(void) {   printPerBandSettings("myState: printing per-band settings:",wdrc_perBand);  }
    static void printPerBandSettings(String s, BTNRH_WDRC::CHA_DSL &this_dsl) {
      Serial.println(s);
      Serial.print("  : attack = ");Serial.print(this_dsl.attack); Serial.print(", release = ");Serial.println(this_dsl.release);
      Serial.print("  : maxdB = ");Serial.print(this_dsl.maxdB); Serial.print(", numChan = ");Serial.println(this_dsl.nchannel);
      int nchan = min(this_dsl.nchannel,8);
      Serial.print("  : freq = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.cross_freq[i]); Serial.print(" "); }; Serial.println();
      Serial.print("  : lowSPL CR = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.exp_cr[i]); Serial.print(" "); }; Serial.println(); 
      Serial.print("  : lowSPL knee = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.exp_end_knee[i]); Serial.print(" "); }; Serial.println(); 
      Serial.print("  : linear gain = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.tkgain[i]); Serial.print(" "); }; Serial.println(); 
      Serial.print("  : Comp CR = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.cr[i]); Serial.print(" "); }; Serial.println(); 
      Serial.print("  : Comp knee = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.tk[i]); Serial.print(" "); }; Serial.println(); 
      Serial.print("  : Lim Thresh = "); for (int i=0;i < nchan; i++) { Serial.print(this_dsl.bolt[i]); Serial.print(" "); }; Serial.println(); 
    } 
    void printBroadbandSettings(void) {   printBroadbandSettings("myState: printing broadband settings:",wdrc_broadBand);  }
    static void printBroadbandSettings(String s, BTNRH_WDRC::CHA_WDRC &this_gha) {
      Serial.println(s);
      Serial.print("  : attack = ");Serial.print(this_gha.attack); Serial.print(", release = ");Serial.println(this_gha.release);
      Serial.print("  : maxdB = ");Serial.println(this_gha.maxdB); 
      Serial.print("  : lowSPL CR = ");Serial.print(this_gha.exp_cr); Serial.print(", Exp Knee = ");Serial.println(this_gha.exp_end_knee);
      Serial.print("  : linear gain = ");Serial.println(this_gha.tkgain);
      Serial.print("  : Comp CR = ");Serial.print(this_gha.cr); Serial.print(", Comp Knee = ");Serial.println(this_gha.tk);
      Serial.print("  : Lim Thresh = "); Serial.println(this_gha.bolt); 
    }      
    void printAFCSettings(void) {   printAFCSettings("myState: printing AFC settings:",afc);  }
    static void printAFCSettings(String s, BTNRH_WDRC::CHA_AFC &this_afc) {
      Serial.println(s);
      Serial.print("  : Enabled = ");Serial.println(this_afc.default_to_active);
      Serial.print("  : Filter Length = ");Serial.println(this_afc.afl); 
      Serial.print("  : Adaptation Speed (mu) = ");Serial.println(this_afc.mu,6); 
      Serial.print("  : Smoothing Factor (rho) = ");Serial.println(this_afc.rho,3);
      Serial.print("  : Min Level (eps) = ");Serial.println(this_afc.eps,6); 
    } 
    //printing of CPU and memory status
    bool flag_printCPUandMemory = false;
    bool flag_printCPUtoGUI = false;
    void printCPUandMemory(unsigned long curTime_millis = 0, unsigned long updatePeriod_millis = 0) {
       static unsigned long lastUpdate_millis = 0;
      //has enough time passed to update everything?
      if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
      if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
        printCPUandMemoryMessage();
        lastUpdate_millis = curTime_millis; //we will use this value the next time around.
      }
    }
    int FreeRam() {
      char top; //this new variable is, in effect, the mem location of the edge of the heap
      return &top - reinterpret_cast<char*>(sbrk(0));
    }
    void printCPUandMemoryMessage(void) {
      local_serial->print("CPU Cur/Pk: ");
      local_serial->print(local_audio_settings->processorUsage(), 1);
      local_serial->print("%/");
      local_serial->print(local_audio_settings->processorUsageMax(), 1);
      local_serial->print("%, ");
      local_serial->print("MEM Cur/Pk: ");
      local_serial->print(AudioMemoryUsage_F32());
      local_serial->print("/");
      local_serial->print(AudioMemoryUsageMax_F32());
      local_serial->print(", FreeRAM(B) ");
      local_serial->print(FreeRam());
      local_serial->println();
    }
    float getCPUUsage(void) { return local_audio_settings->processorUsage(); }
  
//    void printGainSettings(void) {
//      local_serial->print("Gains (dB): ");
//      local_serial->print("In ");  local_serial->print(inputGain_dB,1);
//      local_serial->print(", UComm ");  local_serial->print(ucommGain_dB,1);
//      local_serial->print(", HearThru ");  local_serial->print(hearthruGain_dB,1);
//      local_serial->print(", Out "); local_serial->print(vol_knob_gain_dB,1);
//      local_serial->println();
//    }

    bool flag_printPlottableData = false;
    

  private:
    Print *local_serial;
    AudioSettings_F32 *local_audio_settings;
};

#endif
