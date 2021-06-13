

#ifndef State_h
#define State_h

// define structure for tracking state of system (for GUI)
class State : public TympanStateBase_UI { // look in TympanStateBase for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial) : TympanStateBase_UI(given_settings, given_serial) {}
    
    //Put states here to ease the updating of the GUI
    float input_gain_dB = 0.0;
    float digital_gain_dB = 0.0;
    float output_gain_dB = 0.0;

    //Frequency processing
    float NR_attack_sec = 5.0;
    float NR_release_sec = 1.0;
    float NR_max_atten_dB = 20.0;
    float NR_threshold_SNR_dB = 10.0;
    bool NR_enable_noise_est_updates = true;
    
};

#endif
