
#ifndef _AudioEffectNoiseReduction_FD_F32_h
#define _AudioEffectNoiseReduction_FD_F32_h


#include <AudioFreqDomainBase_FD_F32.h> //from Tympan_Library: inherit all the good stuff from this!
#include <arm_math.h>  //fast math library for our processor

// THIS IS AN EXAMPLE OF HOW TO CREATE YOUR OWN FREQUENCY-DOMAIN ALGRITHMS
// TRY TO DO YOUR OWN THING HERE! HAVE FUN!

//Create an audio processing class to do the lowpass filtering in the frequency domain.
// Let's inherit from  the Tympan_Library class "AudioFreqDomainBase_FD_F32" to do all of the 
// audio buffering and FFT/IFFT operations.  That allows us to just focus on manipulating the 
// FFT bins and not all of the detailed, tricky operations of going into and out of the frequency
// domain.
class AudioEffectNoiseReduction_FD_F32 : public AudioFreqDomainBase_FD_F32   //AudioFreqDomainBase_FD_F32 is in Tympan_Library
{
  public:
    //constructor
    AudioEffectNoiseReduction_FD_F32(const AudioSettings_F32 &settings) : AudioFreqDomainBase_FD_F32(settings) {};

    //destructor...release all of the memory that has been allocated
    ~AudioEffectNoiseReduction_FD_F32(void) {
      if (gains != NULL) delete gains;
      if (ave_spectrum != NULL) delete ave_spectrum;
    }

    //setup...extend the setup that is part of AudioFreqDomainBase_FD_F32
    int setup(const AudioSettings_F32 &settings, const int target_N_FFT);
   
    // get/set methods specific to this particular frequency-domain algorithm
    float32_t getAveSpectrumN(void) { return myFFT.getNFFT(); }; //myFFT is part of AudioFreqDomainBase_FD_F32
    float32_t getAveSpectrum(int ind) { if (ind < ave_spectrum_N) return ave_spectrum[ind]; }
    float32_t *getAveSpectrumPtr(void) { return ave_spectrum; }
    float32_t setAttack_sec(float32_t att) { 
      if (att >= 0.0001) attack_sec = att; 
      attack_coeff = calcCoeffGivenTimeConstant(attack_sec, getOverlappedFFTRate_Hz());     //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32
      return attack_sec; 
    }
    float32_t getAttack_sec(void) { return attack_sec; }
    float32_t setRelease_sec(float32_t rel) { 
      if (rel >= 0.0001) release_sec = rel; 
      release_coeff = calcCoeffGivenTimeConstant(release_sec, getOverlappedFFTRate_Hz()); //getOverlappedFFTRate_Hz() is in AudioFreqDomainBase_FD_F32
      return release_sec; 
    }
    float32_t getRelease_sec(void) { return release_sec; }
    float32_t setMaxAttenuation_dB(float32_t atten_dB) { return max_gain = sqrtf(powf(10.0f, 0.1f*atten_dB)); }
    float32_t getMaxAttenuation_dB(void) { return 10.0f*log10f(max_gain*max_gain); }
    float32_t setThresholdSNR_dB(float32_t val_dB) { return SNR_thresh_for_signal = sqrtf(powf(10.0f, 0.1f*val_dB)); }
    float32_t getThresholdSNR_dB(void) { return 10.0f*log10f(SNR_thresh_for_signal*SNR_thresh_for_signal); }
    float32_t calcCoeffGivenTimeConstant(float32_t time_const_sec, float32_t block_rate_Hz) {
      //http://www.tsdconseil.fr/tutos/tuto-iir1-en.pdf
      //normally, this is computed using the sample rate.  We however will be applying the filter
      //only once every time the FFT is computed, so we substitute the FFT rate for the sample rate 
      return 1.0f - expf(-1.0f / (time_const_sec * block_rate_Hz));
    }
    bool setEnableNoiseEstimationUpdates(bool true_is_update) { return enableNoiseEstimationUpdates = true_is_update; }
    bool getEnableNoiseEstimationUpdates(void) { return enableNoiseEstimationUpdates; }
   
    void resetAveSpectrumAndGains(void) { for (int ind=0; ind < ave_spectrum_N; ind++) { ave_spectrum[ind]=0.0f; gains[ind] = 1.0f; } };
    void updateAveSpectrum(float32_t *current_pow);
    void calcGainBasedOnSpectrum(float32_t *current_pow);

    //this is the method from AudioFreqDomainBase that we are overriding where we will
    //put our own code for manipulating the frequency data.  This is called by update()
    //from the AudioFreqDomainBase_FD_F32.  The update() method is itself called by the
    //Tympan (Teensy) audio system, as with every other Audio processing class.
    virtual void processAudioFD(float32_t *complex_2N_buffer, const int NFFT); 

  protected:
    //create some data members specific to our processing
    float *ave_spectrum = NULL, *gains = NULL;
    int ave_spectrum_N = 0;
    float32_t attack_sec = 5.0f, attack_coeff = 0;
    float32_t release_sec = 1.0f, release_coeff = 0;
    bool enableNoiseEstimationUpdates = true;
    float32_t max_gain = 1.0;               //linear, not dB
    float32_t SNR_thresh_for_signal = 3.0;  //linear, not dB  (so, 10dB is about 3.0)
   
};

