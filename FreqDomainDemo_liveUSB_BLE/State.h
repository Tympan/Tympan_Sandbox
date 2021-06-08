

#ifndef State_h
#define State_h


// define structure for tracking state of system (for GUI)
class State : public TympanStateBase_UI {  // look in TympanStateBase for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial) : TympanStateBase_UI(given_settings, given_serial) {}

    // State variables...be sure to look at TympanStateBase.h for more state variables and methods!
    bool is_enabled_formant = false;
    float cur_scale_factor = 1.0;

    bool is_enabled_freq = false;
    int cur_shift_bins = 0;
    
    //helpful state-related methods   
    void printGainSettings(void) {
      local_serial->print("Gains (dB): ");
      //local_serial->print("In ");  local_serial->print(earpieceMixer->inputGain_dB,1);
      //local_serial->print(", Digital "); local_serial->print(digitalGain_dB,1);
      local_serial->println();
    }
};

#endif
