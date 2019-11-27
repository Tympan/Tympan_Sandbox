
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>
#include "State.h"

//add in the algorithm whose gains we wish to set via this SerialManager...change this if your gain algorithms class changes names!
#include "AudioEffectCompWDRC_F32.h"    //change this if you change the name of the algorithm's source code filename
typedef AudioEffectCompWDRC_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

#define MAX_DATASTREAM_LENGTH 1024
#define DATASTREAM_START_CHAR (char)0x02
#define DATASTREAM_SEPARATOR (char)0x03
#define DATASTREAM_END_CHAR (char)0x04
enum read_state_options {
  SINGLE_CHAR,
  STREAM_LENGTH,
  STREAM_DATA
};

//objects in the main sketch that I want to call from here
//extern Tympan myTympan;
//extern AudioSDWriter_F32 audioSDWriter;
extern State myState;

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(TympanBase &_audioHardware,
          int n,
          GainAlgorithm_t *gain_algsL, 
          GainAlgorithm_t *gain_algsR,
          AudioControlTestAmpSweep_F32 &_ampSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester_filterbank,
          AudioEffectFeedbackCancel_F32 &_feedbackCancel,
          AudioEffectFeedbackCancel_F32 &_feedbackCancelR
          )
      : audioHardware(_audioHardware),
        gain_algorithmsL(gain_algsL), 
        gain_algorithmsR(gain_algsR),
        ampSweepTester(_ampSweepTester), 
        freqSweepTester(_freqSweepTester),
        freqSweepTester_filterbank(_freqSweepTester_filterbank),
        feedbackCanceler(_feedbackCancel),
        feedbackCancelerR(_feedbackCancelR)
        {
          N_CHAN = n;
          serial_read_state = SINGLE_CHAR;
        };
      
    void respondToByte(char c);
    void processSingleCharacter(char c);
    void processStream(void);
    void printHelp(void);
    void incrementChannelGain(int chan, float change_dB);
    void decreaseChannelGain(int chan);
    void set_N_CHAN(int _n_chan) { N_CHAN = _n_chan; };
    void printChanUpMsg(int N_CHAN);
    void printChanDownMsg(int N_CHAN);
    void interpretStreamGHA(int idx);
    void interpretStreamDSL(int idx);
    void interpretStreamAFC(int idx);
    int readStreamIntArray(int idx, int* arr, int len);
    int readStreamFloatArray(int idx, float* arr, int len);
    void setFullGUIState(void);
    void setButtonState_algPresets(void);
    void setButtonState_inputMixer(void);
    void setButtonState(String btnId, bool newState);
    void setButtonText(String btnId, String text);
    String channelGainAsString(int chan);

    float channelGainIncrement_dB = 2.5f;  
    int N_CHAN;
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;
    float FAKE_GAIN_LEVEL[8] = {0.,0.,0.,0.,0.,0.,0.,0.};
  private:
    TympanBase &audioHardware;
    GainAlgorithm_t *gain_algorithmsL;  //point to first element in array of expanders
    GainAlgorithm_t *gain_algorithmsR;  //point to first element in array of expanders
    AudioControlTestAmpSweep_F32 &ampSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester_filterbank;
    AudioEffectFeedbackCancel_F32 &feedbackCanceler;
    AudioEffectFeedbackCancel_F32 &feedbackCancelerR;
};

