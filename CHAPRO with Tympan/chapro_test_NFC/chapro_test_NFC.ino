
#include <Arduino.h>
#include "implementStdio.h"
#include <Tympan_Library.h>
#include <chapro.h>
#include "AudioEffectNFC.h"

//Create CHA structures
#include "test_nfc.h"

//set the sample rate and block size
const float sample_rate_Hz = (int)srate;  //must be one of the values in the table in AudioOutputI2S_F32.h
const int audio_block_samples = chunk;    //must be <= 128 
AudioSettings_F32 audio_settings(sample_rate_Hz, audio_block_samples);

// Create the Tympan
Tympan myTympan(TympanRev::E, audio_settings);

//create audio objects
//AudioSDPlayer_F32 audioSDPlayer(audio_settings);
AudioInputI2S_F32   audio_in(audio_settings);
AudioEffectNFC      BTNRH_alg1(audio_settings);
//AudioEffectNFC    BTNRH_alg2(audio_settings);
AudioOutputI2S_F32  audio_out(audio_settings);

//create audio connections
//AudioConnection_F32 patchCord1(audioSDPlayer, 0, effect1, 0);
//AudioConnection_F32 patchCord2(audioSDPlayer, 1, effect2, 0);
//AudioConnection_F32 patchCord1(i2s_in, 0, effect2, 0);
//AudioConnection_F32 patchCord2(audioSDPlayer, 1, effect2, 0);
//AudioConnection_F32 patchCord4(effect2, 0, audioOutput, 1);

AudioConnection_F32 patchCord1(audio_in, 0, BTNRH_alg1, 0);  //left input
AudioConnection_F32 patchCord3(BTNRH_alg1, 0, audio_out, 0); //left output
AudioConnection_F32 patchCord4(BTNRH_alg1, 0, audio_out, 1); //right output same as left


unsigned long end_millis = 0;
String filename = "CAT.WAV"; // filenames are always uppercase 8.3 format

void setup()
{  
    myTympan.beginBothSerial();
    if (Serial) Serial.print(CrashReport);
    delay(1000);
    myTympan.println("CHAPRO for Tympan: Test NFC: setup():...");
    myTympan.println("  Sample Rate (Hz): " + String(audio_settings.sample_rate_Hz));
    myTympan.println("  Audio Block Size (samples): " + String(audio_settings.audio_block_samples));

    //test printf()
    while (!debugOut && millis() < 4000) {}  // Wait for Serial initialization
    printf("Testing printf\n");

    // Audio connections require memory to work.
    AudioMemory_F32(50, audio_settings);

    // /////////////////////////////////////////////  do any setup of the algorithms

    // Note that io and cp are both created in test_nfc.h in the global space
    
    Serial.println("setup: BTNRH configure...");
    configure(&io);               //in test_nfc.h
    
    Serial.println("setup: BTNRH prepare...");
    prepare(&io, cp);             //in test_nfc.h
    
    Serial.println("setup: BTNRH_alg1: setup()...");
    BTNRH_alg1.setup(cp);        //in AudioEffectNFC.h

    Serial.println("setup: BTNRH_alg1: enabling...");
    BTNRH_alg1.setEnabled(true);  //see AudioEffectNFC.h.  This could be done later in setup()
    
    // //////////////////////////////////////////// End setup of the algorithms

    // Start the Tympan
    Serial.println("setup: Tympan enable...");
    myTympan.enable();

    // setup DC-blocking highpass filter running in the ADC hardware itself
    float cutoff_Hz = 40.0;  //set the default cutoff frequency for the highpass filter
    myTympan.setHPFonADC(true,cutoff_Hz,audio_settings.sample_rate_Hz); //set to false to disble

    // Choose the desired analog input
    myTympan.inputSelect(TYMPAN_INPUT_ON_BOARD_MIC);     // use the on board microphones
    //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_MIC);    // use the microphone jack - defaults to mic bias 2.5V
    //myTympan.inputSelect(TYMPAN_INPUT_JACK_AS_LINEIN); // use the microphone jack - defaults to mic bias OFF
    
    // Set the desired volume levels
    myTympan.volume_dB(0);                   // headphone amplifier.  -63.6 to +24 dB in 0.5dB steps.
    float input_gain_dB = 15.0;              // choose amount of analog input gain
    myTympan.setInputGain_dB(input_gain_dB); // set input volume, 0-47.5dB in 0.5dB setps

    // prepare SD player
    //audioSDPlayer.begin();

    // Finish setup
    Serial.println("Setup complete.");


}

void loop()
{

  
//    //service the audio player
//    if (!audioSDPlayer.isPlaying())
//    { //wait until previous play is done
//        AudioProcessorUsageMaxReset();
//        //start playing audio
//        myTympan.print("Starting audio player: ");
//        myTympan.println(filename);
//        audioSDPlayer.play(filename);
//    }

    //do other things here, if desired...like, maybe check the volume knob?

    //periodically print the CPU and Memory Usage
    myTympan.printCPUandMemory(millis(),1000); //print every 3000 msec

    //stall, just to be nice?
    delay(5);

}
