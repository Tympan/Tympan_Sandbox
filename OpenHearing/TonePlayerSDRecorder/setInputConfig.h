

#define USING_EARPIECE_SHEILD false

extern float setInputGain_dB(float);  //this will be defined somewhere else in the pile of Arduino code

void setInputConfiguration(int config) {
  myState.input_source = config;
  const float default_mic_gain_dB = 0.0f;

  switch (config) {
    case State::INPUT_PCBMICS:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); 
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      #if USING_EARPIECE_SHEILD 
        earpieceShield.enableDigitalMicInputs(false);
        earpieceShield.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones
      #endif
      setInputGain_dB(default_mic_gain_dB);
      break;

    case State::INPUT_JACK_MIC:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); 
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      #if USING_EARPIECE_SHIELD
        earpieceShield.enableDigitalMicInputs(false);
        earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the on-board microphones
        earpieceShield.setEnableStereoExtMicBias(true);  //put the mic bias on both channels
      #endif
      setInputGain_dB(default_mic_gain_dB);
      break;
      
    case State::INPUT_JACK_LINE:
      //Select Input and set gain
      myState.input_source = config;
      myTympan.enableDigitalMicInputs(false); 
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      #if USING_EARPIECE_SHIELD
        earpieceShield.enableDigitalMicInputs(false);
        earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      #endif
      setInputGain_dB(0.0);
      break;

    case State::INPUT_PDM_MICS:
      myState.input_source = config;
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
      myTympan.enableDigitalMicInputs(true);
      #if USING_EARPIECE_SHIELD
        earpieceShield.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes
        earpieceShield.enableDigitalMicInputs(true);
      #endif
      setInputGain_dB(0.0);
      break;
  }
}
