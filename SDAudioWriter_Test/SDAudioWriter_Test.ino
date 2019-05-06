/*
   SD Audio Writer Testing

   Created: Chip Audette, OpenAudio, May 2019
   Purpose: Designed or use with Tympan 

   MIT License.  use at your own risk.
*/


// Include all the of the needed libraries
#include <Tympan_Library.h>

// State constants
const int INPUT_PCBMICS = 0, INPUT_MICJACK = 1, INPUT_LINEIN_SE = 2, INPUT_LINEIN_JACK = 3;

//define state
#define NO_STATE (-1)
class State_t {
  public:
    int input_source = NO_STATE;
};

//local files
#include "AudioSDWriter.h"
#include "SerialManager.h"

//definitions for memory for SD writing
#define MAX_F32_BLOCKS (60)      //Can't seem to use more than 192, so you could set it to 192.  Won't run at all if much above 400.  


//set the sample rate and block size
const float sample_rate_Hz = 96000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 128;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)  Must be 128 for SD recording.
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Define the overall setup
String overall_name = String("Tympan: SD Audio Writer Tester");
float default_input_gain_dB = 5.0f; //gain on the microphone
float input_gain_dB = default_input_gain_dB;
float vol_knob_gain_dB = 0.0; //will be overridden by volume knob, if used
float output_volume_dB = 0.0; //volume setting of output PGA

// /////////// Define audio objects...they are configured later

//create audio library objects for handling the audio
Tympan                        myTympan(TympanRev::D);
AudioInputI2S_F32             i2s_in(audio_settings);   //Digital audio input from the ADC
AudioSynthWaveform_F32        waveform(audio_settings);
AudioSDWriter_F32             audioSDWriter(audio_settings); //this is stereo by default
AudioOutputI2S_F32            i2s_out(audio_settings);  //Digital audio output to the DAC.  Should always be last.

//Connect to outputs
AudioConnection_F32           patchcord500(i2s_in, 0, i2s_out, 0);    //Left mixer to left output
AudioConnection_F32           patchcord501(i2s_in, 1, i2s_out, 1);    //Right mixer to right output

//Connect to SD logging
AudioConnection_F32           patchcord600(i2s_in, 0, audioSDWriter, 0);   //connect Raw audio to left channel of SD writer
AudioConnection_F32           patchcord601(waveform, 0, audioSDWriter, 1);   //connect Raw audio to right channel of SD writer


//control display and serial interaction
bool enable_printCPUandMemory = false;
void togglePrintMemoryAndCPU(void) {
  enable_printCPUandMemory = !enable_printCPUandMemory;
}; //"extern" let's be it accessible outside
void setPrintMemoryAndCPU(bool state) {
  enable_printCPUandMemory = state;
};
bool enable_printAveSignalLevels = false;
bool printAveSignalLevels_as_dBSPL = false;
void togglePrintAveSignalLevels(bool as_dBSPL) {
  enable_printAveSignalLevels = !enable_printAveSignalLevels;
  printAveSignalLevels_as_dBSPL = as_dBSPL;
};
SerialManager serialManager;
#define BOTH_SERIAL myTympan

//keep track of state
State_t myState;

int current_config = 0;
void setConfiguration(int config) {

  //myTympan.volume_dB(-60.0); delay(50);  //mute the output audio
  myState.input_source = config;

  switch (config) {
    case INPUT_PCBMICS:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on-board microphones

      //Set input gain to 0dB
      input_gain_dB = 15;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = INPUT_PCBMICS;
      break;

    case INPUT_MICJACK:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the mic jack
      myTympan.setEnableStereoExtMicBias(true);

      //Set input gain to 0dB
      input_gain_dB = default_input_gain_dB;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = INPUT_MICJACK;
      break;
    case INPUT_LINEIN_JACK:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the line-input through holes

      //Set input gain to 0dB
      input_gain_dB = 0;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = INPUT_LINEIN_JACK;
      break;
    case INPUT_LINEIN_SE:
      //Select Input
      myTympan.inputSelect(TYMPAN_INPUT_LINE_IN); // use the line-input through holes

      //Set input gain to 0dB
      input_gain_dB = default_input_gain_dB;
      myTympan.setInputGain_dB(input_gain_dB);

      //Store configuration
      current_config = INPUT_LINEIN_SE;
      break;
  }

  //bring the output volume back up
  //delay(50);  myTympan.volume_dB(output_volume_dB);  // output amp: -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit
}

// ///////////////// Main setup() and loop() as required for all Arduino programs

