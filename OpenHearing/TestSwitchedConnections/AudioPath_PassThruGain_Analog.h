#ifndef AudioPath_PassThruGain_Analog_h
#define AudioPath_PassThruGain_Analog_h

#include <vector>
#include "AudioPath_Base.h"


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioPath_Base.h
//
// ////////////////////////////////////////////////////////////////////////////


// This AudioPath takes the audio input (all four channels) and applies gain.  This uses the analog inputs.
class AudioPath_PassThruGain_Analog : public AudioPath_Base {
  public:
    //Constructor
    AudioPath_PassThruGain_Analog(AudioSettings_F32 &_audio_settings, Tympan *_tympan_ptr, EarpieceShield *_shield_ptr)  : AudioPath_Base(_audio_settings, _tympan_ptr, _shield_ptr) 
    {
      //instantiate audio classes...we're only storing them in a vector so that it's easier to destroy them later (whenver destruction is allowed by AudioStream)
      audioObjects.push_back( startNode = new AudioSwitchMatrix4_F32( _audio_settings ) ); startNode->instanceName = String("Input Matrix"); //per AudioPath_Base, always have this first
      for (int i=0; i  <4; i++) {
        allGains.push_back( new AudioEffectGain_F32( _audio_settings ) ); 
        allGains[i]->instanceName = String("Gain" + String(i)); //give a human readable name to help Chip's debugging of startup issues
        audioObjects.push_back( allGains[i] );
      }
      audioObjects.push_back( endNode = new AudioSwitchMatrix4_F32( _audio_settings ) ); endNode->instanceName = String("Output Matrix"); //per AudioPath_Base, always have this last

      //connect the audio classes
      for (int I=0; I < (int)allGains.size(); I++)  patchCords.push_back( new AudioConnection_F32(*startNode,   I, *allGains[I], 0)); //connect inputs to the gain objects
      for (int I=0; I < (int)allGains.size(); I++)  patchCords.push_back( new AudioConnection_F32(*allGains[I], 0, *endNode,     I)); //connect gain objects to the outputs
     
      //setup the parameters of the audio processing
      setupAudioProcessing();

      //initialize to being inactive
      setActive(false);  //setActive is in AudioPath_Base

      //choose a human-readable name for this audio path
      name = "Audio Pass-Thru Analog";  //"name" is defined as a String in AudioPath_Base
    }

    //~AudioPath_PassThruGain();  //using destructor from AudioPath_Base, which destorys everything in audioObjects and in patchCords

    // //Connect the input of this AudioPath to the given source.
    // //For this AudioPath, the Nth input should be connected to the input of the Nth gain block
    // virtual int connectToSource(AudioStream_F32 *src, const int source_index, const int audio_path_input_index)
    // {
    //   if (src != NULL) {
    //      if (audio_path_input_index < (int)allGains.size()) {
    //        patchCords.push_back( new AudioConnection_F32(*src, audio_path_input_index, *allGains[audio_path_input_index], 0) ); //arguments: source, output index of source, destination, output index of destination
    //        return 0;
    //      }
    //   }
    //   return -1;  
    // }

    // //Connct the output of this AudioPath to the given destiation.
    // //For this AudioPath, the Nth output should be connected to the output of the Nth gain block
    // virtual int connectToDestination(const int audio_path_output_index, AudioStream_F32 *dst, const int dest_index) 
    // {
    //   if (dst != NULL) {
    //     if (audio_path_output_index < (int)allGains.size()) {
    //        patchCords.push_back( new AudioConnection_F32(*allGains[audio_path_output_index], 0, *dst, dest_index) ); //arguments: source, output index of source, destination, output index of destination
    //        return 0;
    //     }
    //   }
    //   return -1;    
    // }

    //setupAudioProcess: initialize all gain blocks to a certain gain value
    virtual void setupAudioProcessing(void) {
      //Serial.println("AudioPath_PassThruGain_Analog: setupAudioProcessing...");
      for (int i=0; i < (int)allGains.size(); i++) allGains[i]->setGain_dB(10.0);
    }

    //setup the hardware.  This is called automatically by AudioPath_Base::setActive(flag_active) whenever flag_active = true
    virtual void setupHardware(void) {
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
    }

    virtual int serviceMainLoop(void) {
      unsigned long int cur_millis = millis();
      unsigned long int targ_time_millis = lastChange_millis+update_period_millis;
      if ((cur_millis < lastChange_millis) || (cur_millis > targ_time_millis)) { //also catches wrap-around of millis()
        Serial.print(name);
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
