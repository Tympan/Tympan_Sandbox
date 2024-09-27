#ifndef AudioPath_Sine_h
#define AudioPath_Sine_h

#include <vector>
#include "AudioStreamComposite_F32.h"


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioStreamComposite_F32.h
//
// ////////////////////////////////////////////////////////////////////////////


// This AudioPath creates a sine wave and toggles it on-and-off once per second.
class AudioPath_Sine : public AudioStreamComposite_F32 {
  public:
    //Constructor
    AudioPath_Sine(AudioSettings_F32 &_audio_settings)  : AudioStreamComposite_F32(_audio_settings) {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( startNode = new AudioSwitchMatrix4_F32( _audio_settings ) ); startNode->instanceName = String("Input Matrix");  //per AudioStreamComposite_F32, always have this first
      audioObjects.push_back( sineWave  = new AudioSynthWaveform_F32( _audio_settings ) ); sineWave->instanceName  = String("Sine Wave");     //give a human readable name to help Chip's debugging of startup issues
      audioObjects.push_back( endNode   = new AudioForwarder4_F32( _audio_settings, this ) ); endNode->instanceName = String("Output Forwarder"); //per AudioStreamComposite_F32, always have this last
      
      // Make all audio connections, except the final one to the destiation...we're only storing them in a vector so that it's easier to destroy them later
      for (int Ioutput=0; Ioutput < 4; Ioutput++)  patchCords.push_back( new AudioConnection_F32(*sineWave, 0, *endNode, Ioutput)); //same sine to all outputs

      //setup the parameters of the audio processing
      setupAudioProcessing(); 

      //initialize to being inactive
      setActive(false);  //setActive is in AudioStreamComposite_F32

      //choose a human-readable name for this audio path
      instanceName = "Sine Generator";  //"name" is defined as a String in AudioStream_F32
    }

    //~AudioPath_Sine();  //using destructor from AudioStreamComposite.h, which destorys everything in audioObjects and in patchCords

    //setupAudioProcess: initialize the sine wave to the desired frequency and amplitude
    virtual void setupAudioProcessing(void) {
      setFrequency_Hz(freq_Hz);
      setAmplitude(sine_amplitude);
      tone_active = true;
      lastChange_millis = millis();
    }

    //setup the hardware.  This is called automatically by AudioStreamComposite_F32::setActive(flag_active) whenever flag_active = true
    virtual void setup_fromSetActive(void) {
      Serial.println("AudioPath_Sine: setup_fromSetActive: tympan_ptr = " + String((int)tympan_ptr) + ", shield_ptr = " + String((int)shield_ptr));
      if (tympan_ptr != NULL) {
        tympan_ptr->muteHeadphone(); tympan_ptr->muteDAC();
        //tympan_ptr->inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); //Choose the desired input and set volume levels (on-board mics)
        //tympan_ptr->setInputGain_dB(input_gain_dB);       //set input volume, 0-47.5dB in 0.5dB setps
        tympan_ptr->setDacGain_dB(dac_gain_dB,dac_gain_dB); //set the DAC gain.  left and right
        tympan_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB,headphone_amp_gain_dB);  //set the headphone amp gain.  left and right     
        tympan_ptr->unmuteDAC(); tympan_ptr->unmuteHeadphone();
      }
      if (shield_ptr != NULL) {
        shield_ptr->muteHeadphone(); shield_ptr->muteDAC();
        //shield_ptr->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //Choose the desired input and set volume levels (on-board mics)
        //shield_ptr->setInputGain_dB(input_gain_dB);       //set input volume, 0-47.5dB in 0.5dB setps
        shield_ptr->setDacGain_dB(dac_gain_dB,dac_gain_dB);   //set the DAC gain.  left and right
        shield_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB,headphone_amp_gain_dB);  //set the headphone amp gain.  left and right      
        shield_ptr->unmuteDAC(); shield_ptr->unmuteHeadphone();                     
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
          Serial.println(instanceName + ": increased tone frequency to " + String(freq_Hz));
          break;
        case 'F':
          setFrequency_Hz(freq_Hz/sqrt(2.0));
          Serial.println(instanceName + ": decreased tone frequency to " + String(freq_Hz));
          break;
        case 'a':
          setAmplitude(sine_amplitude*sqrt(2.0));
          Serial.println(instanceName + ": increased tone amplitude to " + String(20.f*log10f(sine_amplitude),1) + " dBFS");
          break;
        case 'A':
          setAmplitude(sine_amplitude/sqrt(2.0));
          Serial.println(instanceName + ": decreased tone amplitude to " + String(20.f*log10f(sine_amplitude),1) + " dBFS");
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
    //data members for generating the sine wave
    AudioSynthWaveform_F32  *sineWave;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted
    float                   freq_Hz = 1000.0f;
    float                   sine_amplitude = sqrtf(pow10f(0.1*-50.0));
    const unsigned long int tone_dur_millis = 1000UL;
    const unsigned long int silence_dur_millis = 1000UL;
    bool                    tone_active = false;

    //data members for tracking last change in the sine wave amplitude
    unsigned long int       lastChange_millis = 0UL;

    //data members for the audio hardware
    //float adc_gain_dB = 0.0;            //set input gain, 0-47.5dB in 0.5dB setps
    float dac_gain_dB = 0.0;            //set the DAC gain: -63.6 dB to +24 dB in 0.5dB steps.  
    float headphone_amp_gain_dB = 0.0;  //set the headphone gain: -6 to +14 dB (I think)

};


#endif
