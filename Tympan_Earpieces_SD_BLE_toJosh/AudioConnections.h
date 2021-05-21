
#include "EarpieceMixer_F32.h"

// Define audio objects
AudioInputI2SQuad_F32         i2s_in(audio_settings);        //Digital audio *from* the Tympan AIC.
EarpieceMixer_F32             earpieceMixer(audio_settings);  //mixes earpiece mics, allows switching to analog inputs, mixes left+right, etc
AudioEffectGain_F32           gain[2];   
AudioOutputI2SQuad_F32        i2s_out(audio_settings);       //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default

// ////////////////////////////  setup the audio connections

//start by connecting the I2S input to the earpiece mixer
AudioConnection_F32           patchcord1(i2s_in,0,earpieceMixer,0);
AudioConnection_F32           patchcord2(i2s_in,1,earpieceMixer,1);
AudioConnection_F32           patchcord3(i2s_in,2,earpieceMixer,2);
AudioConnection_F32           patchcord4(i2s_in,3,earpieceMixer,3);

//connect the left and right outputs of the earpiece mixer to the two gain modules (one for left, one for right)
AudioConnection_F32           patchcord11(earpieceMixer,earpieceMixer.LEFT,gain[LEFT],0);
AudioConnection_F32           patchcord12(earpieceMixer,earpieceMixer.RIGHT,gain[RIGHT],0);

//Connect the gain modules to the outputs so that you hear the audio
AudioConnection_F32           patchcord31(gain[LEFT],  0, i2s_out, EarpieceShield::OUTPUT_LEFT_TYMPAN);   //First AIC, left output
AudioConnection_F32           patchcord32(gain[RIGHT], 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_TYMPAN); //First AIC, right output
AudioConnection_F32           patchcord33(gain[LEFT],  0, i2s_out, EarpieceShield::OUTPUT_LEFT_EARPIECE); //Second AIC (Earpiece!), left output
AudioConnection_F32           patchcord34(gain[RIGHT], 0, i2s_out, EarpieceShield::OUTPUT_RIGHT_EARPIECE); //Secibd AIC (Earpiece!), right output

//Connect the raw inputs to the SD card for recording the raw microphone audio
AudioConnection_F32           patchcord41(i2s_in, EarpieceShield::PDM_LEFT_FRONT, audioSDWriter, 0);   //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord42(i2s_in, EarpieceShield::PDM_LEFT_REAR,  audioSDWriter, 1);    //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord43(i2s_in, EarpieceShield::PDM_RIGHT_FRONT,audioSDWriter, 2);  //connect Raw audio to queue (to enable SD writing)
AudioConnection_F32           patchcord44(i2s_in, EarpieceShield::PDM_RIGHT_REAR, audioSDWriter, 3);   //connect Raw audio to queue (to enable SD writing)
