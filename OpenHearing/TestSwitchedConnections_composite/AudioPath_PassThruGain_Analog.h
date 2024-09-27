#ifndef AudioPath_PassThruGain_Analog_h
#define AudioPath_PassThruGain_Analog_h

#include <vector>
#include "AudioStreamComposite_F32.h"


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioStreamComposite_F32.h
//
// ////////////////////////////////////////////////////////////////////////////


// This AudioPath takes the audio input (all four channels) and applies gain.  This uses the analog inputs.
class AudioPath_PassThruGain_Analog : public AudioStreamComposite_F32 {
  public:
    //Constructor
    AudioPath_PassThruGain_Analog(AudioSettings_F32 &_audio_settings)  : AudioStreamComposite_F32(_audio_settings) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( startNode = new AudioSwitchMatrix4_F32( _audio_settings ) ); startNode->instanceName = String("Input Matrix"); //per AudioStreamComposite_F32, always have this first
      for (int i=0; i < 4; i++) {
        allGains.push_back( new AudioEffectGain_F32( _audio_settings ) ); 
        allGains[i]->instanceName = String("Gain" + String(i)); //give a human readable name to help Chip's debugging of startup issues
        audioObjects.push_back( allGains[i] );
      }
      audioObjects.push_back( endNode = new AudioForwarder4_F32( _audio_settings, this ) ); endNode->instanceName = String("Output Forwarder"); //per AudioStreamComposite_F32, always have this last

      //connect the audio classes
      for (int I=0; I < (int)allGains.size(); I++)  patchCords.push_back( new AudioConnection_F32(*startNode,   I, *allGains[I], 0)); //connect inputs to the gain objects
      for (int I=0; I < (int)allGains.size(); I++)  patchCords.push_back( new AudioConnection_F32(*allGains[I], 0, *endNode,     I)); //connect gain objects to the outputs
     
      //setup the parameters of the audio processing
      setupAudioProcessing();

      //initialize to being inactive
      setActive(false);  //setActive is in AudioStreamComposite_F32

      //choose a human-readable name for this audio path
      instanceName = "Audio Pass-Thru Analog";  //"name" is defined as a String in AudioStream_F32
    }

    //~AudioPath_PassThruGain();  //using destructor from AudioStreamComposite_F32, which destorys everything in audioObjects and in patchCords

     //setupAudioProcess: initialize all gain blocks to a certain gain value
    virtual void setupAudioProcessing(void) {
      //Serial.println("AudioPath_PassThruGain_Analog: setupAudioProcessing...");
      for (int i=0; i < (int)allGains.size(); i++) allGains[i]->setGain_dB(10.0);
    }

    //setup the hardware.  This is called automatically by AudioStreamComposite_F32::setActive(flag_active) whenever flag_active = true
    virtual void setup_fromSetActive(void) {
      Serial.println("AudioPath_Sine: setup_fromSetActive: tympan_ptr = " + String((int)tympan_ptr) + ", shield_ptr = " + String((int)shield_ptr));
      if (tympan_ptr != NULL) {
        tympan_ptr->muteHeadphone(); tympan_ptr->muteDAC(); //try to silence the switching noise/pop...doesn't seem to work.
        tympan_ptr->enableDigitalMicInputs(false);          //switch to analog inputs 
        tympan_ptr->inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); //Choose the desired input (on-board mics)
        tympan_ptr->setInputGain_dB(adc_gain_dB);           //set input gain, 0-47.5dB in 0.5dB setps
        tympan_ptr->setDacGain_dB(dac_gain_dB, dac_gain_dB); //set the DAC gain.  left and right
        tympan_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB, headphone_amp_gain_dB);  //set the headphone amp gain.  left and right       
        tympan_ptr->unmuteDAC();  tympan_ptr->unmuteHeadphone();
      }
      if (shield_ptr != NULL) {
        shield_ptr->muteHeadphone(); shield_ptr->muteDAC(); //try to silence the switching noise/pop...doesn't seem to work. 
        shield_ptr->enableDigitalMicInputs(false);            //switch to analog inputs 
        shield_ptr->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //Choose the desired input  (no on-board mics, so use pink input jack as line ine)
        shield_ptr->setInputGain_dB(adc_gain_dB);             //set input gain, 0-47.5dB in 0.5dB setps
        shield_ptr->setDacGain_dB(dac_gain_dB, dac_gain_dB);   //set the DAC gain.  left and right
        shield_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB, headphone_amp_gain_dB);  //set the headphone amp gain.  left and right                    
        shield_ptr->unmuteDAC();  shield_ptr->unmuteHeadphone();              
      }
      Serial.println("AudioPath_PassThruGain_Analog: setup_fromSetActive(): done");
    }

    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+update_period_millis;
      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        Serial.print(instanceName);  //instanceName is in AudioStream_F32
        Serial.print(": serviceMainLoop: Gains (dB) = ");
        for (int i=0; i < (int)allGains.size(); i++) { Serial.print(allGains[i]->getGain_dB(),1);  Serial.print(", ");  }
        Serial.println();
        lastChange_millis = cur_millis;
      }
      return 0;
    }

  protected:
    std::vector<AudioEffectGain_F32 *> allGains;  //if you use the audioObjects vector from AudioPathBase, any allocation to this pointer will get automatically deleted  
    unsigned long int       lastChange_millis = 0UL;
    const unsigned long int update_period_millis = 1000UL; //here's how often (milliseconds) we allow our main-loop update to execute

    //settings for the audio hardware
    float adc_gain_dB = 0.0;            //set input gain, 0-47.5dB in 0.5dB setps
    float dac_gain_dB = 0.0;            //set the DAC gain: -63.6 dB to +24 dB in 0.5dB steps.  
    float headphone_amp_gain_dB = 0.0;  //set the headphone gain: -6 to +14 dB (I think)
};


#endif
