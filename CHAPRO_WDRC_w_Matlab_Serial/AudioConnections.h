

//create audio objects
AudioInputI2SQuad_F32   audio_in(audio_settings);
EarpieceMixer_F32_UI    earpieceMixer(audio_settings); //mixes earpiece mics, allows switching to analog inputs, mixes left+right, etc
AudioEffectBTNRH_F32    BTNRH_alg1(audio_settings);    //see tab "AudioEffectBTNRH.h"
AudioEffectGain_F32     gain1(audio_settings);         //added gain block to easily increase or lower the gain
AudioOutputI2SQuad_F32  audio_out(audio_settings);
AudioSDWriter_F32_UI    audioSDWriter(audio_settings); //this is 2-channels of audio by default, but can be changed to 4 in setup()


//connect the inputs to the earpiece mixer
AudioConnection_F32   patchCord1(audio_in, 0, earpieceMixer, 0);
AudioConnection_F32   patchCord2(audio_in, 1, earpieceMixer, 1);
AudioConnection_F32   patchCord3(audio_in, 2, earpieceMixer, 2);
AudioConnection_F32   patchCord4(audio_in, 3, earpieceMixer, 3);

//connect to BTNRH algorithm
AudioConnection_F32   patchCord6(earpieceMixer, earpieceMixer.LEFT, BTNRH_alg1, 0);
AudioConnection_F32   patchCord7(BTNRH_alg1, 0, gain1, 0);

//connect the BTNRH alg to the outputs
AudioConnection_F32   patchCord11(gain1, 0, audio_out,  EarpieceShield::OUTPUT_LEFT_TYMPAN);    //Tympan AIC, left output
AudioConnection_F32   patchCord12(gain1, 0, audio_out,  EarpieceShield::OUTPUT_RIGHT_TYMPAN);   //Tympan AIC, right output
AudioConnection_F32   patchCord13(gain1, 0, audio_out,  EarpieceShield::OUTPUT_LEFT_EARPIECE);  //Shield AIC, left output
AudioConnection_F32   patchCord14(gain1, 0, audio_out,  EarpieceShield::OUTPUT_RIGHT_EARPIECE); //Shield AIC, right output

//connect to the SD writer
AudioConnection_F32   patchCord21(earpieceMixer, earpieceMixer.LEFT, audioSDWriter,  0);  //left will be the raw input
AudioConnection_F32   patchCord22(gain1,    0, audioSDWriter,  1);  //right will be the processed output
