/*
   MyAudioAlgorithm_SD

   Created: Chip Audette, OpenAudio, March 2021
   Purpose: Be a blank canvas for adding your own floating-point audio processing.
        Can read/write audio to SD card
        Processes a mono (single channel) audio stream

   MIT License.  use at your own risk.
*/

// /////////////////  AUDIO SETUP
#include <Tympan_Library.h>
#include "MyAudioAlgorithm_F32.h"  //include the file holding your new audio algorithm

//set the sample rate and block size
const float sample_rate_Hz = 24000.0f ; //24000 or 44117 (or other frequencies in the table in AudioOutputI2S_F32)
const int audio_block_samples = 32;     //do not make bigger than AUDIO_BLOCK_SAMPLES from AudioStream.h (which is 128)
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

//create audio library objects for handling the audio
Tympan						          myTympan(TympanRev::E, audio_settings);  //TympanRev::D or TympanRev::C
AudioInputI2S_F32           i2s_in(audio_settings);   //Digital audio *from* the Tympan AIC.
AudioSDPlayer_F32           audioSDPlayer(audio_settings);
AudioSummer4_F32            inputSwitch(audio_settings);
MyAudioAlgorithm_F32        myAlg(audio_settings); //This is your own algorithm form MyAudioAlgorithm_F32.h
AudioOutputI2S_F32          i2s_out(audio_settings);  //Digital audio *to* the Tympan AIC.  Always list last to minimize latency
AudioSDWriter_F32           audioSDWriter(audio_settings); //this is stereo by default

//Make all of the audio connections
#define INPUTSWITCH_LIVE 0
#define INPUTSWITCH_SD 1
AudioConnection_F32         patchCord1(i2s_in, 0, inputSwitch, INPUTSWITCH_LIVE);         //left input
AudioConnection_F32         patchCord3(audioSDPlayer, 0, inputSwitch, INPUTSWITCH_SD);    //left SD
AudioConnection_F32         patchCord11(inputSwitch, 0, myAlg, 0);         //process left
AudioConnection_F32         patchCord21(myAlg, 0, i2s_out, 0);     //left output
AudioConnection_F32         patchCord22(myAlg, 0, i2s_out, 1);     //right output
//AudioConnection_F32         patchCord23(i2s_out, 0, audioSDWriter, 0);   //left output to SD
//AudioConnection_F32         patchCord24(i2s_out, 1, audioSDWriter, 1);   //right output to SD
// ////////////////// END AUDIO SETUP


// SD Reading
bool use_sd_for_input = true;          //set to false to do live audio
char sd_read_fname[] = "testfile.wav"; //what filename do you want to read from?

// SD Writing
bool record_output_to_sd = true;       //set to false to turn off SD recording
char sd_write_fname[] = "testfile_output.wav"; //what filename do you want to write to?

// define the setup() function, the function that is called once when the device is booting
const float input_gain_dB = 20.0f; //gain on the microphone
void setup() {
  //begin the serial comms (for debugging)
  myTympan.beginBothSerial(); delay(1500); //both Serial (USB) and Serial1 (BT) are started here
  myTympan.println("MyAudioAlgorithm_SD: starting setup()..");

  //allocate the audio memory
  AudioMemory_F32(40,audio_settings); 

  //Enable the Tympan to start the audio flowing!
  myTympan.enable(); // activate AIC

  //Choose the desired input
  myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC); // use the on board microphones
  //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC); // use the microphone jack - defaults to mic bias 2.5V
  // myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF

  //Set the desired volume levels
  myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
  myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

  delay(500);

 //prepare the SD writer for the format that we want and any error statements
  audioSDWriter.setSerial(&myTympan);
  //audioSDWriter.setNumWriteChannels(2);    //You can set to 2 channels or to 4 channels
  //audioSDWriter.allocateBuffer(75000);   //I believe that 150000 is the default.  Set to smaller values to save RAM at runtime
  if (record_output_to_sd) {
    //myTympan.print("Setup: Opening for writing: "); myTympan.println(sd_write_fname);
    setupSdWriting(sd_write_fname);
  }

  delay(500);

   //prepare the SD reader
  inputSwitch.switchChannel(INPUTSWITCH_LIVE);         //default setting for getting live audio
  if (use_sd_for_input) {
    myTympan.print("Setup: Opening for reading: "); myTympan.println(sd_read_fname);
    setupSdReading(audioSDWriter.getSdPtr(), sd_read_fname); //if you want SD, this will change the input to INPUTSWITCH_SD
  }

  delay(500);
   
  myTympan.println("Setup complete.");

} //end setup()


// define the loop() function, the function that is repeated over and over for the life of the device
void loop() {
  
  //periodically print the CPU and Memory Usage
  myTympan.printCPUandMemory(millis(),3000); //print every 3000 msec

  //service the SD recording
  //serviceSdWriting();

  //service the SD reading
  serviceSdReading();

} //end loop()

// ///////////////// Servicing routines


void setupSdWriting(char *fname) { 
  //audioSDWriter.startRecording(fname);
}

void setupSdReading(SdFs *sd_ptr, char *fname) {
  audioSDPlayer.setSdPtr(sd_ptr);
  if (audioSDPlayer.play(fname) == false) {
    myTympan.print("setupSdReading: could not open ");
    myTympan.println(fname);
  } else {
    myTympan.print("setupSdReading: isFileOpen() = ");
    myTympan.println(audioSDPlayer.isFileOpen());
    myTympan.print("setupSdReading: isPlaying() = ");
    myTympan.println(audioSDPlayer.isPlaying());
    inputSwitch.switchChannel(INPUTSWITCH_SD);
  }
}


// service SD 
void serviceSdWriting(void) {
  int bytes_written = audioSDWriter.serviceSD();
  if ( bytes_written > 0 ) {  //only look for error messages if any data is being written
    
    if (i2s_in.get_isOutOfMemory()) {
      float approx_time_sec = ((float)(millis() - audioSDWriter.getStartTimeMillis())) / 1000.0;
      if (approx_time_sec > 0.1) {
        myTympan.print("serviceSdWriting: there was a hiccup in the writing.");
        myTympan.println(approx_time_sec );
      }
    }
    i2s_in.clear_isOutOfMemory();
  }
}


int count = 0;
void serviceSdReading(void) {
  if (audioSDPlayer.isFileOpen()) {
    //if SD is done playing, stop the recording, too
    if (audioSDPlayer.isPlaying() == false) {
      count++;

    myTympan.print("serviceSdReading: isFileOpen() = ");
    myTympan.println(audioSDPlayer.isFileOpen());
    myTympan.print("serviceSdReading: isPlaying() = ");
    myTympan.println(audioSDPlayer.isPlaying());
          
      delay(10);
      if (count > 100) {
        myTympan.println("Reading from SD audio file is complete.  Stopping SD recording.");
        audioSDPlayer.stop(); 

    myTympan.print("serviceSdReading: isFileOpen() = ");
    myTympan.println(audioSDPlayer.isFileOpen());
    myTympan.print("serviceSdReading: isPlaying() = ");
    myTympan.println(audioSDPlayer.isPlaying());
        
        //audioSDWriter.stopRecording();
      }
    }
  }
}
