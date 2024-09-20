#ifndef AudioPath_Sine_h
#define AudioPath_Sine_h

#include <vector>
#include "AudioPath_Base.h"


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioPath_Base.h
//
// ////////////////////////////////////////////////////////////////////////////


// This AudioPath creates a sine wave and toggles it on-and-off once per second.
class AudioPath_Sine : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_Sine(AudioSettings_F32 &_audio_settings, Tympan *_tympan_ptr, EarpieceShield *_shield_ptr)  : AudioPath_Base(_audio_settings, _tympan_ptr, _shield_ptr) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( sineWave = new AudioSynthWaveform_F32( _audio_settings ) );
      audioObjects.push_back( gainSine = new AudioEffectGain_F32 ( _audio_settings ) );
      
      // Make all audio connections, except the final one to the destiation...we're only storing them in a vector so that it's easier to destroy them later
      patchCords.push_back( new AudioConnection_F32(*sineWave, 0, *gainSine, 0)); //sine wave to gain module

      //setup the parameters of the audio processing
      setupAudioProcessing(); 

      //initialize to being inactive
      setActive(false);  //setActive is in AudioPath_Base

      //choose a human-readable name for this audio path
      name = "Sine Generator";  //"name" is defined as a String in AudioPath_Base
    }

    //~AudioPath_Sine();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords

    //Connct the input of this AudioPath to the given source.
    //This AudioPath just creates a sine tone, so no inputs are needed or used
    virtual int connectToSource(AudioStream_F32 *src, const int source_index, const int audio_path_input_index)   {  return 0;  }

    //Connect the output of this AudioPath to the given destiation.
    //For this AudioPath, all outputs should come from our single sine generator
    virtual int connectToDestination(const int audio_path_output_index, AudioStream_F32 *dst, const int dest_index) 
    {
      if (dst != NULL) {
         patchCords.push_back( new AudioConnection_F32(*gainSine, 0, *dst, dest_index) ); //arguments: source, output index of source, destination, output index of destination
         return 0;
      }
      return -1;    
    }

    //setupAudioProcess: initialize the sine wave to the desired frequency and amplitude
    virtual void setupAudioProcessing(void) {
      setFrequency_Hz(freq_Hz);
      setAmplitude(sine_amplitude);
      tone_active = true;
      gainSine->setGain_dB(0.0); //set to no gain for now
      lastChange_millis = millis();
    }

    //setup the hardware.  This is called automatically by AudioPath_Base::setActive(flag_active) whenever flag_active = true
    virtual void setupHardware(void) {
      if (tympan_ptr != NULL) {
        tympan_ptr->muteDAC();
        //tympan_ptr->inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); //Choose the desired input and set volume levels (on-board mics)
        //tympan_ptr->setInputGain_dB(input_gain_dB);       //set input volume, 0-47.5dB in 0.5dB setps
        tympan_ptr->setDacGain_dB(dac_gain_dB,dac_gain_dB); //set the DAC gain.  left and right
        //tympan_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB,headphone_amp_gain_dB);  //set the headphone amp gain.  left and right       
        tympan_ptr->unmuteDAC();
        //tympan_ptr->unmuteHeadphone();
      }
      if (shield_ptr != NULL) {
        shield_ptr->muteDAC();
        //shield_ptr->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //Choose the desired input and set volume levels (on-board mics)
        //shield_ptr->setInputGain_dB(input_gain_dB);       //set input volume, 0-47.5dB in 0.5dB setps
        shield_ptr->setDacGain_dB(dac_gain_dB,dac_gain_dB);   //set the DAC gain.  left and right
        //shield_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB,headphone_amp_gain_dB);  //set the headphone amp gain.  left and right                    
        shield_ptr->unmuteDAC();
        //shield_ptr->unmuteHeadphone();                     
      }
    }

    //The sine wave should toggle itself to be audible then silent then audible then silent, etc.
    //The serviceMainLoop implements this periodic on/off behavior.
    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+tone_dur_millis;
      if (tone_active == false) { targ_time_millis = lastChange_millis + silence_dur_millis; }

      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        if (tone_active) {
          //turn off the tone
          tone_active = false;
          Serial.println("AudioPath_Sine: serviceMainLoop: Muting tone.");
          sineWave->amplitude(0.0f);
        } else {
          //turn on the tone
          tone_active = true;
          Serial.println("AudioPath_Sine: serviceMainLoop: Activating tone (" + String(20.f*log10f(sine_amplitude),1) + " dBFS)");
          sineWave->amplitude(sine_amplitude);
        }
        lastChange_millis = cur_millis;
      } else {
        //not enough time has passed.  do nothing.
      }
      return 0;
    }
    
    virtual void printHelp(void) {
      Serial.println("   f/F: Increment/Decrement the tone frequency (cur = " + String(freq_Hz) + "Hz)");
      Serial.println("   a/A: Increment/Decrement the tone amplitude (cur = " + String(20.f*log10f(sine_amplitude),1) + " dBFS");
    }
    virtual void respondToByte(char c) {
      switch (c) {
        case 'f':
          setFrequency_Hz(freq_Hz*sqrt(2.0));
          Serial.println(name + ": increased tone frequency to " + String(freq_Hz));
          break;
        case 'F':
          setFrequency_Hz(freq_Hz/sqrt(2.0));
          Serial.println(name + ": decreased tone frequency to " + String(freq_Hz));
          break;
        case 'a':
          setAmplitude(sine_amplitude*sqrt(2.0));
          Serial.println(name + ": increased tone amplitude to " + String(20.f*log10f(sine_amplitude),1) + " dBFS");
          break;
        case 'A':
          setAmplitude(sine_amplitude/sqrt(2.0));
          Serial.println(name + ": decreased tone amplitude to " + String(20.f*log10f(sine_amplitude),1) + " dBFS");
          break;
      }
    }
    float setFrequency_Hz(float _freq_Hz) {
      freq_Hz = _freq_Hz;
      sineWave->frequency(freq_Hz);
      return freq_Hz;
    }
    float setAmplitude(float _amplitude) {
      sine_amplitude = _amplitude;
      if (tone_active) sineWave->amplitude(sine_amplitude);
      return sine_amplitude;
    }

  protected:
    AudioSynthWaveform_F32  *sineWave;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    AudioEffectGain_F32     *gainSine;      //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    float                   freq_Hz = 1000.0f;
    float                   sine_amplitude = sqrtf(pow10f(0.1*-50.0));
    unsigned long int       lastChange_millis = 0UL;
    const unsigned long int tone_dur_millis = 1000UL;
    const unsigned long int silence_dur_millis = 1000UL;
    bool                    tone_active = false;

    //settings for the audio hardware
    //float adc_gain_dB = 0.0;            //set input gain, 0-47.5dB in 0.5dB setps
    float dac_gain_dB = 0.0;            //set the DAC gain: -63.6 dB to +24 dB in 0.5dB steps.  
    float headphone_amp_gain_dB = 0.0;  //set the headphone gain: -6 to +14 dB (I think)

};


#endif
