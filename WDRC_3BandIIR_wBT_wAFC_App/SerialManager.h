
#ifndef _SerialManager_h
#define _SerialManager_h

#include <Tympan_Library.h>

//add in the algorithm whose gains we wish to set via this SerialManager...change this if your gain algorithms class changes names!
#include "AudioEffectCompWDRC_F32.h"    //change this if you change the name of the algorithm's source code filename
typedef AudioEffectCompWDRC_F32 GainAlgorithm_t; //change this if you change the algorithm's class name

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(TympanBase &_audioHardware,
          int n,
          GainAlgorithm_t *gain_algs, 
          AudioControlTestAmpSweep_F32 &_ampSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester,
          AudioControlTestFreqSweep_F32 &_freqSweepTester_filterbank,
          AudioEffectFeedbackCancel_F32 &_feedbackCancel)
      : audioHardware(_audioHardware),
        gain_algorithms(gain_algs), 
        ampSweepTester(_ampSweepTester), 
        freqSweepTester(_freqSweepTester),
        freqSweepTester_filterbank(_freqSweepTester_filterbank),
        feedbackCanceler(_feedbackCancel)
        {
          N_CHAN = n;
        };
      
    void respondToByte(char c);
    void printHelp(void);
    void incrementChannelGain(int chan, float change_dB);
    void decreaseChannelGain(int chan);
    void set_N_CHAN(int _n_chan) { N_CHAN = _n_chan; };
    void printChanUpMsg(int N_CHAN);
    void printChanDownMsg(int N_CHAN);

    float channelGainIncrement_dB = 2.5f;  
    int N_CHAN;
  private:
    TympanBase &audioHardware;
    GainAlgorithm_t *gain_algorithms;  //point to first element in array of expanders
    AudioControlTestAmpSweep_F32 &ampSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester;
    AudioControlTestFreqSweep_F32 &freqSweepTester_filterbank;
    AudioEffectFeedbackCancel_F32 &feedbackCanceler;
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
  audioHardware.println("   C: Toggle printing of CPU and Memory usage");
  audioHardware.println("   l: Toggle printing of pre-gain per-channel signal levels (dBFS)");
  audioHardware.println("   L: Toggle printing of pre-gain per-channel signal levels (dBSPL, per DSL 'maxdB')");
  audioHardware.println("   A: Self-Generated Test: Amplitude sweep.  End-to-End Measurement.");
  audioHardware.println("   F: Self-Generated Test: Frequency sweep.  End-to-End Measurement.");
  audioHardware.println("   f: Self-Generated Test: Frequency sweep.  Measure filterbank.");
  audioHardware.print("   k: Increase the gain of all channels (ie, knob gain) by "); audioHardware.print(channelGainIncrement_dB); audioHardware.println(" dB");
  audioHardware.print("   K: Decrease the gain of all channels (ie, knob gain) by ");audioHardware.println(" dB");
  printChanUpMsg(N_CHAN);  audioHardware.print(channelGainIncrement_dB); audioHardware.println(" dB");
  printChanDownMsg(N_CHAN);  audioHardware.print(channelGainIncrement_dB); audioHardware.println(" dB");
  audioHardware.print("   d,D: Switch to DSL Preset A or Preset B");audioHardware.println();
  audioHardware.print("   p,P: Enable or Disable Adaptive Feedback Cancelation.");audioHardware.println();
  audioHardware.print("   m,M: Increase or Decrease AFC mu (currently "); audioHardware.print(feedbackCanceler.getMu(),6) ; audioHardware.println(").");
  audioHardware.print("   r,R: Increase or Decrease AFC rho (currently "); audioHardware.print(feedbackCanceler.getRho(),6) ; audioHardware.println(").");
  audioHardware.print("   e,E: Increase or Decrease AFC eps (currently "); audioHardware.print(feedbackCanceler.getEps(),6) ; audioHardware.println(").");
  audioHardware.print("   x,X: Increase or Decrease AFC filter length (currently "); audioHardware.print(feedbackCanceler.getAfl()) ; audioHardware.println(").");
  audioHardware.print("   u,U: Increase or Decrease Cutoff Frequency of HP Prefilter (currently "); audioHardware.print(audioHardware.getHPCutoff_Hz()); audioHardware.println(" Hz).");
  //audioHardware.print("   z,Z: Increase or Decrease AFC N_Coeff_To_Zero (currently "); audioHardware.print(feedbackCanceler.getNCoeffToZero()) ; audioHardware.println(").");  
  audioHardware.println("   J: Print the JSON config object, for the Tympan Remote app");
  audioHardware.println();
}

