
#ifndef AudioPaths_h
#define AudioPaths_h

#include <vector>

// AudioPath_Base:
//
// This is a base class intended to provide an interface (and some common functionality) for derived AudioPath classes
// that are actually employed by the user.
//
// The idea is that an AudioPath is a composite class that instantiates and manages a whole chain of AudioStream_F32
// audio-processing objects.  One can simply instantiate an AudioPath and all of the constituant objects and connections
// are created.  The only thing left to do is connect the final output.
//
// Note that the final output is treated separately because the final output AudioStream_F32 object should be instantiated
// last, after *all* of the AudioPath instances are created.  This ensures that the output is serviced (behind the scenes,
// by AudioStrea::update_all()) after all of the AudioPath continuents.  This minimizes audio latency through the system.
// 
// So, in your overall program, you should instantiate your audio source (AudioInputI2S_F32, for example), then you
// should instantiate all of the your AudioPath objects, then you should instantiate your output desitnation (AudioMixer8_F32
// and AudioOutputI2S_f32, for example)
//
class AudioPath_Base {
  public:
    AudioPath_Base(void) {}
    AudioPath_Base(AudioSettings_F32 &_audio_settings, AudioStream_F32 *_src) 
    { 
      if (_src != NULL) src = _src; 
    }
    
    //Destructor: note that destroying Audio Objects is not a good idea because AudioStream (from Teensy)
    //does not currently support (as of Sept 9, 2024) having instances of AudioStream be destroyed.  The
    //problem is that AudioStream keeps pointers to all AudioStream instances ever created.  If an AudioStream
    //instance is deleted, AudioStream does not remove the instance from its lists.  So, it has stale pointers
    //that eventually take down the system.
    //So, it is *not* recommended that you destroy AudioStream objects or, therefore, AudioPath objects
    virtual ~AudioPath_Base() {
      for (int i=(int)patchCords.size()-1; i>=0; i--) delete patchCords[i]; //destroy all the Audio Connections (in reverse order...maybe less memroy fragmentation?)
      Serial.println("AudioPath_Base: Destructor: *** WARNING ***: destroying AudioStream instances, which is not supported by Teensy's AudioStream.");
      for (int i=(int)audioObjects.size()-1; i>=0; i--) delete audioObjects[i];       //destroy all the audio class instances (in reverse order...maybe less memroy fragmentation?)
    }

    //connect the last-created audio object to the given destination object
    // virtual int connectToDestination(const int source_index, AudioStream_F32 *_dst, const int dest_index) {
    //   if (_dst != NULL) {
    //     dst = _dst;
    //     patchCords.push_back( new AudioConnection_F32(*(audioObjects.back()), source_index, *dst, dest_index) ); //route right input to right output
    //     return 0;
    //   }
    //   return -1;
    // }
    virtual int connectToDestination(const int source_index, AudioStream_F32 *_dst, const int dest_index) = 0;

    //Loop through each audio object and set them all to the desired state of active or inactive.
    //Per the rules of AudioStream::update_all(), active = false should mean that the audio object
    //is not invoked, thereby saving CPU.
    virtual bool setActive(bool _active) { 
      for (auto & audioObject : audioObjects) { audioObject->setActive(_active); } //iterate over each audio object and set whether it is active. (setting to zero reduces CPU)
      return _active;
    }
  
    //Interfact to allow for any slower main-loop updates
    virtual int serviceMainLoop(void) { return 0; }

  protected:
    AudioStream_F32 *src=NULL, *dst=NULL;
    std::vector<AudioConnection_F32 *> patchCords;
    std::vector<AudioStream_F32 *> audioObjects;
};


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Here is one version of an AudioPath.  This one creates a sine wave and toggles it on-and-off once per second.
class AudioPath_Sine : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_Sine(AudioSettings_F32 &_audio_settings, AudioStream_F32 *_src) : AudioPath_Base(_audio_settings, _src) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( sineWave = new AudioSynthWaveform_F32( _audio_settings ) );
      audioObjects.push_back( gainSine = new AudioEffectGain_F32 ( _audio_settings ) );
      
      // Make all audio connections, except the final one to the destiation...we're only storing them in a vector so that it's easier to destroy them later
      patchCords.push_back( new AudioConnection_F32(*sineWave, 0, *gainSine, 0)); //sine wave to gain module

      //setup the parameters of the audio processing
      setupAudioProcessing(); 
    }

    //~AudioPath_Sine();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords

    virtual int connectToDestination(const int source_index, AudioStream_F32 *_dst, const int dest_index) 
    {
      if (_dst != NULL) {
         dst = _dst;
         patchCords.push_back( new AudioConnection_F32(*gainSine, 0, *dst, dest_index) ); //route right input to right output
         return 0;
      }
      return -1;    
    }

    void setupAudioProcessing(void) {
      sineWave->frequency(1000.0f);
      sineWave->amplitude(sine_amplitude);
      tone_active = true;
      gainSine->setGain_dB(0.0); //set to no gain for now
      lastChange_millis = millis();
    }

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


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Here is another version of an AudioPath.  This one takes the audio input (in stereo) and applies gain.
class AudioPath_PassThruGain : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_PassThruGain(AudioSettings_F32 &_audio_settings, AudioStream_F32 *_src) : AudioPath_Base(_audio_settings, _src) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( gainL = new AudioEffectGain_F32( _audio_settings ) );
      audioObjects.push_back( gainR = new AudioEffectGain_F32( _audio_settings ) );

      //make audio connections..we're only storing them in a vector so that it's easier to destroy them later
      if (src != NULL) {
        patchCords.push_back( new AudioConnection_F32(*src,   0, *gainL,  0));  //left input to gainL
        patchCords.push_back( new AudioConnection_F32(*src,   1, *gainR,  0));  //right input to gainR
      } else {
        Serial.println("AudioPath_PassThruGain: ***ERROR***: source (src) is NULL.  Cannot create in-coming AudioConnections.");
      }
      
      //setup the parameters of the audio processing
      setupAudioProcessing();
    }

    //~AudioPath_PassThruGain();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords

    virtual int connectToDestination(const int source_index, AudioStream_F32 *_dst, const int dest_index) 
    {
      if (_dst != NULL) {
         dst = _dst;
         if (source_index == 0) {
           patchCords.push_back( new AudioConnection_F32(*gainL, 0, *dst, dest_index) ); //route right input to right output
           return 0;
         } else if (source_index == 1) {
           patchCords.push_back( new AudioConnection_F32(*gainR, 0, *dst, dest_index) ); //route right input to right output
           return 0; 
         }
      }
      return -1;    
    }

    void setupAudioProcessing(void) {
      gainL->setGain_dB(10.0);
      gainR->setGain_dB(10.0);
    }

    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+update_period_millis;
      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        Serial.println("AudioPath_PassThruGain: serviceMainLoop: Gain L (dB) = " + String(gainL->getGain_dB()) + ", Gain R (dB) = " + String(gainR->getGain_dB()));
        lastChange_millis = cur_millis;
      }
      return 0;
    }

    protected:
      AudioEffectGain_F32 *gainL;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted   
      AudioEffectGain_F32 *gainR;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted   
      unsigned long int       lastChange_millis = 0UL;
      const unsigned long int update_period_millis = 1000UL;
};

#endif

