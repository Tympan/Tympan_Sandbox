/*
   AudioEffectNFC

   Created: Ira Ray Jenkins, Creare, June 2021
            Chip Audette, OpenAudio, August 2021

   Based on MyAudioAlgorithm by Chip Audette, OpenAudio April 2017
   Purpose: Demonstrate a port of tst_nfc.c from ChaPro to Tympan.

   MIT License.  use at your own risk.
*/

#include <Arduino.h>
#include "AudioStream_F32.h"
//#include <arm_math.h> //ARM DSP extensions.  https://www.keil.com/pack/doc/CMSIS/DSP/html/index.html

#include "chapro.h"
//include "tst_nfc_data.h"

class AudioEffectNFC : public AudioStream_F32
{
public:
    //constructor
    AudioEffectNFC(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32){};

    //here's the method that is called automatically by the Teensy Audio Library
    void update(void)
    {
        if (!enabled) return;
      
        //Serial.println("AudioEffectMine_F32: doing update()");  //for debugging.
        audio_block_f32_t *audio_block;
        audio_block = AudioStream_F32::receiveWritable_f32();
        if (!audio_block) return;

        //do your work
        applyMyAlgorithm(audio_block);

        ///transmit the block and release memory
        AudioStream_F32::transmit(audio_block);
        AudioStream_F32::release(audio_block);
    }

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!11
    // Here is where you can add your algorithm.
    // This function gets called block-wise...which is usually hard-coded to every 128 samples
    void applyMyAlgorithm(audio_block_f32_t *audio_block)
    {
        float *x = audio_block->data;
        int cs = audio_block->length;

        cha_nfc_process(this->cp, x, x, cs);  //see CHAPRO

    } //end of applyMyAlgorithms
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    void setup(CHA_PTR *c)
    {
        set_CHA_PTR(c);
    }
    void set_CHA_PTR(CHA_PTR *c) { cp = c; }
    bool setEnabled(bool val = true) { return enabled = val; }
private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enabled = false;
    CHA_PTR *cp;

}; //end class definition for NFC_DEMO