//functions in the main sketch that I want to call from here
extern void incrementKnobGain(float);
extern void printGainSettings(void);
extern void printGainSettings(void);
extern void togglePrintMemoryAndCPU(void);
extern void togglePrintAveSignalLevels(bool);
//extern void incrementDSLConfiguration(Stream *);
extern void setDSLConfiguration(int);

//switch yard to determine the desired action
void SerialManager::respondToByte(char c) {
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
      incrementChannelGain(1-1, channelGainIncrement_dB); break;
    case '2':
      incrementChannelGain(2-1, channelGainIncrement_dB); break;
    case '3':
      incrementChannelGain(3-1, channelGainIncrement_dB); break;
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
      incrementChannelGain(1-1, -channelGainIncrement_dB); break;
    case '@':  //which is "shift 2"
      incrementChannelGain(2-1, -channelGainIncrement_dB); break;
    case '#':  //which is "shift 3"
      incrementChannelGain(3-1, -channelGainIncrement_dB); break;
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
      audioHardware.println("Command Received: starting test using amplitude sweep...");
      ampSweepTester.begin();
      while (!ampSweepTester.available()) {delay(100);};
      audioHardware.println("Press 'h' for help...");
      break;
    case 'C': case 'c':
      audioHardware.println("Command Received: toggle printing of memory and CPU usage.");
      togglePrintMemoryAndCPU(); break;
    case 'd':
      audioHardware.println("Command Received: changing to DSL Preset A");
      setDSLConfiguration(0);
      break;      
    case 'D':
      audioHardware.println("Command Received: changing to DSL Preset B");
      setDSLConfiguration(1);
      break;
    case 'F':
      //frequency sweep test...end-to-end
      { //limit the scope of any variables that I create here
        freqSweepTester.setSignalAmplitude_dBFS(-70.f);
        float start_freq_Hz = 125.0f, end_freq_Hz = 12000.f, step_octave = powf(2.0,1.0/6.0); //pow(2.0,1.0/3.0) is 3 steps per octave
        freqSweepTester.setStepPattern(start_freq_Hz, end_freq_Hz, step_octave);
        freqSweepTester.setTargetDurPerStep_sec(1.0);
      }
      audioHardware.println("Command Received: starting test using frequency sweep, end-to-end assessment...");
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
      audioHardware.println("Command Received: starting test using frequency sweep.  Filterbank assessment...");
      freqSweepTester_filterbank.begin();
      while (!freqSweepTester_filterbank.available()) {delay(100);};
      audioHardware.println("Press 'h' for help...");
      break;      
    case 'J':
    {
      // Print the layout for the Tympan Remote app, in a JSON-ish string 
      // (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).  
      char jsonConfig[] = "JSON={'pages':["
                      "{'title':'Presets','cards':["
                          "{'name':'Algorithm','buttons':[{'label': 'A', 'cmd': 'd'},{'label': 'B', 'cmd': 'D'}]},"
                      "]},"
                      "{'title':'Tuner','cards':["
                          "{'name':'High Gain', 'buttons':[{'label': '-', 'cmd': '#'},{'label': '+', 'cmd': '3'}]},"
                          "{'name':'Mid Gain', 'buttons':[{'label': '-', 'cmd': '@'},{'label': '+', 'cmd': '2'}]},"
                          "{'name':'Low Gain', 'buttons':[{'label': '-', 'cmd': '!'},{'label': '+', 'cmd': '1'}]}"
                      "]}"
                    "]}";
      audioHardware.println(pages);
      break;
    }
    case 'l':
      audioHardware.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = false; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'L':
      audioHardware.println("Command Received: toggle printing of per-band ave signal levels.");
      { bool as_dBSPL = true; togglePrintAveSignalLevels(as_dBSPL); }
      break;
    case 'p':
      audioHardware.println("Command Received: enabling adaptive feedback cancelation.");
      feedbackCanceler.setEnable(true);
      //feedbackCanceler.resetAFCfilter();
      break;
    case 'P':
      audioHardware.println("Command Received: disabling adaptive feedback cancelation.");      
      feedbackCanceler.setEnable(false);
      break;
    case 'm':
      old_val = feedbackCanceler.getMu(); new_val = old_val * 2.0;
      audioHardware.print("Command received: increasing AFC mu to "); audioHardware.println(feedbackCanceler.setMu(new_val),6);
      break;
    case 'M':
      old_val = feedbackCanceler.getMu(); new_val = old_val / 2.0;
      audioHardware.print("Command received: decreasing AFC mu to "); audioHardware.println(feedbackCanceler.setMu(new_val),6);
      break;
    case 'r':
      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)/sqrt(2.0));
      audioHardware.print("Command received: increasing AFC rho to "); audioHardware.println(feedbackCanceler.setRho(new_val),6);
      break;
    case 'R':
      old_val = feedbackCanceler.getRho(); new_val = 1.0-((1.0-old_val)*sqrt(2.0));
      audioHardware.print("Command received: increasing AFC rho to "); audioHardware.println(feedbackCanceler.setRho(new_val),6);
      break;
    case 'e':
      old_val = feedbackCanceler.getEps(); new_val = old_val*sqrt(10.0);
      audioHardware.print("Command received: increasing AFC eps to "); audioHardware.println(feedbackCanceler.setEps(new_val),6);
      break;
    case 'E':
      old_val = feedbackCanceler.getEps(); new_val = old_val/sqrt(10.0);
      audioHardware.print("Command received: increasing AFC eps to "); audioHardware.println(feedbackCanceler.setEps(new_val),6);
      break;    
    case 'x':
      old_val = feedbackCanceler.getAfl(); new_val = old_val + 5;
      audioHardware.print("Command received: increasing AFC filter length to "); audioHardware.println(feedbackCanceler.setAfl(new_val));
      break;    
    case 'X':
      old_val = feedbackCanceler.getAfl(); new_val = old_val - 5;
      audioHardware.print("Command received: decreasing AFC filter length to "); audioHardware.println(feedbackCanceler.setAfl(new_val));
      break;            
