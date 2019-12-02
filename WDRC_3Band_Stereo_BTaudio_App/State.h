
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
    
    //enum INPUTS {NO_STATE, INPUT_PCBMICS, INPUT_MICJACK, INPUT_LINEIN_SE, INPUT_MICJACK_LINEIN};
    //enum GAIN_TYPE {INPUT_GAIN_ID, DSL_GAIN_ID, GHA_GAIN_ID, OUTPUT_GAIN_ID};
   
    //int input_source = NO_STATE;
    //float inputGain_dB = 0.0;  //gain on the microphone
    //float vol_knob_gain_dB = 0.0;  //will be overwritten once the potentiometer is read

    //set input streo/mono configuration
    enum INPUTMIX {INPUTMIX_STEREO, INPUTMIX_MONO, INPUTMIX_MUTE};
    int input_mixer_config = INPUTMIX_STEREO;
    
    //algorithm settings
    enum DSL_CONFIG {DSL_PRESET_A, DSL_PRESET_B};
    int current_dsl_config = DSL_PRESET_A; //default to whatever the first algorithm is
    BTNRH_WDRC::CHA_DSL wdrc_perBand;
    BTNRH_WDRC::CHA_WDRC wdrc_broadBand;
    BTNRH_WDRC::CHA_AFC afc;

    //printing of CPU and memory status
    bool flag_printCPUandMemory = false;
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
  
//    void printGainSettings(void) {
//      local_serial->print("Gains (dB): ");
//      local_serial->print("In ");  local_serial->print(inputGain_dB,1);
//      local_serial->print(", UComm ");  local_serial->print(ucommGain_dB,1);
//      local_serial->print(", HearThru ");  local_serial->print(hearthruGain_dB,1);
//      local_serial->print(", Out "); local_serial->print(vol_knob_gain_dB,1);
//      local_serial->println();
//    }
  private:
    Print *local_serial;
    AudioSettings_F32 *local_audio_settings;
};

#endif
