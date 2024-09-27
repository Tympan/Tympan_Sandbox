#ifndef AudioPath_Sine_wFFT_h
#define AudioPath_Sine_wFFT_h

#include <vector>
#include "AudioStreamComposite_F32.h"
#include <utility/BTNRH_rfft.h>  //Tympan_Library FFT routine (generic C, not ARM-specific)
#include <math.h>


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioStreamComposite_F32.h
//
// ////////////////////////////////////////////////////////////////////////////


// This AudioPath takes the audio input (all four channels) and applies gain.  This uses the analog inputs.
class AudioPath_Sine_wFFT : public AudioStreamComposite_F32 {
  public:
    //Constructor
    AudioPath_Sine_wFFT(AudioSettings_F32 &_audio_settings)  : AudioStreamComposite_F32(_audio_settings) 
    {
      //instantiate audio classes...
      audioObjects.push_back( startNode = new AudioSwitchMatrix4_F32( _audio_settings ) ); startNode->instanceName = String("Input Matrix"); //per AudioStreamComposite_F32, always have this first

      //Create the audio queues necessary for doing the FFT on the inputs
      for (int i=0; i<4; i++) {  //create a queue of audio blocks for each possible input (up to 4 inputs!)
        allQueues.push_back( new AudioRecordQueue_F32( _audio_settings ) );   //create the queue and keep it in a local vector (with an easy-to-read name)
        allQueues[i]->instanceName = String("Record Queue " + String(i));   //give a human readable name to help Chip's debugging of startup issues
        audioObjects.push_back( allQueues[i] );  //save the queue in the vector maintained by AudioStreamComposite_F32_h, which is intended to hold *all* audio objects that are instantiated (not just queues)
      }

      //Create the sound-generation objects for creating the sine output
      audioObjects.push_back( sineWave1 = new AudioSynthWaveform_F32( _audio_settings ) ); sineWave1->instanceName = String("Sine Wave");  //give a human readable name to help Chip's debugging of startup issues
      
      //Create the output end-node
      audioObjects.push_back( endNode   = new AudioForwarder4_F32( _audio_settings, this ) ); endNode->instanceName   = String("Output Forwarder"); //per AudioStreamComposite_F32, always have this last

      //connect the audio classes
      for (int I=0; I < (int)allQueues.size(); I++)  patchCords.push_back( new AudioConnection_F32(*startNode, I, *allQueues[I], 0)); //connect inputs to the queue objects
      for (int I=0; I < 4; I++)                      patchCords.push_back( new AudioConnection_F32(*sineWave1, 0, *endNode,      I)); //connect sine to all outputs

      //setup the parameters of the audio processing
      setupAudioProcessing();

      //initialize to being inactive
      setActive(false);  //setActive is in AudioStreamComposite_F32_h

      //choose a human-readable name for this audio path
      instanceName = "Sine with FFT Analysis";  //"name" is defined as a String in AudioStream_F32

      //set some other parameters and allocate the required memory for buffers
      sample_rate_Hz = _audio_settings.sample_rate_Hz;
      n_audio_blocks_required = (int)((Nfft-1) / _audio_settings.audio_block_samples) + 1;  //make sure that we round up
      for (int Ichan=0; Ichan < (int)allQueues.size(); Ichan++) {
        float *fftMemory_cmplx = new float[Nfft + 1];  //allocate memory for (complex) FFT results (docs say N+2 samples, so it'll hold positive frequency values as interleaved complex values up to Nyquist)
        if (fftMemory_cmplx == NULL) Serial.println("AudioPath_Sine_wFFT: Constructor: *** ERROR ***: Could not allocate FFT memory for chan " + String(Ichan));
        allFftMemory_cmplx.push_back(fftMemory_cmplx);
      }
      fftWindow = new float[Nfft];  computeFftWindow(Nfft, fftWindow);
    }