//    case 'z':
//      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val + 5;
//      audioHardware.print("Command received: increasing AFC N_Coeff_To_Zero to "); audioHardware.println(feedbackCanceler.setNCoeffToZero(new_val));
//      break;
//    case 'Z':
//      old_val = feedbackCanceler.getNCoeffToZero(); new_val = old_val - 5;
//      audioHardware.print("Command received: decreasing AFC N_Coeff_To_Zero to "); audioHardware.println(feedbackCanceler.setNCoeffToZero(new_val));      
//      break;
    case 'u':
    {
      old_val = audioHardware.getHPCutoff_Hz(); new_val = min(old_val*sqrt(2.0), 8000.0); //half-octave steps up
      float fs_Hz = audioHardware.getSampleRate_Hz();
      audioHardware.setHPFonADC(true,new_val,fs_Hz);
      audioHardware.print("Command received: Increasing ADC HP Cutoff to "); audioHardware.print(audioHardware.getHPCutoff_Hz());audioHardware.println(" Hz");
    }
      break;
    case 'U':
    {
      old_val = audioHardware.getHPCutoff_Hz(); new_val = max(old_val/sqrt(2.0), 5.0); //half-octave steps down
      float fs_Hz = audioHardware.getSampleRate_Hz();
      audioHardware.setHPFonADC(true,new_val,fs_Hz);
      audioHardware.print("Command received: Decreasing ADC HP Cutoff to "); audioHardware.print(audioHardware.getHPCutoff_Hz());audioHardware.println(" Hz");   
      break;
    }
  }
}

void SerialManager::incrementChannelGain(int chan, float change_dB) {
  if (chan < N_CHAN) {
    gain_algorithms[chan].incrementGain_dB(change_dB);
    //audioHardware.print("Incrementing gain on channel ");audioHardware.print(chan);
    //audioHardware.print(" by "); audioHardware.print(change_dB); audioHardware.println(" dB");
    printGainSettings();  //in main sketch file
  }
}

#endif