// define the setup() function, the function that is called once when the device is booting
void setup() {
  delay(100);

  myTympan.beginBothSerial(); delay(1000);
  BOTH_SERIAL.print(overall_name); BOTH_SERIAL.println(": setup():...");
  BOTH_SERIAL.print("Sample Rate (Hz): "); BOTH_SERIAL.println(audio_settings.sample_rate_Hz);
  BOTH_SERIAL.print("Audio Block Size (samples): "); BOTH_SERIAL.println(audio_settings.audio_block_samples);

  //allocate the audio memory
  AudioMemory_F32(MAX_F32_BLOCKS, audio_settings); //I can only seem to allocate 400 blocks
  BOTH_SERIAL.println("Setup: memory allocated.");

  //activate the Tympan audio hardware
  myTympan.enable();        // activate AIC
  myTympan.volume_dB(output_volume_dB);  // output amp: -63.6 to +24 dB in 0.5dB steps.  uses signed 8-bit

  //Configure for Tympan PCB mics
  BOTH_SERIAL.println("Setup: Using Mic Jack with Mic Bias.");
  setConfiguration(INPUT_MICJACK); //this will also unmute the system

  //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  audioSDWriter.setWriteDataType(AudioSDWriter::WriteDataType::INT16);  //this is the built-in the default, but here you could change it to FLOAT32
  audioSDWriter.setNumWriteChannels(2);             //this is also the defaullt, but you could set it to 2
  //audioSDWriter.setWriteSizeBytes(4*512);  //most efficient in 512 or bigger (always in multiples of 512?)

  //setup saw wav (as a test signal)
  waveform.oscillatorMode(AudioSynthWaveform_F32::OscillatorMode::OSCILLATOR_MODE_SAW);
  waveform.frequency(1000.0);
  waveform.amplitude(0.25);
  
  //End of setup
  BOTH_SERIAL.println("Setup: complete."); serialManager.printHelp();

} //end setup()

// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  //asm(" WFI");  //save power by sleeping.  Wakes when an interrupt is fired (usually by the audio subsystem...so every 256 audio samples)

  //respond to Serial commands
  if (Serial.available()) serialManager.respondToByte((char)Serial.read());   //USB Serial
  if (Serial1.available()) serialManager.respondToByte((char)Serial1.read()); //BT Serial

  //service the SD recording
  serviceSD();

  //update the memory and CPU usage...if enough time has passed
  if (enable_printCPUandMemory) printCPUandMemory(millis());

  //service the LEDs
  serviceLEDs();

} //end loop()



// ///////////////// Servicing routines

void printCPUandMemory(unsigned long curTime_millis) {
  static unsigned long updatePeriod_millis = 3000; //how many milliseconds between updating gain reading?
  static unsigned long lastUpdate_millis = 0;

  //has enough time passed to update everything?
  if (curTime_millis < lastUpdate_millis) lastUpdate_millis = 0; //handle wrap-around of the clock
  if ((curTime_millis - lastUpdate_millis) > updatePeriod_millis) { //is it time to update the user interface?
    printCPUandMemoryMessage();
    lastUpdate_millis = curTime_millis; //we will use this value the next time around.
  }
}

extern "C" char* sbrk(int incr);
int FreeRam() {
  char top; //this new variable is, in effect, the mem location of the edge of the heap
  return &top - reinterpret_cast<char*>(sbrk(0));
}
void printCPUandMemoryMessage(void) {
  BOTH_SERIAL.print("CPU Cur/Pk: ");
  BOTH_SERIAL.print(audio_settings.processorUsage(), 1);
  BOTH_SERIAL.print("%/");
  BOTH_SERIAL.print(audio_settings.processorUsageMax(), 1);
  BOTH_SERIAL.print("%, ");
  BOTH_SERIAL.print("MEM Cur/Pk: ");
  BOTH_SERIAL.print(AudioMemoryUsage_F32());
  BOTH_SERIAL.print("/");
  BOTH_SERIAL.print(AudioMemoryUsageMax_F32());
  BOTH_SERIAL.print(", FreeRAM(B) ");
  BOTH_SERIAL.print(FreeRam());
  BOTH_SERIAL.println();
}


