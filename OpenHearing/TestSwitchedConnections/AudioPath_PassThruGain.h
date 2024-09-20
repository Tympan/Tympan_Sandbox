#ifndef AudioPath_PassThruGain_h
#define AudioPath_PassThruGain_h

#include <vector>
#include "AudioPath_Base.h"


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioPath_Base.h
//
// ////////////////////////////////////////////////////////////////////////////


// Here is another version of an AudioPath.  This one takes the audio input (in stereo) and applies gain.
class AudioPath_PassThruGain : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_PassThruGain(AudioSettings_F32 &_audio_settings) : AudioPath_Base(_audio_settings) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      for (int i=0; i<4; i++) {
        allGains.push_back( new AudioEffectGain_F32( _audio_settings ) );
        audioObjects.push_back( allGains[i] );
      }
     
      //setup the parameters of the audio processing
      setupAudioProcessing();

      //choose a human-readable name for this audio path
      name = "Audio Pass-Thru";  //"name" is defined as a String in AudioPath_Base
    }

    //~AudioPath_PassThruGain();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords

    //Connect the input of this AudioPath to the given source.
    //For this AudioPath, the Nth input should be connected to the input of the Nth gain block
    virtual int connectToSource(AudioStream_F32 *src, const int source_index, const int audio_path_input_index)
    {
      if (src != NULL) {
         if (audio_path_input_index < allGains.size()) {
           patchCords.push_back( new AudioConnection_F32(*src, audio_path_input_index, *allGains[audio_path_input_index], 0) ); //arguments: source, output index of source, destination, output index of destination
           return 0;
         }
      }
      return -1;  
    }

    //Connct the output of this AudioPath to the given destiation.
    //For this AudioPath, the Nth output should be connected to the output of the Nth gain block
    virtual int connectToDestination(const int audio_path_output_index, AudioStream_F32 *dst, const int dest_index) 
    {
      if (dst != NULL) {
        if (audio_path_output_index < (int)allGains.size()) {
           patchCords.push_back( new AudioConnection_F32(*allGains[audio_path_output_index], 0, *dst, dest_index) ); //arguments: source, output index of source, destination, output index of destination
           return 0;
        }
      }
      return -1;    
    }

    //setupAudioProcess: initialize all gain blocks to a certain gain value
    void setupAudioProcessing(void) {
      for (int i=0; i < (int)allGains.size(); i++) allGains[i]->setGain_dB(10.0);
    }

    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+update_period_millis;
      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        Serial.print("AudioPath_PassThruGain: serviceMainLoop: Gains (dB) = ");
        for (int i=0; i < (int)allGains.size(); i++) { Serial.print(allGains[i]->getGain_dB(),1);  Serial.print(", ");  }
        lastChange_millis = cur_millis;
      }
      return 0;
    }

    protected:
      std::vector<AudioEffectGain_F32 *> allGains;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted  
      unsigned long int       lastChange_millis = 0UL;
      const unsigned long int update_period_millis = 1000UL;
};


#endif
