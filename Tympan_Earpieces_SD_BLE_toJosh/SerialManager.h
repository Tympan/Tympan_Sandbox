    
#ifndef SerialManager_h
#define SerialManager_h

#include <Tympan_Library.h>


#define MAX_DATASTREAM_LENGTH 1024
#define DATASTREAM_START_CHAR (char)0x02
#define DATASTREAM_SEPARATOR (char)0x03
#define DATASTREAM_END_CHAR (char)0x04
enum read_state_options {
  SINGLE_CHAR,
  STREAM_LENGTH,
  STREAM_DATA
};


//Extern variables
extern Tympan myTympan;
extern AudioSDWriter_F32 audioSDWriter;
extern State myState;
extern BLE ble;
extern EarpieceMixer_F32 earpieceMixer;


extern const int LEFT, RIGHT, FRONT, REAR;
extern const int ANALOG_IN, PDM_IN;

//Extern Functions (from main sketch file)
//extern int setAnalogInputSource(int);
//extern int setInputAnalogVsPDM(int);
//extern void setInputMixer(int);
extern float incrementInputGain(float);

extern float incrementKnobGain(float);
extern void printGainSettings(void);
extern float incrementRearMicGain(float);

//now, define the Serial Manager class
class SerialManager {
  public:
    SerialManager(void) {  };
      
    void respondToByte(char c);
    void processSingleCharacter(char c);
    void processStream(void);
    int readStreamIntArray(int idx, int* arr, int len);
    int readStreamFloatArray(int idx, float* arr, int len);
    
    void printHelp(void); 
    const float INCREMENT_INPUTGAIN_DB = 2.5f;  
    const float INCREMENT_HEADPHONE_GAIN_DB = 2.5f;  

    void printTympanRemoteLayout(void);
    void printCPUtoGUI(unsigned long, unsigned long);    
    void setFullGUIState(void);
    void setInputConfigButtons(void);    
    void setButtonState_rearMic(void);
    void setButtonState_frontRearMixer(void);
    void setButtonState_inputMixer(void);
    void setButtonState_gains(void);
    void setSDRecordingButtons(void);
    void setInputGainButtons(void);
    void setOutputGainButtons(void);
    
    void setButtonState(String btnId, bool newState);
    void setButtonText(String btnId, String text);
    void setMicConfigButtons(bool disableAll);

    float rearMicGainIncrement_dB = 0.5f;
    int N_CHAN;
    
    int serial_read_state; // Are we reading one character at a time, or a stream?
    char stream_data[MAX_DATASTREAM_LENGTH];
    int stream_length;
    int stream_chars_received;

};

