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

//include "chapro.h"
//include "test_gha.h"

class AudioEffectBTNRH : public AudioStream_F32
{
public:
    //constructor
    AudioEffectBTNRH(const AudioSettings_F32 &settings) : AudioStream_F32(1, inputQueueArray_f32){};

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

        //call the relevant processing function from CHAPRO
        process_chunk(cp, x, x, cs); //see test_gha.h  (or whatever test_xxxx.h is #included at the top

    } //end of applyMyAlgorithms
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    void set_CHA_PTR(CHA_PTR c) { cp = c; }
    bool setEnabled(bool val = true) { return enabled = val; }
private:
    //state-related variables
    audio_block_f32_t *inputQueueArray_f32[1]; //memory pointer for the input to this module
    bool enabled = false;
    CHA_PTR cp;

}; //end class definition for AudioEffectBTNRH
