#ifndef AudioPath_PassThruGain_PDM_h
#define AudioPath_PassThruGain_PDM_h

#include "AudioPath_PassThruGain_PDM.h"


// ////////////////////////////////////////////////////////////////////////////
//
// For an explanation of AudioPaths, see the explanation in AudioStreamComposite_F32.h
//
// ////////////////////////////////////////////////////////////////////////////


// This AudioPath takes the audio input (all four channels) and applies gain.  This uses the digital PDM mic inputs.
class AudioPath_PassThruGain_PDM : public AudioPath_PassThruGain_Analog {
  public:
      //Constructor
      AudioPath_PassThruGain_PDM(AudioSettings_F32 &_audio_settings) : AudioPath_PassThruGain_Analog(_audio_settings) 
      {
        setupAudioProcessing();
        instanceName = "Audio Pass-Thru PDM";  //"name" is defined as a String in AudioStream_F32
      }
      
    virtual void setupAudioProcessing(void) {
      //Serial.println("AudioPath_PassThruGain_PDM: setupAudioProcessing...");
      AudioPath_PassThruGain_Analog::setupAudioProcessing(); //I don't think that it does anything other than setGain_dB, but let's call it just to be sure
      //Serial.println("AudioPath_PassThruGain_PDM: setting allGains again..." + String((int)allGains.size()));
      for (int i=0; i < (int)allGains.size(); i++) allGains[i]->setGain_dB(25.0);  //increase the gain to handle the low-output of the PDM mics
    }

    virtual void setup_fromSetActive(void) {
      if (tympan_ptr != NULL) {
        tympan_ptr->muteHeadphone(); tympan_ptr->muteDAC(); //try to silence the switching noise/pop...doesn't seem to work.
        tympan_ptr->enableDigitalMicInputs(true);             //switch to digital PDM inputs 
        //tympan_ptr->inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); //Choose the desired input (on-board mics)...does nothing when in PDM mode
        //tympan_ptr->setInputGain_dB(adc_gain_dB);           //set input gain, 0-47.5dB in 0.5dB setps...does nothing when in PDM mode
        tympan_ptr->setDacGain_dB(dac_gain_dB,dac_gain_dB);   //set the DAC gain.  left and right
        tympan_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB,headphone_amp_gain_dB);  //set the headphone amp gain.  left and right       
        tympan_ptr->unmuteDAC();
        tympan_ptr->unmuteDAC();  tympan_ptr->unmuteHeadphone();
      }
      if (shield_ptr != NULL) {
        shield_ptr->muteHeadphone(); shield_ptr->muteDAC(); //try to silence the switching noise/pop...doesn't seem to work.    
        shield_ptr->enableDigitalMicInputs(true);               //switch to digital PDM inputs 
        //shield_ptr->inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); //Choose the desired input...does nothing when in PDM mode
        //shield_ptr->setInputGain_dB(adc_gain_dB);             //set input gain, 0-47.5dB in 0.5dB setps...does nothing when in PDM mode
        shield_ptr->setDacGain_dB(dac_gain_dB,dac_gain_dB);     //set the DAC gain.  left and right
        shield_ptr->setHeadphoneGain_dB(headphone_amp_gain_dB,headphone_amp_gain_dB);  //set the headphone amp gain.  left and right                    
        shield_ptr->unmuteDAC();
        shield_ptr->unmuteDAC();  shield_ptr->unmuteHeadphone();                     
      }
    }
  private:
};


#endif
