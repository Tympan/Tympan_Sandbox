
#ifndef _CalibrationClasses_h
#define _CalibrationClasses_h

#include <vector>

class CalibrationSteppedSine {
  public:
    CalibrationSteppedSine(AudioSynthWaveform_F32 *_sine, 
      AudioFilterBiquad_F32 *_filtL, AudioFilterBiquad_F32 *_filtR,
      AudioCalcEnvelope_F32 *_envL, AudioCalcEnvelope_F32 *_envR)
    {
      sine = _sine; 
      filtL = _filtL; filtR = _filtR;
      envL = _envL;  envR = _envR;
    }

    //core methods for executing the test
    enum state {INACTIVE, ACTIVE, TEST_SILENCE};
    int start(float _start_Hz, float _end_Hz, int _n_tones, float step_duration_sec);
    void end(void) { if (cur_state != INACTIVE) { cur_state = INACTIVE; stopTone(); n_steps = get_n_measurements(); } } //pre-maturely stop
    int update(void);

    //utilities for running the test
    void reset(void) { all_freq_Hz.clear(); all_left_dBFS.clear(); all_right_dBFS.clear(); }
    void startTone(float freq_Hz, float amp_dBFS) { 
      //change filter
      filtL->setBandpass(0,freq_Hz);
      filtR->setBandpass(0,freq_Hz);

      //change sine wav
      sine->frequency(freq_Hz); 
      sine->amplitude(dBFS_to_amp(amp_dBFS)); 
      last_start_millis = millis();
    }
    void stopTone(void) { sine->amplitude(0.0); }
    float dBFS_to_amp(float amp_dBFS) { return sqrtf(2.0)*sqrtf(powf(10.f,amp_dBFS/10.0)); }
    void takeMeasurement(void);

    //reporting
    int get_n_steps(void) { return n_steps; }
    int get_n_measurements(void) { return (int)all_freq_Hz.size(); }
    float get_freq_Hz(int ind) { return all_freq_Hz[ind]; }
    float get_drive_dBFS(int ind) { return all_drive_dBFS[ind]; };
    float get_left_dBFS(int ind) { return all_left_dBFS[ind]; }
    float get_right_dBFS(int ind) { return all_right_dBFS[ind]; };

    //printing of results
    void printOneResult(int i) {
      if (all_is_silence[i]) {
        Serial.print("Calibraiton Result " + String(i) + ": Drive = AMBIENT.");  
      } else {
        Serial.print("Calibraiton Result " + String(i) + ": Drive = ");
        Serial.print(String(all_freq_Hz[i]) + " Hz, ");
        Serial.print(String(all_drive_dBFS[i]) + " dBFS."); 
      }
      Serial.print(" Measured = ");
      Serial.print(String(all_left_dBFS[i]) + " dBFS, ");
      Serial.print(String(all_right_dBFS[i]) + " dBFS");
      Serial.println();
    }
    void printAllResults(void) {
      Serial.println("Calibration Results: Drive: Freq (Hz), Level (dB FS), Measured: Left (dB FS), Right (dB FS):");
      for (int i=0; i< get_n_measurements(); i++) {
        if (all_is_silence[i] == false) {
          Serial.print(all_freq_Hz[i]);
          Serial.print(", ");
          Serial.print(all_drive_dBFS[i]);
          Serial.print(", ");
        } else {
          Serial.print("AMBIENT:         ");
        }
        Serial.print(all_left_dBFS[i]);
        Serial.print(", ");
        Serial.print(all_right_dBFS[i]);
        Serial.println();
      }
    }

    float default_amp_dBFS = -26.0;
  private:
    AudioSynthWaveform_F32 *sine;
    AudioFilterBiquad_F32 *filtL, *filtR;
    AudioCalcEnvelope_F32 *envL, *envR;
    int cur_state = INACTIVE;