    // The base destructor will destroy the audio objects (in "audioObjects") and connections (in "patchCords"), but 
    // need to destroy everything else that I might have instantiated in addition to those two types of objects
    virtual ~AudioPath_Sine_wFFT() //will automatically call the destructor for AudioStreamComposite_F32  
    {
      delete[] fftWindow;
      for (auto & fftMemory : allFftMemory_cmplx) { //iterate over each item in the vector
        delete[] fftMemory;
      }
    }

    //setupAudioProcess: initialize the sine wave to the desired frequency and amplitude
    virtual void setupAudioProcessing(void) {
      setFrequency_Hz(freq1_Hz);
      setAmplitude(sine1_amplitude);
      enableTone(true); //make tone active
    }

    //setActive: extend the default setAcive() method from AudioStreamComposite_F32 so that it starts/stops the queues
    virtual bool setActive(bool _active) { return setActive(_active, false); }  //needed or it won't compile?  I don't know why the base class version doesn't suffice.
    virtual bool setActive(bool _active, bool flag_moreSetup) {
      bool active = AudioStreamComposite_F32::setActive(_active, flag_moreSetup); //call the default setActive behavior

      AudioNoInterrupts();  //stop the audio processing in order to change all the active states (from Audio library)
      if (active) { 
        clearRecording();
        enableTone(true);
        beginRecording(); 
      } else { 
        stopRecording(); 
        enableTone(false); 
        clearRecording();
      }
      AudioInterrupts();  //restart the audio processing in order to change all the active states

      return active;
    }