void serviceLEDs(void) {
  static int loop_count = 0;
  loop_count++;
  
  if (audioSDWriter.getState() == AudioSDWriter::STATE::UNPREPARED) {
    //myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW); //Turn ON RED
    if (loop_count > 200000) {  //slow toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } else if (audioSDWriter.getState() == AudioSDWriter::STATE::RECORDING) {
    //myTympan.setRedLED(LOW); myTympan.setAmberLED(HIGH); //Go Amber

    //let's flicker the LEDs while writing
    loop_count++;
    if (loop_count > 20000) { //fast toggle
      loop_count = 0;
      toggleLEDs(true,true); //blink both
    }
  } else {
    //myTympan.setRedLED(HIGH); myTympan.setAmberLED(LOW); //Go Red
    if (loop_count > 200000) { //slow toggle
      loop_count = 0;
      toggleLEDs(false,true); //just blink the red
    }
  }
}

void toggleLEDs(void) {
  toggleLEDs(true,true);  //toggle both
}
void toggleLEDs(const bool &useAmber, const bool &useRed) {
  static bool LED = false;
  LED = !LED;
  if (LED) {
    if (useAmber) myTympan.setAmberLED(true);
    if (useRed) myTympan.setRedLED(false);
  } else {
    if (useAmber) myTympan.setAmberLED(false);
    if (useRed) myTympan.setRedLED(true);
  }

  if (!useAmber) myTympan.setAmberLED(false);
  if (!useRed) myTympan.setRedLED(false);
  
  //Serial.println("toggleLEDs: toggled.");
}

#define PRINT_OVERRUN_WARNING 1   //set to 1 to print a warning that the there's been a hiccup in the writing to the SD.
void serviceSD(void) {
  static int write_counter = 0;
  static int max_max_bytes_written = 0;
  static int max_bytes_written = 0;
  static int max_dT_micros = 0;
  static int max_max_dT_micros = 0;
  int bytes_written = 0;
  static float max_micros_per_byte = 0;
  static float max_max_micros_per_byte = 0;

  unsigned long dT_micros = micros();
  bytes_written = audioSDWriter.serviceSD();
  dT_micros = micros() - dT_micros;

  if ( bytes_written > 0 ) {
    //if we're here, data was written to the SD, so do some checking of the timing...
    write_counter++;

    float micros_per_byte = ((float)dT_micros)/((float)bytes_written);
    
    max_bytes_written = max(max_bytes_written, bytes_written);
    max_dT_micros = max((int)max_dT_micros, (int)dT_micros);
    max_micros_per_byte = max(micros_per_byte,max_micros_per_byte);
    
    //if (write_counter > 1000) {
    if (dT_micros > 1000) {
      max_max_bytes_written = max(max_bytes_written,max_max_bytes_written);
      max_max_dT_micros = max(max_dT_micros, max_max_dT_micros);
      max_max_micros_per_byte = max(max_micros_per_byte, max_max_micros_per_byte);
      
      Serial.print("serviceSD: bytes written = ");
      Serial.print(bytes_written); Serial.print(", ");
      Serial.print(max_bytes_written); Serial.print(", ");
      Serial.print(max_max_bytes_written); Serial.print(", ");
      Serial.print("dT millis = "); 
      Serial.print((float)dT_micros/1000.0,1); Serial.print(", ");
      Serial.print((float)max_dT_micros/1000.0,1); Serial.print(", "); 
      Serial.print((float)max_max_dT_micros/1000.0,1);Serial.print(", "); 
      //Serial.print("micros per byte = "); 
      //Serial.print(micros_per_byte,2); Serial.print(", ");
      //Serial.print(max_micros_per_byte,1); Serial.print(", "); 
      //Serial.print(max_max_micros_per_byte,1);Serial.print(", ");      
      Serial.println();
      max_bytes_written = 0;
      max_dT_micros = 0;
      write_counter = 0;
      
    }
      

    //print a warning if there has been an SD writing hiccup
    if (PRINT_OVERRUN_WARNING) {
      //if (audioSDWriter.getQueueOverrun() || i2s_in.get_isOutOfMemory()) {
      if (i2s_in.get_isOutOfMemory()) {
        float approx_time_sec = ((float)(millis()-audioSDWriter.getStartTimeMillis()))/1000.0;
        if (approx_time_sec > 0.1) {
          BOTH_SERIAL.print("SD Write Warning: there was a hiccup in the writing.");//  Approx Time (sec): ");
          BOTH_SERIAL.println(approx_time_sec );
        }
      }
    }

    //print timing information to help debug hiccups in the audio.  Are the writes fast enough?  Are there overruns?
    if (PRINT_FULL_SD_TIMING) {
      Serial.print("SD Write Status: ");
      //Serial.print(audioSDWriter.getQueueOverrun()); //zero means no overrun
      //Serial.print(", ");
      Serial.print(AudioMemoryUsageMax_F32());  //hopefully, is less than MAX_F32_BLOCKS
      Serial.print(", ");
      Serial.print(MAX_F32_BLOCKS);  // max possible memory allocation
      Serial.print(", ");
      Serial.println(i2s_in.get_isOutOfMemory());  //zero means i2s_input always had memory to work with.  Non-zero means it ran out at least once.

      //Now that we've read the flags, reset them.
      AudioMemoryUsageMaxReset_F32();
    }

    //audioSDWriter.clearQueueOverrun();
    i2s_in.clear_isOutOfMemory();
  } else {
    //no SD recording currently, so no SD action
  }
}

// //////////////////////////////////// Control the audio processing from the SerialManager
//here's a function to change the volume settings.   We'll also invoke it from our serialManager
void incrementInputGain(float increment_dB) {
  input_gain_dB += increment_dB;
  if (input_gain_dB < 0.0) {
    BOTH_SERIAL.println("Error: cannot set input gain less than 0 dB.");
    BOTH_SERIAL.println("Setting input gain to 0 dB.");
    input_gain_dB = 0.0;
  }
  myTympan.setInputGain_dB(input_gain_dB);
}


