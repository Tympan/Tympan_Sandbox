AudioInputI2SQuad_F32      i2s_in(audio_settings);         //Bring audio in
AudioSynthToneSweepExp_F32 chirp(audio_settings);          //from the Tympan_Library (synth_tonesweep_F32.h)
AudioSDPlayer_F32_logging  audioSDPlayer(&sd, audio_settings);  //from the Tympan_Library
AudioMixer4_F32            audioMixerL(audio_settings);    //from the Tympan_Library
AudioMixer4_F32            audioMixerR(audio_settings);    //from the Tympan_Library
AudioOutputI2SQuad_F32     i2s_out(audio_settings);        //Send audio out
AudioSDWriter_F32_UI_logging       audioSDWriter(&sd, audio_settings);  //Write audio to the SD card (if activated)

//Connect the signal sources to the audio mixer
AudioConnection_F32           patchcord1(chirp,    0, audioMixerL, 0);   //chirp is mono, so simply send to left channel mixer
AudioConnection_F32           patchcord2(audioSDPlayer, 0, audioMixerL, 1);   //left channel of SD to left channel mixer
AudioConnection_F32           patchcord3(chirp,    0, audioMixerR, 0);   //chirp is mono, so simply send it to right channel mixer too
AudioConnection_F32           patchcord4(audioSDPlayer, 1, audioMixerR, 1);   //right channel of SD to right channel mixer

//Connect inputs to outputs...let's just connect the first two mics to the black jack on the Tympan
AudioConnection_F32           patchcord11(i2s_in, 1, i2s_out, 0);    //Left input to left output
AudioConnection_F32           patchcord12(i2s_in, 0, i2s_out, 1);    //Right input to right output

//Connect audio mixer to the earpieces and to the black jack on the Earpiece shield
AudioConnection_F32           patchcord21(audioMixerL, 0, i2s_out, 2);  //connect chirp to left output on earpiece shield
AudioConnection_F32           patchcord22(audioMixerR, 0, i2s_out, 3);  //connect chirp to right output on earpiece shield

//Connect all four mics to SD logging
#define NUM_SD_CHAN 4
AudioConnection_F32           patchcord31(i2s_in, 1, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord32(i2s_in, 0, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer
AudioConnection_F32           patchcord33(i2s_in, 3, audioSDWriter, 2);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord34(i2s_in, 2, audioSDWriter, 3);   //connect Raw audio to right channel of SD writer


// /////////// Functions for configuring the system
float setInputGain_dB(float val_dB) {
  myState.input_gain_dB = myTympan.setInputGain_dB(val_dB);
  return earpieceShield.setInputGain_dB(myState.input_gain_dB);
}

void setConfiguration(int config) {
  myState.input_source = config;
  const float default_mic_gain_dB = 0.0f;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); earpieceShield.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      earpieceShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      setInputGain_dB(default_mic_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); earpieceShield.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the on-board microphones
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      earpieceShield.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      setInputGain_dB(default_mic_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); earpieceShield.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      setInputGain_dB(0.0);
      break;

    case State::INPUT_PDM_MICS:
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      myTympan.enableDigitalMicInputs(true);
      earpieceShield.enableDigitalMicInputs(true);
      setInputGain_dB(0.0);
      break;

    // Use MIC_JACK on Tympan main board, and digital mics on Earpiece Shield
    case State::INPUT_MIC_JACK_WTIH_PDM_MIC:
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false);
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the line-input through holes
      earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      earpieceShield.enableDigitalMicInputs(true);
      setInputGain_dB(0.0);
      break;
  }
}