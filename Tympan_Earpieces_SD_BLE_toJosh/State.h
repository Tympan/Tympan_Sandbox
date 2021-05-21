
#ifndef State_h
#define State_h

#include "EarpieceMixer_F32.h"

// define structure for tracking state of system (for GUI)
extern "C" char* sbrk(int incr);
class State {
  public:
    //State(void) {};
    State(AudioSettings_F32 *given_settings, Print *given_serial) {
      local_audio_settings = given_settings; 
      local_serial = given_serial;
    }
    
    //enum GAIN_TYPE {INPUT_GAIN_ID, DSL_GAIN_ID, GHA_GAIN_ID, OUTPUT_GAIN_ID};

    //enum ANALOG_VS_PDM {INPUT_ANALOG=0, INPUT_PDM};
    //int input_analogVsPDM = INPUT_PDM;

    //enum ANALOG_INPUTS {INPUT_PCBMICS=0, INPUT_MICJACK_MIC, INPUT_LINEIN_SE, INPUT_MICJACK_LINEIN};
    //int input_analog_config = INPUT_PCBMICS;
   
    //int input_source = INPUT_PDM;
    //float inputGain_dB = 15.0;  //gain on the analog inputs (ie, the AIC ignores this when in PDM mode, right?)
    float volKnobGain_dB = 0.0;  //will be overwritten once the potentiometer is read
   
    //set the front-rear parameters
//    enum FRONTREAR_CONFIG_TYPE {MIC_FRONT, MIC_REAR, MIC_BOTH_INPHASE, MIC_BOTH_INVERTED, MIC_BOTH_INVERTED_DELAYED};
//    int input_frontrear_config = MIC_FRONT;
//    int targetFrontDelay_samps = 0;  //in samples
//    int targetRearDelay_samps = 0;  //in samples
//    int currentFrontDelay_samps = 0; //in samples
//    int currentRearDelay_samps = 0; //in samples
//    float rearMicGain_dB = 0.0f;;

   //set input streo/mono configuration
//    enum INPUTMIX {INPUTMIX_STEREO, INPUTMIX_MONO, INPUTMIX_MUTE, INPUTMIX_BOTHLEFT, INPUTMIX_BOTHRIGHT};
//    int input_mixer_config = INPUTMIX_BOTHLEFT;
//    int input_mixer_nonmute_config = INPUTMIX_BOTHLEFT;

    EarpieceMixerState *earpieceMixer;
    
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
      local_serial->print(Tympan::FreeRam());
      local_serial->println();
    }
    float getCPUUsage(void) { return local_audio_settings->processorUsage(); }

    void printGainSettings(void) {
      local_serial->print("Gains (dB): ");
      local_serial->print("In ");  local_serial->print(earpieceMixer->inputGain_dB,1);
      //local_serial->print(", Out "); local_serial->print(volKnobGain_dB,1);
      local_serial->println();
    }
  private:
    Print *local_serial;
    AudioSettings_F32 *local_audio_settings;
};

#endif