    //setup the hardware.  This is called automatically by AudioStreamComposite_F32::setActive(flag_active) whenever flag_active = true
    virtual void setup_fromSetActive(void) {
      if (tympan_ptr != NULL) {
        tympan_ptr->muteHeadphone(); tympan_ptr->muteDAC();  //try to silence the switching noise/pop...doesn't seem to work.
        tympan_ptr->enableDigitalMicInputs(false);           //switch to analog inputs...or, change to TRUE for using digital mic (PDM) inputs
        //tympan_ptr->inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);  //Choose the desired input (on-board mics)
        tympan_ptr->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);  //Choose the desired input  (no on-board mics, so use pink input jack as line ine)
        tympan_ptr->setInputGain_dB(adc_gain_dB);            //set input gain, 0-47.5dB in 0.5dB setps
        tympan_ptr->setDacGain_dB(dac_gain_dB, dac_gain_dB); //set the DAC gain.  left and right
        tympan_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB, headphone_amp_gain_dB);  //set the headphone amp gain.  left and right       
        tympan_ptr->unmuteDAC();  tympan_ptr->unmuteHeadphone();
      }
      if (shield_ptr != NULL) {
        shield_ptr->muteHeadphone(); shield_ptr->muteDAC();    //try to silence the switching noise/pop...doesn't seem to work. 
        shield_ptr->enableDigitalMicInputs(false);             //switch to analog inputs...or, change to TRUE for using digital mic (PDM) inputs
        shield_ptr->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN);  //Choose the desired input  (no on-board mics, so use pink input jack as line ine)
        shield_ptr->setInputGain_dB(adc_gain_dB);              //set input gain, 0-47.5dB in 0.5dB setps
        shield_ptr->setDacGain_dB(dac_gain_dB, dac_gain_dB);   //set the DAC gain.  left and right
        shield_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB, headphone_amp_gain_dB);  //set the headphone amp gain.  left and right                    
        shield_ptr->unmuteDAC();  shield_ptr->unmuteHeadphone();              
      }
    }

    virtual void beginRecording(void) {
      Serial.println("AudioPath_Sine_wFFT: beginRecording...");
      for (auto & queue : allQueues) queue->begin(); //loop over all queues in vector and begin the audio buffering process}
    }
    virtual void stopRecording(void) {
      for (auto & queue : allQueues) queue->end(); //loop over all queues in vector and end the audio buffering process}
    }
    virtual void clearRecording(void) {
      for (auto & queue : allQueues) queue->clear(); //loop over all queues in vector and clear any audio buffers that are held
    }

    virtual int serviceMainLoop(void) {
      int return_val = 0;

      //loop over each channel and see if enough data is present to do an FFT
      for (int Ichan = 0; Ichan < (int)allQueues.size(); Ichan++) { //loop over all channels}
        AudioRecordQueue_F32 *queue = allQueues[Ichan];
        if (queue->available() >= n_audio_blocks_required) {
          //gather the queued data and compute the FFT!
          return_val = processFFT(Ichan,queue);  //returns zero if OK.  Automatically releases queued blocks once they are used
        }
      }    

      //periodically print FFT results to the serial monitor
      if ((millis() < lastUpdate_millis) || (millis() > lastUpdate_millis + update_period_millis)) {
        float targ_freq_Hz = 1000.0;
        float Hz_per_bin = sample_rate_Hz / (float)Nfft;
        int targ_bin = (int)(targ_freq_Hz / Hz_per_bin + 0.5); //round to closest bin
        float act_freq_Hz = targ_bin*Hz_per_bin;
        int Ichan = 0;
        Serial.println("AudiPath_Sine_wFFT: FFT magnitude at " + String((int)act_freq_Hz) + " Hz for Chan " + String(Ichan) + " = " + String(20.0f*log10f(getFftMag(Ichan,targ_bin))) + " dBFS");
        lastUpdate_millis = millis();
      }

      return return_val;
    }

    virtual void printHelp(void) {
      Serial.println("   m/M: Mute/Unute the tone (cur = " + String(tone1_active ? "enabled" : "muted")  + ")");
      Serial.println("   f/F: Increment/Decrement tone1 frequency (cur = " + String(freq1_Hz) + "Hz)");
      Serial.println("   a/A: Increment/Decrement tone1 amplitude (cur = " + String(20.f*log10f(sine1_amplitude),1) + " dBFS");

    }
    virtual void respondToByte(char c) {
      switch (c) {
        case 'm':
          enableTone(false);
          Serial.println(instanceName + ": muted tone1");
          break;
        case 'M':
          enableTone(true);
          Serial.println(instanceName + ": un-muted tone1");
          break;
        case 'f':
          setFrequency_Hz(freq1_Hz*sqrt(2.0));
          Serial.println(instanceName + ": increased tone1 frequency to " + String(freq1_Hz));
          break;
        case 'F':
          setFrequency_Hz(freq1_Hz/sqrt(2.0));
          Serial.println(instanceName + ": decreased tone1 frequency to " + String(freq1_Hz));
          break;
        case 'a':
          setAmplitude(sine1_amplitude*sqrt(2.0));
          Serial.println(instanceName + ": increased tone1 amplitude to " + String(20.f*log10f(sine1_amplitude),1) + " dBFS");
          break;
        case 'A':
          setAmplitude(sine1_amplitude/sqrt(2.0));
          Serial.println(instanceName + ": decreased tone1 amplitude to " + String(20.f*log10f(sine1_amplitude),1) + " dBFS");
          break;
      }
    }

    float setFrequency_Hz(float _freq_Hz) {
      freq1_Hz = _freq_Hz;
      sineWave1->frequency(freq1_Hz);
      return freq1_Hz;
    }

    float setAmplitude(float _amplitude) {
      sine1_amplitude = _amplitude;
      if (tone1_active) sineWave1->amplitude(sine1_amplitude);
      return sine1_amplitude;
    }

    bool enableTone(bool _enable) {
      tone1_active = _enable;
      if (tone1_active) {
        sineWave1->amplitude(sine1_amplitude);
      } else {
        sineWave1->amplitude(0.0);
      }
      return tone1_active;
    }

    //Gather the queued data and compute the FFT.  Return zero if OK
    int processFFT(int Ichan, AudioRecordQueue_F32 *queue) {
      //Serial.println("AudioPath_Sine_wFFT: processFFT...");
      float *fftMemory = allFftMemory_cmplx[Ichan];  //get pointer to FFT buffer that we're going to use
      if (copyQueuesIntoFftBuffer(queue, fftMemory) != 0) return -1;  //copies from queue to fftMemory.  Releases all queues that are used
      applyFftWindow(fftWindow, fftMemory, Nfft);    //apply window to data prior to computing the FFT
      BTNRH_FFT::cha_fft_rc(fftMemory, Nfft);  //execute the FFT.  From Tympan_Library.  Results are in-place in fftMemory
      for (int I=0; I < 2*(Nfft/2+1); I++) fftMemory[I] /= Nfft;  //normalize values by length of FFT (to get power instad of energy)
      return 0;
    }

    //Create a Hanning window function for use during FFT analysis
    static void computeFftWindow(int N, float *window) {
      if (window == NULL) return;
      for (int i=0; i < N; i++) window[i] = 0.5f*(1.0f - cosf(2.0f*M_PI*(float)i/((float)N)));
    }
    static void applyFftWindow(float *window, float *data, int N) {
      for (int I=0; I < N; I++) data[I] *= window[I];
    }

    //access the FFT results
    virtual int getNfft(void) { return Nfft; }
    virtual float* getFftOutput_cmplx(int Ichan) { 
      if ((Ichan>=0) && (Ichan <= (int)allFftMemory_cmplx.size())) {
        return allFftMemory_cmplx[Ichan];
      } else {
        return NULL;
      }
    }
    virtual float getFftMag(int Ichan, int Ibin) {
      if ((Ichan>=0) && (Ichan <= (int)allFftMemory_cmplx.size())) {
        if ((Ibin >= 0) && (Ibin <= (Nfft/2+1))) {
          float *buff_cmplx = allFftMemory_cmplx[Ichan];
          float real = buff_cmplx[Ibin*2];
          float imag = buff_cmplx[Ibin*2+1];
          float mag = sqrtf( real*real + imag*imag );
          return mag;          
        }
      }
      return 0.0;
    }

  protected:
    //data members for executing our main loop processing only occasionally
    unsigned long int       lastUpdate_millis = 0UL;
    const unsigned long int update_period_millis = 1000UL; //here's how often (milliseconds) we allow our main-loop update to execute

    //data members for the audio queue and FFT analysis
    float                   sample_rate_Hz = 44100.0; //overwritten by constructor
    std::vector<AudioRecordQueue_F32 *> allQueues;    //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    const int               Nfft = 4096;            //requires it to be poewr of 2 and requires it to be an integer multiple of the length of the samples in an audio block
    int                     n_audio_blocks_required = -9999;  //will compute later, when we know the length of the data blocks
    std::vector<float *>    allFftMemory_cmplx;
    float                   *fftWindow = NULL;
        
    //data memebers for the sine wave generation
    AudioSynthWaveform_F32  *sineWave1;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    float                   freq1_Hz = 1000.0f;
    float                   sine1_amplitude = sqrtf(pow10f(0.1*-50.0));
    bool                    tone1_active = false;

    //data memebers for the audio hardware
    float adc_gain_dB = 0.0;            //set input gain, 0-47.5dB in 0.5dB setps
    float dac_gain_dB = 0.0;            //set the DAC gain: -63.6 dB to +24 dB in 0.5dB steps.  
    float headphone_amp_gain_dB = 0.0;  //set the headphone gain: -6 to +14 dB (I think)

    //define private methods
    int copyQueuesIntoFftBuffer(AudioRecordQueue_F32 *queue, float *fftBuffer) {
      if (queue->available() < n_audio_blocks_required) return -1;
      int Isamp_buffer = 0;
      for (int Iblock=0; Iblock < n_audio_blocks_required; Iblock++) {
        audio_block_f32_t *block = queue->getAudioBlock();  //gets pointer to audio block
        for (int Isamp_block = 0; Isamp_block < block->length; Isamp_block++) {
          if (Isamp_buffer < Nfft) fftBuffer[Isamp_buffer++] = block->data[Isamp_block];
        }
        queue->freeBuffer();
      }
      return 0;
    }
};



#endif