#define MAX_CHANS 8
void SerialManager::printChanUpMsg(int N_CHAN) {
  char fooChar[] = "12345678";
  audioHardware.print("   ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    audioHardware.print(fooChar[i]); 
    if (i < (N_CHAN-1)) audioHardware.print(",");
  }
  audioHardware.print(": Increase linear gain of given channel (1-");
  audioHardware.print(N_CHAN);
  audioHardware.print(") by ");
}
void SerialManager::printChanDownMsg(int N_CHAN) {
  char fooChar[] = "!@#$%^&*";
  audioHardware.print("   ");
  for (int i=0;i<min(MAX_CHANS,N_CHAN);i++) {
    audioHardware.print(fooChar[i]); 
    if (i < (N_CHAN-1)) audioHardware.print(",");
  }
  audioHardware.print(": Decrease linear gain of given channel (1-");
  audioHardware.print(N_CHAN);
  audioHardware.print(") by ");
}
void SerialManager::printHelp(void) {  
  audioHardware.println();
  audioHardware.println("SerialManager Help: Available Commands:");
  audioHardware.println("   h: Print this help");
  audioHardware.println("   g: Print the gain settings of the device.");
  audioHardware.println("   c/C: Enable/disable printing of CPU and Memory");
  audioHardware.println("   l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  audioHardware.println("   L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  audioHardware.println("   A: Self-Generated Test: Amplitude sweep.  End-to-End Measurement.");
  audioHardware.println("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  audioHardware.println("   f: Self-Generated Test: Frequency sweep.  Measure filterbank.");
  audioHardware.print("   k: Increase the gain of all channels (ie, knob gain) by "); audioHardware.print(channelGainIncrement_dB); audioHardware.println(" dB");
  audioHardware.print("   K: Decrease the gain of all channels (ie, knob gain) by ");audioHardware.println(" dB");
  audioHardware.println("   q,Q: Mute or Unmute the audio.");
  audioHardware.println("   s,S: Mono or Stereo Audio.");
  printChanUpMsg(N_CHAN);  audioHardware.print(channelGainIncrement_dB); audioHardware.println(" dB");
  printChanDownMsg(N_CHAN);  audioHardware.print(channelGainIncrement_dB); audioHardware.println(" dB");
  audioHardware.print("   d,D: Switch to WDRC Preset A or Preset B");audioHardware.println();
  //audioHardware.print("   p,P: Enable or Disable Adaptive Feedback Cancelation.");audioHardware.println();
  //audioHardware.print("   m,M: Increase or Decrease AFC mu (currently "); audioHardware.print(feedbackCanceler.getMu(),6) ; audioHardware.println(").");
  //audioHardware.print("   r,R: Increase or Decrease AFC rho (currently "); audioHardware.print(feedbackCanceler.getRho(),6) ; audioHardware.println(").");
  //audioHardware.print("   e,E: Increase or Decrease AFC eps (currently "); audioHardware.print(feedbackCanceler.getEps(),6) ; audioHardware.println(").");
  //audioHardware.print("   x,X: Increase or Decrease AFC filter length (currently "); audioHardware.print(feedbackCanceler.getAfl()) ; audioHardware.println(").");
  //audioHardware.print("   u,U: Increase or Decrease Cutoff Frequency of HP Prefilter (currently "); audioHardware.print(audioHardware.getHPCutoff_Hz()); audioHardware.println(" Hz).");
  //audioHardware.print("   z,Z: Increase or Decrease AFC N_Coeff_To_Zero (currently "); audioHardware.print(feedbackCanceler.getNCoeffToZero()) ; audioHardware.println(").");  
  audioHardware.println("   J: Print the JSON config object, for the Tympan Remote app");
  audioHardware.println();
}

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void togglePrintAveSignalLevels(bool);
//extern void incrementDSLConfiguration(Stream *);
extern void setDSLConfiguration(int);
extern void updateDSL(BTNRH_WDRC::CHA_DSL &);
extern void updateGHA(BTNRH_WDRC::CHA_WDRC &);
extern void updateAFC(BTNRH_WDRC::CHA_AFC &);
extern void configureLeftRightMixer(int);

//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
  switch (serial_read_state) {
    case SINGLE_CHAR:
      if (c == DATASTREAM_START_CHAR) {
        Serial.println("Start data stream.");
        // Start a data stream:
        serial_read_state = STREAM_LENGTH;
        stream_chars_received = 0;
      } else {
        Serial.print("Processing character ");
        Serial.println(c);
        processSingleCharacter(c);
      }
      break;
    case STREAM_LENGTH:
      //Serial.print("Reading stream length char: ");
      //Serial.print(c);
      //Serial.print(" = ");
      //Serial.println(c, HEX);
      if (c == DATASTREAM_SEPARATOR) {
        // Get the datastream length:
        stream_length = *((int*)(stream_data));
        serial_read_state = STREAM_DATA;
        stream_chars_received = 0;
        Serial.print("Stream length = ");
        Serial.println(stream_length);
      } else {
        //Serial.println("End of stream length message");
        stream_data[stream_chars_received] = c;
        stream_chars_received++;
      }
      break;
    case STREAM_DATA:
      if (stream_chars_received < stream_length) {
        stream_data[stream_chars_received] = c;
        stream_chars_received++;        
      } else {
        if (c == DATASTREAM_END_CHAR) {
          // successfully read the stream
          Serial.println("Time to process stream!");
          processStream();
        } else {
          audioHardware.print("ERROR: Expected string terminator ");
          audioHardware.print(DATASTREAM_END_CHAR, HEX);
          audioHardware.print("; found ");
          audioHardware.print(c,HEX);
          audioHardware.println(" instead.");
        }
        serial_read_state = SINGLE_CHAR;
        stream_chars_received = 0;
      }
      break;
  }
}

void SerialManager::processSingleCharacter(char c) {
  float old_val = 0.0, new_val = 0.0;
  switch (c) {
    case 'h': case '?':
      printHelp(); break;
    case 'g': case 'G':
      printGainSettings(); break;
    case 'k':
      incrementKnobGain(channelGainIncrement_dB); break;
    case 'K':   //which is "shift k"
      incrementKnobGain(-channelGainIncrement_dB);  break;
    case '1':
      incrementChannelGain(1-1, channelGainIncrement_dB);
      setButtonText("lowGain",channelGainAsString(1-1));
      break;
    case '2':
      incrementChannelGain(2-1, channelGainIncrement_dB);
      setButtonText("midGain",channelGainAsString(2-1));
      break;
    case '3':
      incrementChannelGain(3-1, channelGainIncrement_dB);
      setButtonText("highGain",channelGainAsString(3-1));
      break;
    case '4':
      incrementChannelGain(4-1, channelGainIncrement_dB); break;
    case '5':
      incrementChannelGain(5-1, channelGainIncrement_dB); break;
    case '6':
      incrementChannelGain(6-1, channelGainIncrement_dB); break;
    case '7':
      incrementChannelGain(7-1, channelGainIncrement_dB); break;
    case '8':      
      incrementChannelGain(8-1, channelGainIncrement_dB); break;    
    case '!':  //which is "shift 1"
      incrementChannelGain(1-1, -channelGainIncrement_dB);
      setButtonText("lowGain",channelGainAsString(1-1));
      break;  
    case '@':  //which is "shift 2"
      incrementChannelGain(2-1, -channelGainIncrement_dB);
      setButtonText("midGain",channelGainAsString(2-1));
      break;  
    case '#':  //which is "shift 3"
      incrementChannelGain(3-1, -channelGainIncrement_dB);
      setButtonText("highGain",channelGainAsString(3-1));
      break;  
    case '$':  //which is "shift 4"
      incrementChannelGain(4-1, -channelGainIncrement_dB); break;
    case '%':  //which is "shift 5"
      incrementChannelGain(5-1, -channelGainIncrement_dB); break;
    case '^':  //which is "shift 6"
      incrementChannelGain(6-1, -channelGainIncrement_dB); break;
    case '&':  //which is "shift 7"
      incrementChannelGain(7-1, -channelGainIncrement_dB); break;
    case '*':  //which is "shift 8"
      incrementChannelGain(8-1, -channelGainIncrement_dB); break;          
    case 'A':
      //amplitude sweep test
      { //limit the scope of any variables that I create here
        ampSweepTester.setSignalFrequency_Hz(1000.f);
        float start_amp_dB = -100.0f, end_amp_dB = 0.0f, step_amp_dB = 5.0f;
        ampSweepTester.setStepPattern(start_amp_dB, end_amp_dB, step_amp_dB);
        ampSweepTester.setTargetDurPerStep_sec(1.0);
      }
      audioHardware.println("Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      audioHardware.println("Press 'h' for help...");
      break;
    case 'c':
      Serial.println("Received: printing memory and CPU.");
      myState.flag_printCPUandMemory = true;
      //setButtonState("cpuStart",true);
      break;
    case 'C':
      Serial.println("Received: stopping printing of memory and CPU.");
      myState.flag_printCPUandMemory = false;
      //setButtonState("cpuStart",false);
      break;
    case 'd':
      audioHardware.println("Received: changing to Preset A");
      setDSLConfiguration(State::DSL_PRESET_A);
      setButtonState_algPresets();
      break;      
    case 'D':
      audioHardware.println("Received: changing to Preset B");
      setDSLConfiguration(State::DSL_PRESET_B);
      setButtonState_algPresets();      break;
    case 'F':
      //frequency sweep test...end-to-end
      { //limit the scope of any variables that I create here
        freqSweepTester.setSignalAmplitude_dBFS(-70.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester.setTargetDurPerStep_sec(1.0);
      }
      audioHardware.println("Received: starting test using frequency sweep, end-to-end assessment...");
      freqSweepTester.begin();
      while (!freqSweepTester.available()) {delay(100);};
      audioHardware.println("Press 'h' for help...");
      break; 
    case 'f':
      //frequency sweep test
      { //limit the scope of any variables that I create here
        freqSweepTester_filterbank.setSignalAmplitude_dBFS(-30.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester_filterbank.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester_filterbank.setTargetDurPerStep_sec(0.5);
      }
      audioHardware.println("Received: starting test using frequency sweep.  Filterbank assessment...");
      freqSweepTester_filterbank.begin();
      while (!freqSweepTester_filterbank.available()) {delay(100);};
      audioHardware.println("Press 'h' for help...");
      break; 
    case 'q':
      configureLeftRightMixer(State::INPUTMIX_MUTE);
      audioHardware.println("Received: Muting audio.");
      setButtonState_inputMixer();
      break;
    case 'Q':
      configureLeftRightMixer(State::INPUTMIX_STEREO);
      audioHardware.println("Received: Stereo audio.");
      setButtonState_inputMixer();
      break;  
    case 's':
      configureLeftRightMixer(State::INPUTMIX_MONO);
      audioHardware.println("Received: Mono audio.");
      setButtonState_inputMixer();
      break;
    case 'S':
      configureLeftRightMixer(State::INPUTMIX_STEREO);
      audioHardware.println("Received: Stereo audio.");
      setButtonState_inputMixer();
      break;   
    case 'J':
    {
      // Print the layout for the Tympan Remote app, in a JSON-ish string 
      // (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).  
      char jsonConfig[] = "JSON={"
        "'pages':["
          "{'title':'Presets','cards':["
              "{'name':'Algorithm Presets','buttons':[{'label': 'Compression (WDRC)', 'cmd': 'd', 'id': 'alg_preset0'},{'label': 'Linear', 'cmd': 'D', 'id': 'alg_preset1'}]},"
              "{'name':'Overall Audio','buttons':[{'label': 'Stereo','cmd': 'Q','id':'inp_stereo','width':'6'},{'label': 'Mono','cmd': 's','id':'inp_mono','width':'6'},{'label': 'Mute','cmd': 'q','id':'inp_mute','width':'12'}]}"
          "]},"
          "{'title':'Tuner','cards':["
              "{'name':'Overall Volume', 'buttons':[{'label': '-', 'cmd' :'K'},{'label': '+', 'cmd': 'k'}]},"
              "{'name':'High Gain', 'buttons':[{'label': '-', 'cmd': '#','width':'4'},{'id':'highGain', 'label': '', 'width':'4'},{'label': '+', 'cmd': '3','width':'4'}]},"
              "{'name':'Mid Gain', 'buttons':[{'label': '-', 'cmd': '@','width':'4'},{'id':'midGain', 'label':'', 'width':'4'},{'label': '+', 'cmd': '2','width':'4'}]},"
              "{'name':'Low Gain', 'buttons':[{'label': '-', 'cmd': '!','width':'4'},{'id':'lowGain', 'label':'', 'width':'4'},{'label': '+', 'cmd': '1','width':'4'}]}"                          
          "]}"
        "],"
        "'prescription':{'type':'BoysTown','pages':['multiband','broadband','afc','plot']}"
      "}";

      audioHardware.println(jsonConfig);
      delay(100);
      setFullGUIState();
      setButtonText("highGain",0);
      setButtonText("midGain",0);
      setButtonText("lowGain",0);
      break;
    }
    case 'l':
      audioHardware.println("Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = false; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      audioHardware.println("Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break;
//    case 'p':
//      audioHardware.println("Received: enabling feedback cancelation.");
//      feedbackCanceler.setEnable(true);feedbackCancelerR.setEnable(true);
//      //feedbackCanceler.resetAFCfilter();
//      break;
//    case 'P':
//      audioHardware.println("Received: disabling feedback cancelation.");      
//      feedbackCanceler.setEnable(false);feedbackCancelerR.setEnable(false);
//      break;
//    case 'm':
//      old_val = feedbackCanceler.getMu(); new_val = old_val * 2.0;
//      audioHardware.print("Received: increasing AFC mu to ");
//      audioHardware.println(feedbackCanceler.setMu(new_val),6);
//      feedbackCancelerR.setMu(new_val);
//      break;
//    case 'M':
//      old_val = feedbackCanceler.getMu(); new_val = old_val / 2.0;
//      audioHardware.print("Received: decreasing AFC mu to "); 
//      audioHardware.println(feedbackCanceler.setMu(new_val),6);
//      feedbackCancelerR.setMu(new_val);
//      break;
//    case 'r':
//      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)/sqrt(2.0));
//      audioHardware.print("Received: increasing AFC rho to "); 
//      audioHardware.println(feedbackCanceler.setRho(new_val),6);
//      feedbackCancelerR.setRho(new_val);
//      break;
//    case 'R':
//      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)*sqrt(2.0));
//      audioHardware.print("Received: increasing AFC rho to "); 
//      audioHardware.println(feedbackCanceler.setRho(new_val),6);
//      feedbackCancelerR.setRho(new_val);
//      break;
//    case 'e':
//      old_val = feedbackCanceler.getEps(); new_val = old_val*sqrt(10.0);
//      audioHardware.print("Received: increasing AFC eps to "); 
//      audioHardware.println(feedbackCanceler.setEps(new_val),6);
//      feedbackCancelerR.setEps(new_val);
//      break;
//    case 'E':
//      old_val = feedbackCanceler.getEps(); new_val = old_val/sqrt(10.0);
//      audioHardware.print("Received: increasing AFC eps to ");
//      audioHardware.println(feedbackCanceler.setEps(new_val),6);
//      feedbackCancelerR.setEps(new_val);
//      break;    
//    case 'x':
//      old_val = feedbackCanceler.getAfl(); new_val = old_val + 5;
//      audioHardware.print("Received: increasing AFC filter length to ");
//      audioHardware.println(feedbackCanceler.setAfl(new_val));
//      feedbackCancelerR.setAfl(new_val);
//      break;    
//    case 'X':
//      old_val = feedbackCanceler.getAfl(); new_val = old_val - 5;
//      audioHardware.print("Received: decreasing AFC filter length to ");
//      audioHardware.println(feedbackCanceler.setAfl(new_val));
//      feedbackCanceler.setAfl(new_val);
//      break;            
//    case 'z':
//      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val + 5;
//      audioHardware.print("Received: increasing AFC N_Coeff_To_Zero to "); audioHardware.println(feedbackCanceler.setNCoeffToZero(new_val));
//      break;
//    case 'Z':
//      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val - 5;
//      audioHardware.print("Received: decreasing AFC N_Coeff_To_Zero to "); audioHardware.println(feedbackCanceler.setNCoeffToZero(new_val));      
//      break;
    case 'u':
    {
      old_val = audioHardware.getHPCutoff_Hz(); new_val = min(old_val*sqrt(2.0), 8000.0); //half-octave steps up
      float fs_Hz = audioHardware.getSampleRate_Hz();
      audioHardware.setHPFonADC(true,new_val,fs_Hz);
      audioHardware.print("Received: Increasing ADC HP Cutoff to "); audioHardware.print(audioHardware.getHPCutoff_Hz());audioHardware.println(" Hz");
    }
      break;
    case 'U':
    {
      old_val = audioHardware.getHPCutoff_Hz(); new_val = max(old_val/sqrt(2.0), 5.0); //half-octave steps down
      float fs_Hz = audioHardware.getSampleRate_Hz();
      audioHardware.setHPFonADC(true,new_val,fs_Hz);
      audioHardware.print("Received: Decreasing ADC HP Cutoff to "); audioHardware.print(audioHardware.getHPCutoff_Hz());audioHardware.println(" Hz");   
      break;
    }
  }
}

void SerialManager::processStream(void) {
  int idx = 0;
  String streamType;
  int tmpInt;
  float tmpFloat;
  
  while (stream_data[idx] != DATASTREAM_SEPARATOR) {
    streamType.append(stream_data[idx]);
    idx++;
  }
  idx++; // move past the datastream separator

  //audioHardware.print("Received stream: ");
  //audioHardware.print(stream_data);

  if (streamType == "gha") {    
    audioHardware.println("Stream is of type 'gha'.");
    interpretStreamGHA(idx);
  } else if (streamType == "dsl") {
    audioHardware.println("Stream is of type 'dsl'.");
    interpretStreamDSL(idx);
  } else if (streamType == "afc") {
    audioHardware.println("Stream is of type 'afc'.");
    interpretStreamAFC(idx);
  } else if (streamType == "test") {    
    audioHardware.println("Stream is of type 'test'.");
    tmpInt = *((int*)(stream_data+idx)); idx = idx+4;
    audioHardware.print("int is "); audioHardware.println(tmpInt);
    tmpFloat = *((float*)(stream_data+idx)); idx = idx+4;
    audioHardware.print("float is "); audioHardware.println(tmpFloat);
  } else {
    audioHardware.print("Unknown stream type: ");
    audioHardware.println(streamType);
  }
}

void SerialManager::interpretStreamGHA(int idx) {
  float attack, release, sampRate, maxdB, compRatioLow, kneepoint, compStartGain, compStartKnee, compRatioHigh, threshold;

  attack        = *((float*)(stream_data+idx)); idx=idx+4;
  release       = *((float*)(stream_data+idx)); idx=idx+4;
  sampRate      = *((float*)(stream_data+idx)); idx=idx+4;
  maxdB         = *((float*)(stream_data+idx)); idx=idx+4;
  compRatioLow  = *((float*)(stream_data+idx)); idx=idx+4;
  kneepoint     = *((float*)(stream_data+idx)); idx=idx+4;
  compStartGain = *((float*)(stream_data+idx)); idx=idx+4;
  compStartKnee = *((float*)(stream_data+idx)); idx=idx+4;
  compRatioHigh = *((float*)(stream_data+idx)); idx=idx+4;
  threshold     = *((float*)(stream_data+idx)); idx=idx+4;

  /* Printing too much causes the teensy to freeze. 
  audioHardware.println("New WDRC:");
  audioHardware.print("  attack = "); audioHardware.println(attack); 
  audioHardware.print("  release = "); audioHardware.println(release);
  audioHardware.print("  sampRate = "); audioHardware.println(sampRate); 
  audioHardware.print("  maxdB = "); audioHardware.println(maxdB); 
  audioHardware.print("  compRatioLow = "); audioHardware.println(compRatioLow); 
  audioHardware.print("  kneepoint = "); audioHardware.println(kneepoint); 
  audioHardware.print("  compStartGain = "); audioHardware.println(compStartGain); 
  audioHardware.print("  compStartKnee = "); audioHardware.println(compStartKnee); 
  audioHardware.print("  compRatioHigh = "); audioHardware.println(compRatioHigh); 
  audioHardware.print("  threshold = "); audioHardware.println(threshold); 
  */
  
  audioHardware.println("SUCCESS.");    

  BTNRH_WDRC::CHA_WDRC gha = {attack, release, sampRate, maxdB, 
    compRatioLow, kneepoint, compStartGain, compStartKnee, compRatioHigh, threshold};

  updateGHA(gha);
}


void SerialManager::interpretStreamDSL(int idx) {
  float attack, release, maxdB;
  int numChannels, i;

  int freq[8];
  float lowSPLRatio[8];
  float expansionKneepoint[8];
  float compStartGain[8];
  float compRatio[8];
  float compStartKnee[8];
  float threshold[8];

  attack        = *((float*)(stream_data+idx)); idx=idx+4;
  release       = *((float*)(stream_data+idx)); idx=idx+4;
  numChannels   = *((int*)(stream_data+idx)); idx=idx+4;
  maxdB         = *((float*)(stream_data+idx)); idx=idx+4;

  idx = readStreamIntArray(idx, freq, 8);
  idx = readStreamFloatArray(idx, lowSPLRatio, 8);
  idx = readStreamFloatArray(idx, expansionKneepoint, 8);
  idx = readStreamFloatArray(idx, compStartGain, 8);
  idx = readStreamFloatArray(idx, compRatio, 8);
  idx = readStreamFloatArray(idx, compStartKnee, 8);
  idx = readStreamFloatArray(idx, threshold, 8);

//  audioHardware.print("attack");
//  audioHardware.print(attack);
//  audioHardware.print("release");
//  audioHardware.print(release);
//  audioHardware.print("#channels");
//  audioHardware.print(numChannels);
//  audioHardware.print("maxdB");
//  audioHardware.print(maxdB);

  audioHardware.print("  freq = {"); 
  audioHardware.print(freq[0]); 
  for (i=1; i<8; i++) {
    audioHardware.print(", "); audioHardware.print(freq[i]);
  }
  audioHardware.println("}");

  audioHardware.print("  lowSPLRatio = {"); 
  audioHardware.print(lowSPLRatio[0]); 
  for (i=1; i<8; i++) {
    audioHardware.print(", "); audioHardware.print(lowSPLRatio[i]);
  }
  audioHardware.println("}");

  BTNRH_WDRC::CHA_DSL dsl = {
    attack,  // attack (ms)
    release,  // release (ms)
    maxdB,  //maxdB.  calibration.  dB SPL for input signal at 0 dBFS.  Needs to be tailored to mic, spkrs, and mic gain.
    0,
    numChannels,    //num channels used (must be less than MAX_CHAN constant set in the main program
    *freq,   // cross frequencies (Hz)...FOR IIR FILTERING, THESE VALUES ARE IGNORED!!!
    *lowSPLRatio,   // compression ratio for low-SPL region (ie, the expander..values should be < 1.0)
    *expansionKneepoint,   // expansion-end kneepoint
    *compStartGain,   // compression-start gain
    *compRatio,   // compression ratio
    *compStartKnee,   // compression-start kneepoint (input dB SPL)
    *threshold    // output limiting threshold (comp ratio 10)
  };
  updateDSL(dsl);
  
  audioHardware.println("SUCCESS.");      
}

void SerialManager::interpretStreamAFC(int idx) {
  int default_to_active; //enable AFC at startup?  1=active. 0=disabled.
  int afl;  //length (samples) of adaptive filter for modeling feedback path.
  float mu; //mu, scale factor for how fast the adaptive filter adapts (bigger is faster)
  float rho;  //rho, smoothing factor for estimating audio envelope (bigger is a longer average)
  float eps;  //eps, when est the audio envelope, this is the min allowed level (avoids divide-by-zero)

  default_to_active = *((int*)(stream_data+idx)); idx=idx+4;
  afl               = *((int*)(stream_data+idx)); idx=idx+4;
  mu                = *((float*)(stream_data+idx)); idx=idx+4;
  rho               = *((float*)(stream_data+idx)); idx=idx+4;
  eps               = *((float*)(stream_data+idx)); idx=idx+4;

  BTNRH_WDRC::CHA_AFC afc = {default_to_active, afl, mu, rho, eps};
  updateAFC(afc);
  
  audioHardware.println("SUCCESS.");
}



int SerialManager::readStreamIntArray(int idx, int* arr, int len) {
  int i;
  for (i=0; i<len; i++) {
    arr[i] = *((int*)(stream_data+idx)); 
    idx=idx+4;
  }

  return idx;
}

int SerialManager::readStreamFloatArray(int idx, float* arr, int len) {
  int i;
  for (i=0; i<len; i++) {
    arr[i] = *((float*)(stream_data+idx)); 
    idx=idx+4;
  }

  return idx;
}



void SerialManager::incrementChannelGain(int chan, float change_dB) {
  //increments the linear gain
  if (chan < N_CHAN) {
    //gain_algorithmsL[chan].incrementGain_dB(change_dB);
    //gain_algorithmsR[chan].incrementGain_dB(change_dB);
    myState.wdrc_perBand.tkgain[chan] += change_dB;
    updateDSL(myState.wdrc_perBand);
    //add something here (?) to send updated prescription values to the remote device
    
    printGainSettings();  //in main sketch file
    FAKE_GAIN_LEVEL[chan] += change_dB;
  }
}

String SerialManager::channelGainAsString(int chan) {
  return String(FAKE_GAIN_LEVEL[chan],1);
}

void SerialManager::setFullGUIState(void) {
  setButtonState_algPresets();
  setButtonState_inputMixer();
  //add something here to send prescription values to the remote device
}

void SerialManager::setButtonState_algPresets(void) {
  setButtonState("alg_preset0",false);  delay(10);
  setButtonState("alg_preset1",false); delay(10);
  switch (myState.current_dsl_config) {
    case (State::DSL_PRESET_A):
      setButtonState("alg_preset0",true);  delay(10); break;
    case (State::DSL_PRESET_B):
      setButtonState("alg_preset1",true); delay(10); break;
  }
}

void SerialManager::setButtonState_inputMixer(void) {
  setButtonState("inp_stereo",false);  delay(10);
  setButtonState("inp_mono",false); delay(10);
  setButtonState("inp_mute",false);delay(10);
  switch (myState.input_mixer_config) {
    case (State::INPUTMIX_STEREO):
      setButtonState("inp_stereo",true);  delay(10); break;
    case (State::INPUTMIX_MONO):
      setButtonState("inp_mono",true); delay(10); break;
    case (State::INPUTMIX_MUTE):
      setButtonState("inp_mute",true);  delay(10); break;
  }
}

void SerialManager::setButtonState(String btnId, bool newState) {
  if (newState) {
    audioHardware.println("STATE=BTN:" + btnId + ":1");
  } else {
    audioHardware.println("STATE=BTN:" + btnId + ":0");
  }
}

void SerialManager::setButtonText(String btnId, String text) {
  audioHardware.println("TEXT=BTN:" + btnId + ":"+text);
}

#endif
