

#ifndef State_h
#define State_h


// The purpose of this class is to be a central place to hold important information
// about the algorithm parameters or the operation of the system.  This is mainly to
// help *you* (the programmer) know and track what is important.  
//
// For example, if it is important enough to display or change in the GUI, you might
// want to put it here.  It is a lot easier to update the GUI if all the data feeding
// the GUI lives in one place.
//
// Also, in the future, you might imagine that you want to save all of the configuration
// and parameter settings together as some sort of system-wide preset.  Then, you might
// save the preset to the SD card to recall later.  If you were to get in the habit of
// referencing all your important parameters in one place (like in this class), it might
// make it easier to implement that future capability to save and recal presets

// define a class for tracking the state of system (primarily to help our implementation of the GUI)
#define N_TONE_TABLE 18
class State : public TympanStateBase_UI { // look in TympanStateBase or TympanStateBase_UI for more state variables and helpful methods!!
  public:
    State(AudioSettings_F32 *given_settings, Print *given_serial, SerialManagerBase *given_sm) : TympanStateBase_UI(given_settings, given_serial, given_sm) {}

    //look in TympanStateBase for more state variables!  (like, bool flag_printCPUandMemory)

    enum INP_SOURCE {INPUT_PCBMICS=0, INPUT_JACK_MIC, INPUT_JACK_LINE, INPUT_PDM_MICS};
    int input_source = INPUT_JACK_LINE;
    
    //Put different gain settings (except those in the compressors) here to ease the updating of the GUI
    float input_gain_dB = 0.0;   //gain of the hardware PGA in the AIC
    float output_dacGain_dB = 0.0;  //gain of the hardware headphone amplifier in the AIC.  Leave at 0.0 unless you have a good reason...set to -15 dB to lower harmonic distortion (which also lowers max SPL)

    //tone parameters
    const int n_tone_table = N_TONE_TABLE;
    float tone_table_Hz[N_TONE_TABLE] = {125., 250., 500., 750., 1000., 1500., 2000., 3000., 4000., 5000., 6000., 8000., 9000., 10000., 11200., 12500., 14000., 16000.};
    int cur_tone_table_ind = 4;
    float tone_Hz = 1000.0;        //freuqency of tone.  any value less than Nyquist
    float tone_dBFS = 0.0;        //loudness of tone.  any value less than 0.0 dB.  Unclear what the max value should be to avoid distortion.  0dB?  -3dB?  Other?
    bool is_tone_active = false;   //the code will flip this to true whenever we want to hear the tone
    
};

#endif