void SerialManager::printHelp(void) {
  myTympan.println();
  myTympan.println("SerialManager Help: Available Commands:");
  myTympan.println(" h: Print this help");
  myTympan.println(" g: Print the gain settings of the device.");
  myTympan.println(" c/C: Enablel/Disable printing of CPU and Memory usage");
  myTympan.println(" w/W/e/E: Inputs: Use PCB Mics, Mic on Jack, Line on Jack, PDM Earpieces");
  myTympan.println(" t/T/H: Inputs: Use Front Mic, Inverted-Delayed Mix of Mics, or Rear Mic");
  myTympan.print  (" i/I: Rear Mic: incr/decr rear gain by "); myTympan.print(rearMicGainIncrement_dB); myTympan.println(" dB");
  myTympan.print  (" y/Y: Rear Mic: incr/decr rear delay by one sample (currently "); myTympan.print(myState.earpieceMixer->targetRearDelay_samps); myTympan.println(")");
  myTympan.print  (" i/I: Increase/Decrease Input Gain by "); myTympan.print(INCREMENT_INPUTGAIN_DB); myTympan.println(" dB");
  myTympan.print  (" k/K: Increase/Decrease Headphone Volume by "); myTympan.print(INCREMENT_HEADPHONE_GAIN_DB); myTympan.println(" dB");
  myTympan.println(" q,Q: Mute or Unmute the audio.");
  myTympan.println(" s,S,B: Left, Right, or Mono-mix the audio");
  myTympan.println(" `,~,|: SD: begin/stop/deleteAll recording");  
  myTympan.println();
}


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
        if ((c != '\r') && (c != '\n')) {
          Serial.print("Processing character ");
          Serial.println(c);
          processSingleCharacter(c);
        }
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
          myTympan.print("ERROR: Expected string terminator ");
          myTympan.print(DATASTREAM_END_CHAR, HEX);
          myTympan.print("; found ");
          myTympan.print(c,HEX);
          myTympan.println(" instead.");
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
    case 'h':
      printHelp(); break;
    case 'g':
      printGainSettings(); break;
    case 'k':
      //incrementKnobGain(channelGainIncrement_dB); 
      setButtonState_gains();
      setButtonState("inp_mute",false);
      break;
    case 'K':   //which is "shift k"
      //incrementKnobGain(-channelGainIncrement_dB); 
      setButtonState_gains(); 
      setButtonState("inp_mute",false);
      break;
  
    case 'c':
      Serial.println("Received: printing memory and CPU.");
      myState.flag_printCPUandMemory = true;
      myState.flag_printCPUtoGUI = true;
      setButtonState("cpuStart",true);
      break;
    case 'C':
      Serial.println("Received: stopping printing of memory and CPU.");
      myState.flag_printCPUandMemory = false;
      myState.flag_printCPUtoGUI = false;
      setButtonState("cpuStart",false);
      break;
   case 'w':
      myTympan.println("Received: Listen to PCB mics");
      earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_PCBMICS);
      //setInputMixer(State::MIC_AIC0_LR);  //PCB Mics are only on the main Tympan board
      setFullGUIState();
      break;
    case 'W':
      myTympan.println("Received: Mic jack as mic");
      earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_MIC);
      //setInputMixer(myState.analog_mic_config); //could be on either board, so remember the last-used state
      setFullGUIState();
      break;
    case 'e':
      myTympan.println("Received: Mic jack as line-in");
      earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_ANALOG);
      earpieceMixer.setAnalogInputSource(EarpieceMixerState::INPUT_MICJACK_LINEIN);
      //setInputMixer(myState.analog_mic_config);
      setFullGUIState();
      break;     
    case 'E':
      myTympan.println("Received: Listen to PDM mics"); //digital mics
      earpieceMixer.setInputAnalogVsPDM(EarpieceMixerState::INPUT_PDM);
      //setInputMixer(myState.digital_mic_config);
      setFullGUIState();
      break;  
    case 't':
      myTympan.println("Received: Front Mics (if using earpieces)"); 
      earpieceMixer.configureFrontRearMixer(EarpieceMixerState::MIC_FRONT);
      setButtonState_frontRearMixer();
      break;
    case 'T':
      myTympan.println("Received: Mix of Front Mics + Delayed Inverted Rear Mics"); 
      earpieceMixer.configureFrontRearMixer(EarpieceMixerState::MIC_BOTH_INVERTED_DELAYED);
      setButtonState_frontRearMixer();
      break;
     case 'H':
      myTympan.println("Received: Rear Mics (if using earpieces)"); 
      earpieceMixer.configureFrontRearMixer(EarpieceMixerState::MIC_REAR);
      setButtonState_frontRearMixer();
      break;
    case 'i':
      new_val = incrementRearMicGain(rearMicGainIncrement_dB);
      myTympan.print("Adjusting rear mic gain to "); myTympan.print(new_val); myTympan.println(" dB");
      setButtonState_rearMic();
      break;
    case 'I':
      new_val = incrementRearMicGain(-rearMicGainIncrement_dB);
      myTympan.print("Adjusting rear mic gain to "); myTympan.print(new_val); myTympan.println(" dB");
      setButtonState_rearMic();
      break;
    case 'y':
      { 
        int new_val = myState.earpieceMixer->targetRearDelay_samps+1;      
        myTympan.print("Received: Increase rear-mic delay by one sample to ");myTympan.println(new_val);
        earpieceMixer.setTargetMicDelay_samps(REAR, new_val);
        setButtonState_frontRearMixer();
      }
      break;
    case 'Y':
      myTympan.println("Received: Reduce rear-mic delay by one sample");
      earpieceMixer.setTargetMicDelay_samps(REAR, myState.earpieceMixer->targetRearDelay_samps-1);
      setButtonState_frontRearMixer();
      break;
    case 'q':
      earpieceMixer.configureLeftRightMixer(EarpieceMixerState::INPUTMIX_MUTE);
      myTympan.println("Received: Muting audio.");
      setButtonState_inputMixer();
      break;
    case 'Q':
      earpieceMixer.configureLeftRightMixer(myState.earpieceMixer->input_mixer_nonmute_config);
      setButtonState_inputMixer();
      break;  
    case 'B':
      myTympan.println("Received: Input: Mix L+R.");
      earpieceMixer.configureLeftRightMixer(EarpieceMixerState::INPUTMIX_MONO);
      setButtonState_inputMixer();
      break;  
    case 's':
      earpieceMixer.configureLeftRightMixer(EarpieceMixerState::INPUTMIX_BOTHLEFT);
      myTympan.println("Received: Input: Both Left.");
      setButtonState_inputMixer();
      break;
    case 'S':
      earpieceMixer.configureLeftRightMixer(EarpieceMixerState::INPUTMIX_BOTHRIGHT);
      myTympan.println("Received: Input: Both Right.");
      setButtonState_inputMixer();
      break;   
    case 'J':
    {
      printTympanRemoteLayout();
      delay(20);
      //sendStreamDSL(myState.wdrc_perBand);
      //sendStreamGHA(myState.wdrc_broadband);
      //sendStreamAFC(myState.afc);
      setFullGUIState();
      break;
    }
  
    case '`':
      myTympan.println("Received: begin SD recording");
      audioSDWriter.startRecording();
      setSDRecordingButtons();
      break;
    case '~':
      myTympan.println("Received: stop SD recording");
      audioSDWriter.stopRecording();
      setSDRecordingButtons();
      break;
    case '|':
      myTympan.println("Recieved: delete all SD recordings.");
      audioSDWriter.stopRecording();
      audioSDWriter.deleteAllRecordings();
      setSDRecordingButtons();
      myTympan.println("Delete all SD recordings complete.");

      break;   
  }
}