int AudioEffectNoiseReduction_FD_F32::setup(const AudioSettings_F32 &settings, const int target_N_FFT) {
  int actual_N_FFT = AudioFreqDomainBase_FD_F32::setup(settings, target_N_FFT);
  ave_spectrum_N = actual_N_FFT / 2 + 1;  // zero bin through Nyquist bin
  ave_spectrum = new float32_t[ave_spectrum_N];
  gains = new float32_t[ave_spectrum_N];
  resetAveSpectrumAndGains();
  setAttack_sec(attack_sec);   //computes the underlying attack filter coefficient
  setRelease_sec(release_sec); //computes the underlying release rilter coefficient
  return actual_N_FFT;
}

void AudioEffectNoiseReduction_FD_F32::updateAveSpectrum(float32_t *current_pow) {
    float32_t att_1 = 0.99999f*(1.0f - attack_coeff), att = attack_coeff;
    float32_t rel_1 = 0.99999f*(1.0f - release_coeff), rel = release_coeff;

    //loop over each bin to update the average for that bin
    for (int ind=0; ind < ave_spectrum_N; ind++) {
      if (current_pow[ind] > ave_spectrum[ind]) {
        //noise is increasing.  use attack time
        ave_spectrum[ind] = att_1 * ave_spectrum[ind] + att*current_pow[ind];
      } else {
        //noise is decreasing.  use release time
        ave_spectrum[ind] = rel_1 * ave_spectrum[ind] + rel*current_pow[ind];
      }
    }
}

void AudioEffectNoiseReduction_FD_F32::calcGainBasedOnSpectrum(float32_t *current_pow) {
  //loop over each bin, evaluate where the current level is relative to the average, and compute the desired gain
  float32_t SNR;
  const float32_t SNR_at_max_gain = 1.0;    //linear.  SNR=1.0 is SNR of 0 dB
  const float32_t gain_at_threshold =1.0; //linear.  Gain=1.0 is gain of 0 dB

  //loop over each frequency bin (up to Nyquist)
  const float32_t coeff = (gain_at_threshold - max_gain)/(SNR_thresh_for_signal - SNR_at_max_gain);
  for (int ind=0; ind < ave_spectrum_N; ind++) {
    SNR = current_pow[ind] / ave_spectrum[ind]; //compute signal to noise ratio (linear, not dB)

    //calculate gain differently based on different SNR regimes
    if (SNR < SNR_at_max_gain) { //note SNR=1.0 is an SNR of 0 dB
      //this is definitely noise.  max attenuation
      gains[ind] = max_gain; //linear value, not dB
      
    } else if (SNR >  SNR_thresh_for_signal) {
      //this is definitely signal.  no attenuation
      gains[ind] = gain_at_threshold; //linear value, not dB
      
    } else {
      //we're in-between, so it's a transition region.  This transition is best done in dB space, but let's try it in linear space
      gains[ind]= (SNR - SNR_at_max_gain) * coeff + max_gain ;
    }
  }
}

//Here is the method we are overriding with our own algorithm...REPLACE THIS WITH WHATEVER YOU WANT!!!!
//  Argument 1: complex_2N_buffer is the float32_t array that holds the FFT results that we are going to
//     manipulate.  It is 2*NFFT in length because it contains the real and imaginary data values
//     for each FFT bin.  Real and imaginary are interleaved.  We only need to worry about the bins
//     up to Nyquist because AudioFreqDomainBase will reconstruct the freuqency bins above Nyquist for us.
//  Argument 2: NFFT, the number of FFT bins
//
//  We get our data from complex_2N_buffer and we put our results back into complex_2N_buffer
void AudioEffectNoiseReduction_FD_F32::processAudioFD(float32_t *complex_2N_buffer, const int NFFT) {

  if (ave_spectrum == NULL) return; //if the memory for the average has yet to be initialized, return early
  if (gains == NULL) return;  //if the memory for the gain has yet to be initialized, return early

  int N_2 = NFFT / 2 + 1;
  float Hz_per_bin = sample_rate_Hz /((float)NFFT); //sample_rate_Hz is from the base class AudioFreqDomainBase_FD_F32
  
  //compute the magnitude^2 of each FFT bin (up to Nyquist
  float raw_pow[N_2];
  arm_cmplx_mag_squared_f32(complex_2N_buffer, raw_pow, N_2);  //get the magnitude for each FFT bin and store somewhere safes

  //loop over each bin and compute the long-term average, which we assume to be the "noise" background
  if (enableNoiseEstimationUpdates) updateAveSpectrum(raw_pow); //updates ave_spectrum, which is one of the data members of this class
 
  //calcluate the new gain values based on the current magnitude versus the ave magnitude
  calcGainBasedOnSpectrum(raw_pow);

  //Loop over each bin and apply the gain
  for (int ind = 0; ind < ave_spectrum_N; ind++) { //only process up to Nyquist...the class will automatically rebuild the frequencies above Nyquist
    //attenuate both the real and imaginary comoponents
    complex_2N_buffer[2 * ind]     *= gains[ind]; //real
    complex_2N_buffer[2 * ind + 1] *= gains[ind]; //imaginary
  }  
}

#endif