    int n_steps=0;
    float start_Hz = 1000.f;
    float end_Hz = 45000.0;
    float step_dur_sec = 0.5;
    unsigned long last_start_millis = 0;
    std::vector<float> all_freq_Hz;
    std::vector<float> all_drive_dBFS;
    std::vector<bool>  all_is_silence;
    std::vector<float> all_left_dBFS;
    std::vector<float> all_right_dBFS;
};

int CalibrationSteppedSine::start(float _start_Hz, float _end_Hz, int _n_tones, float _step_duration_sec) {
  //check the validity of he inputs
  if (_start_Hz < 0.0f) { Serial.println("CalibrationSteppedSine: start: start_Hz must be greater than zero."); return -1; }
  //if (_end_Hz < _start_Hz) { Serial.println("CalibrationSteppedSine: start: end_Hz must be greater than or equal to start_Hz.");  return -1; }
  if (_end_Hz < 0.0f) { Serial.println("CalibrationSteppedSine: start: end_Hz must be greater than zero.");  return -1; }
  if (_n_tones < 1) { Serial.println("CalibrationSteppedSine: start: n_tones must be greater than zero."); return -1; }
  if (_step_duration_sec <= 0.0f) { Serial.println("CalibrationSteppedSine: start: step_duration_sec must be greater than zero"); return -1; }

  //accept the inputs and reset the system
  reset();  //reset any of the previously saved data
  start_Hz = _start_Hz;  end_Hz = _end_Hz;
  n_steps = _n_tones;  step_dur_sec = _step_duration_sec;

  //start the tone
  float freq_Hz = start_Hz;
  startTone(freq_Hz, default_amp_dBFS);
  cur_state = ACTIVE;

  //return
  return 0;  //zero is OK
}

int CalibrationSteppedSine::update(void) {
  if (cur_state == INACTIVE) return 0; //zero is OK

  //check to see how much time has passed
  unsigned long cur_millis = millis();
  unsigned long dT_millis = cur_millis - last_start_millis;
  if (cur_millis < last_start_millis)  dT_millis = cur_millis + (ULONG_MAX - last_start_millis); //correct for wrap-around

  //has enough time passed to take any action
  if ((float)dT_millis < 1000.0*step_dur_sec) return 0; //not enough time has passed

  //yes, enough time has passed, so take a measurement
  takeMeasurement();

  //if this was the silent period, we are now done
  if (cur_state == TEST_SILENCE) { cur_state = INACTIVE; return 1; } //complete!

  //we are still doing tones.  Are we done with all of the test tone frequencies?
  int cur_step = get_n_measurements(); //counting from one
  if (cur_step >= n_steps) { 
    //tones are finished!  switch to silence
    cur_state = TEST_SILENCE; 
    startTone(start_Hz,-200.0f); //play sine of amplitude -200dB (ie, silence)
  } else {
    //go to the next test step
    int next_step = (cur_step-1) + 1; //switch to counting from zero then increment to next step
    float freq_Hz = (end_Hz - start_Hz) / ((float)(n_steps-1))*next_step + start_Hz;
    startTone(freq_Hz, default_amp_dBFS);
  }
  return 0; //zero is OK
}

void CalibrationSteppedSine::takeMeasurement(void) {

  //record the drive signal parameters
  all_freq_Hz.push_back(sine->getFrequency_Hz());
  float drive_rms = (sine->getAmplitude())/sqrtf(2.0);
  all_drive_dBFS.push_back(10.0f*log10f(powf(drive_rms,2.0f)));

  //decide if this is, in effect, an ambient test case
  float thresh_dBFS = -180.f; //some really small signal amplitude
  float thresh_rms = sqrtf(powf(10.0f,0.1f*thresh_dBFS));
  if (drive_rms < thresh_rms) {
    all_is_silence.push_back(true);
  } else {
    all_is_silence.push_back(false);
  }

  //now add the measured values
  all_left_dBFS.push_back(20.0*log10f(envL->getCurrentLevel()));
  all_right_dBFS.push_back(20.0*log10f(envR->getCurrentLevel()));
  printOneResult(all_freq_Hz.size()-1); 
}

#endif