// Print the layout for the Tympan Remote app, in a JSON-ish string
// (single quotes are used here, whereas JSON spec requires double quotes.  The app converts ' to " before parsing the JSON string).
// If you don't give a button an id, then it will be assigned the id 'default'.  IDs don't have to be unique; if two buttons have the same id,
// then you always control them together and they'll always have the same state. 
// Please don't put commas or colons in your ID strings!
// The 'icon' is how the device appears in the app - could be an icon, could be a pic of the device.  Put the
// image in the TympanRemote app in /src/assets/devIcon/ and set 'icon' to the filename.
void SerialManager::printTympanRemoteLayout(void) {
  char jsonConfig[] = "JSON={"
    "'icon':'creare.png',"
    "'pages':["
      "{'title':'Input Select','cards':["
        "{'name':'Audio Source', 'buttons':["
                                           "{'label':'Digital: Earpieces', 'cmd': 'd', 'id':'configPDMMic', 'width':'12'},"
                                           "{'label':'Analog: PCB Mics',  'cmd': 'w', 'id':'configPCBMic',  'width':'12'},"
                                           "{'label':'Analog: Mic Jack (Mic)',  'cmd': 'W', 'id':'configMicJack', 'width':'12'},"
                                           "{'label':'Analog: Mic Jack (Line)',  'cmd': 'e', 'id':'configLineJack', 'width':'12'},"
                                           "{'label':'Analog: BT Audio', 'cmd': 'E', 'id':'configLineSE',  'width':'12'}" //don't have a trailing comma on this last one
                                          "]},"
        "{'name':'Output Gain',  'buttons':["
                                          "{'label':'-', 'cmd':'K', 'width':'4'},"  //comma
                                          "{'id':'outGain', 'label':'0', 'width':'4'}," //comma
                                          "{'label':'+', 'cmd':'k', 'width':'4'}]},"//don't have a trailing comma on this last one
        "{'name':'Digital and Analog Options','buttons':["
                                           "{'label':'Swipe Left'}" //no comma for last one
                                          "]}"  //don't have a trailing comma on this last one
       "]}," //close off this page (title).  Add comma because there are more pages

      "{'title':'Digital Earpieces','cards':["
          "{'name':'Front or Rear Mics', 'buttons':["
                                                  "{'label':'Front','id':'frontMic','cmd':'9','width':'4'}," //comma
                                                  "{'label':'Mix(F-R)','id':'frontRearMix','cmd':')','width':'4'}," //comma
                                                  "{'label':'Rear','id':'rearMic','cmd':'(','width':'4'}"  //no comma for last
                                                 "]},"  //include trailing comma if NOT the last one
           "{'name':'Rear Mic Gain (dB)', 'buttons':["
                                                  "{'label':'-','cmd':'I','width':'4'}," //comma
                                                  "{'label':'','id':'rearGain','width':'4'}," //comma
                                                  "{'label':'+','cmd':'i','width':'4'}"  //no comma for last
                                                 "]}," //include trailing comma if NOT the last one
           "{'name':'Rear Mic Delay (samples)', 'buttons':["
                                                   "{'label':'-','cmd':'Y','width':'4'},"  //comma
                                                   "{'label':'','id':'delaySamps','width':'4'}," //comma
                                                   "{'label':'+','cmd':'y','width':'4'}"  //no comma for last
                                                  "]}"  //exclude trailing comma if it IS the last one   
       "]}," //close off this page ('Digital Earpieces').  Add comma because there are more pages 
                                                           
      //"{'title':'Analog Input Settings','cards':["
      //  "{'name':'Analog Input Source','buttons':[{'label': 'Tympan Board', 'cmd': '-', 'id':'micsAIC0', 'width':'12'},{'label': 'Earpiece Shield', 'cmd': '_', 'id':'micsAIC1', 'width':'12'}, {'label': 'Mix Both','cmd':'=', 'id':'micsBothAIC', 'width':'12'}]},"
      //  "{'name':'Analog Input Gain', 'buttons':[{'label':'-', 'cmd':'I', 'width':'4'},{'id':'inGain',  'label':'0', 'width':'4'},{'label':'+', 'cmd':'i', 'width':'4'}]},"  //whichever line is the last line, NO TRAILING COMMA!
      //  "{'name':'Output Gain',    'buttons':[{'label':'-', 'cmd':'K', 'width':'4'},{'id':'outGain', 'label':'0', 'width':'4'},{'label':'+', 'cmd':'k', 'width':'4'}]}"//don't have a trailing comma on this last one
      //"]},"
      
      "{'title':'Global','cards':["
          "{'name':'CPU Usage (%)', 'buttons':["
                                              "{'label': 'Start', 'cmd' :'c','id':'cpuStart','width':'4'}," //comma
                                              "{'id':'cpuValue', 'label': '', 'width':'4'}," //comma
                                              "{'label': 'Stop', 'cmd': 'C','width':'4'}" //no comma for last
                                             "]},"  //add comma if you add any lines below before this line's closing quote
          "{'name':'Record Mics to SD Card','buttons':["
                                              "{'label': 'Start', 'cmd': 'r', 'id':'recordStart','width':'6'}," //comma
                                              "{'label': 'Stop', 'cmd': 's','width':'6'}," //comma
                                              "{'label': '', 'id':'sdFname','width':'12'}" //no comma for last
                                             "]}"  //no comma for last
      "]}" //close off this page ('Global').  No comma for last page                      
    "]"  // close off all pages
  "}";  //close off the JSON
  myTympan.println(jsonConfig);
  ble.sendMessage(jsonConfig);
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

  //myTympan.print("Received stream: ");
  //myTympan.print(stream_data);

  if (streamType == "gha") {    
    myTympan.println("Stream is of type 'gha'.");
    //interpretStreamGHA(idx);
  } else if (streamType == "dsl") {
    myTympan.println("Stream is of type 'dsl'.");
    //interpretStreamDSL(idx);
  } else if (streamType == "afc") {
    myTympan.println("Stream is of type 'afc'.");
    //interpretStreamAFC(idx);
  } else if (streamType == "test") {    
    myTympan.println("Stream is of type 'test'.");
    tmpInt = *((int*)(stream_data+idx)); idx = idx+4;
    myTympan.print("int is "); myTympan.println(tmpInt);
    tmpFloat = *((float*)(stream_data+idx)); idx = idx+4;
    myTympan.print("float is "); myTympan.println(tmpFloat);
  } else {
    myTympan.print("Unknown stream type: ");
    myTympan.println(streamType);
  }
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

void SerialManager::printCPUtoGUI(unsigned long curTime_millis = 0, unsigned long updatePeriod_millis = 0) {
  static unsigned long lastUpdate_millis = 0;
  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) >= updatePeriod_millis) { //is it time to update the user interface?
    setButtonText("cpuValue", String(myState.getCPUUsage(),1));
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

void SerialManager::setFullGUIState(void) {
  //setButtonState_algPresets();
  //setButtonState_afc();
  setButtonState_rearMic();
  setButtonState_frontRearMixer();
  setButtonState_inputMixer();
  setInputConfigButtons();

  setButtonState_gains();
  //setButtonState_tunerGUI();
  setSDRecordingButtons();  
  
  //update buttons related to the "print" states
  if (myState.flag_printCPUandMemory) {
    setButtonState("cpuStart",true);
  } else {
    setButtonState("cpuStart",false);
  }
}

void SerialManager::setButtonState_gains(void) {
}

void SerialManager::setSDRecordingButtons(void) {
  if (audioSDWriter.getState() == AudioSDWriter_F32::STATE::RECORDING) {
    setButtonState("recordStart",true);
  } else {
    setButtonState("recordStart",false);
  }
  setButtonText("sdFname",audioSDWriter.getCurrentFilename());
}


void SerialManager::setButtonState_rearMic(void) {
  setButtonText("rearGain",String(myState.earpieceMixer->rearMicGain_dB,1));
}
void SerialManager::setButtonState_frontRearMixer(void) {
  setButtonState("frontMic",false);
  setButtonState("frontRearMix",false);
  setButtonState("rearMic",false);
  
  switch (myState.earpieceMixer->input_frontrear_config) {
    case EarpieceMixerState::MIC_FRONT:
      setButtonState("frontMic",true);
      break;
    case EarpieceMixerState::MIC_BOTH_INVERTED_DELAYED:
      setButtonState("frontRearMix",true);
      break;
    case EarpieceMixerState::MIC_REAR:
      setButtonState("rearMic",true);
      break;    
  }
 
  Serial.print("setButtonState_frontRearMixer: setting rear delaySamps field to ");
  Serial.println(String(myState.earpieceMixer->currentRearDelay_samps));
  setButtonText("delaySamps",String(myState.earpieceMixer->currentRearDelay_samps));

  Serial.print("setButtonState_frontRearMixer: setting front delaySamps field to ");
  Serial.println(String(myState.earpieceMixer->currentFrontDelay_samps));
  setButtonText("delaySampsF",String(myState.earpieceMixer->currentFrontDelay_samps));
}
void SerialManager::setButtonState_inputMixer(void) {
   setButtonState("inp_mute",false);
   setButtonState("inp_mono",false); 
   setButtonState("inp_micL",false);
   setButtonState("inp_micR",false);
   //setButtonState("inp_stereo",flae);  delay(3); 
  
  switch (myState.earpieceMixer->input_mixer_config) {
    case (EarpieceMixerState::INPUTMIX_STEREO):
      setButtonState("inp_stereo",true);  delay(3); 
      break;
    case (EarpieceMixerState::INPUTMIX_MONO):
      setButtonState("inp_mono",true); delay(3);
      break;
    case (EarpieceMixerState::INPUTMIX_MUTE):
      setButtonState("inp_mute",true);  delay(3); 
      break;
    case (EarpieceMixerState::INPUTMIX_BOTHLEFT):
      setButtonState("inp_micL",true);  delay(3); 
      break;
    case (EarpieceMixerState::INPUTMIX_BOTHRIGHT):
      setButtonState("inp_micR",true);  delay(3); 
      break;
  }
}


//void SerialManager::setMicConfigButtons(bool disableAll) {
//
//  //clear any previous state of the buttons
//  setButtonState("micsFront",false); delay(10);
//  setButtonState("micsRear",false); delay(10);
//  setButtonState("micsBoth",false); delay(10);
//  setButtonState("micsBothInv",false); delay(10);
//  setButtonState("micsAIC0",false); delay(10);
//  setButtonState("micsAIC1",false); delay(10);
//  setButtonState("micsBothAIC",false); delay(10);
//
//  //now, set the one button that should be active
//  int mic_config = myState.input_frontrear_config;
//  if (disableAll == false) {
//    switch (mic_config) {
//      case myState.MIC_FRONT:
//        setButtonState("micsFront",true);
//        break;
//      case myState.MIC_REAR:
//        setButtonState("micsRear",true);
//        break;
//      case myState.MIC_BOTH_INPHASE:
//        setButtonState("micsBoth",true);
//        break;
//      case myState.MIC_BOTH_INVERTED: case myState.MIC_BOTH_INVERTED_DELAYED:
//        setButtonState("micsBothInv",true);
//        break;
//
//    }
//  }
//}


void SerialManager::setInputConfigButtons(void) {
  //clear out previous state of buttons
  setButtonState("configPDMMic",false);  delay(10);
  setButtonState("configPCBMic",false);  delay(10);
  setButtonState("configMicJack",false); delay(10);
  setButtonState("configLineJack",false);delay(10);
  setButtonState("configLineSE",false);  delay(10);

  //set the new state of the buttons
  if (myState.earpieceMixer->input_analogVsPDM == EarpieceMixerState::INPUT_PDM) {
      setButtonState("configPDMMic",true);  delay(10);
  } else {
      switch (myState.earpieceMixer->input_analog_config) {
        case (EarpieceMixerState::INPUT_PCBMICS):
          setButtonState("configPCBMic",true);  delay(10); break;
        case (EarpieceMixerState::INPUT_MICJACK_MIC): 
          setButtonState("configMicJack",true); delay(10); break;
        case (EarpieceMixerState::INPUT_LINEIN_SE): 
          setButtonState("configLineSE",true);  delay(10); break;
        case (EarpieceMixerState::INPUT_MICJACK_LINEIN): 
          setButtonState("configLineJack",true);delay(10); break;
      }  
  }
}
void SerialManager::setInputGainButtons(void) {
  setButtonText("inGain",String(myState.earpieceMixer->inputGain_dB,1));
}
void SerialManager::setOutputGainButtons(void) {
  setButtonText("outGain",String(myState.volKnobGain_dB,1));
}

void SerialManager::setButtonState(String btnId, bool newState) {
  if (newState) {
    myTympan.println("STATE=BTN:" + btnId + ":1");
    ble.sendMessage("STATE=BTN:" + btnId + ":1");
  } else {
    myTympan.println("STATE=BTN:" + btnId + ":0");
    ble.sendMessage("STATE=BTN:" + btnId + ":0");
  }
}

void SerialManager::setButtonText(String btnId, String text) {
  myTympan.println("TEXT=BTN:" + btnId + ":"+text);
  ble.sendMessage("TEXT=BTN:" + btnId + ":"+text);
}





#endif
