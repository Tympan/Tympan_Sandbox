#ifndef AudioPath_Sine_h
#define AudioPath_Sine_h

#include <vector>
#include "AudioPath_Base.h"

// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioPath_Base.h
//
// ////////////////////////////////////////////////////////////////////////////


// Here is one version of an AudioPath_Base.  This one creates a sine wave and toggles it on-and-off once per second.
class AudioPath_Sine : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_Sine(AudioSettings_F32 &_audio_settings) : AudioPath_Base(_audio_settings) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( sineWave = new AudioSynthWaveform_F32( _audio_settings ) );
      audioObjects.push_back( gainSine = new AudioEffectGain_F32 ( _audio_settings ) );
      
      // Make all audio connections, except the final one to the destiation...we're only storing them in a vector so that it's easier to destroy them later
      patchCords.push_back( new AudioConnection_F32(*sineWave, 0, *gainSine, 0)); //sine wave to gain module

      //setup the parameters of the audio processing
      setupAudioProcessing(); 

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
    void setupAudioProcessing(void) {
      sineWave->frequency(1000.0f);
      sineWave->amplitude(sine_amplitude);
      tone_active = true;
      gainSine->setGain_dB(0.0); //set to no gain for now
      lastChange_millis = millis();
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
          Serial.print("AudioPath_Sine: serviceMainLoop: setting tone amplitude to "); Serial.println(0.0f,3);
          sineWave->amplitude(0.0f);
        } else {
          //turn on the tone
          tone_active = true;
          Serial.print("AudioPath_Sine: serviceMainLoop: setting tone amplitude to "); Serial.println(sine_amplitude,3);
          sineWave->amplitude(sine_amplitude);
        }
        lastChange_millis = cur_millis;
      } else {
        //not enough time has passed.  do nothing.
      }
      return 0;
    }
    
  protected:
    AudioSynthWaveform_F32  *sineWave;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    AudioEffectGain_F32     *gainSine;      //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted   
    const float             sine_amplitude = 0.003f;
    unsigned long int       lastChange_millis = 0UL;
    const unsigned long int tone_dur_millis = 1000UL;
    const unsigned long int silence_dur_millis = 1000UL;
    bool                    tone_active = false;

};


#endif
