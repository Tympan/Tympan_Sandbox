
/*
   Chip Audette, OpenAudio, Apr 2019

   MIT License.  Use at your own risk.
*/

#ifndef _AudioSDWriter_h
#define _AudioSDWriter_h

#include "SDWriter.h"
#include "AudioSettings_F32.h"
#include "AudioStream_F32.h"

//variables to control printing of warnings and timings and whatnot
#define PRINT_FULL_SD_TIMING 0    //set to 1 to print timing information of *every* write operation.  Great for logging to file.  Bad for real-time human reading.

//AudioSDWriter: A class to write data from audio blocks as part of the 
//   Teensy/Tympan audio processing paradigm.  The AudioSDWriter class is 
//   just a virtual Base class.  Use AudioSDWriter_F32 further down.
class AudioSDWriter {
  public:
    AudioSDWriter(void) {};
    enum class STATE { UNPREPARED = -1, STOPPED, RECORDING };
    STATE getState(void) {
      return current_SD_state;
    };
    enum class WriteDataType { INT16 }; //in the future, maybe add FLOAT32
    virtual int setNumWriteChannels(int n) {
      return numWriteChannels = max(1, min(n, 2));  //can be 1 or 2
    }
    virtual int getNumWriteChannels(void) {
      return numWriteChannels;
    }

    virtual void prepareSDforRecording(void) = 0;
    virtual int startRecording(void) = 0;
    virtual int startRecording(char *) = 0;
    virtual void stopRecording(void) = 0;

  protected:
    STATE current_SD_state = STATE::UNPREPARED;
    WriteDataType writeDataType = WriteDataType::INT16;
    int recording_count = 0;
    int numWriteChannels = 2;
};

//AudioSDWriter_F32: A class to write data from audio blocks as part
//   of the Teensy/Tympan audio processing paradigm.  For this class, the
//   audio is given as float32 and written as int16
class AudioSDWriter_F32 : public AudioSDWriter, public AudioStream_F32 {
  //GUI: inputs:2, outputs:0 //this line used for automatic generation of GUI node
  public:
    AudioSDWriter_F32(void) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup();
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup(); 
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings, Print* _serial_ptr) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup(_serial_ptr);
      setSampleRate_Hz(settings.sample_rate_Hz);
    }
    AudioSDWriter_F32(const AudioSettings_F32 &settings, Print* _serial_ptr, const int _writeSizeBytes) :
      AudioSDWriter(),
      AudioStream_F32(2, inputQueueArray)
    { 
      setup(_serial_ptr, _writeSizeBytes); 
      setSampleRate_Hz(settings.sample_rate_Hz); 
    }
    ~AudioSDWriter_F32(void) {
      stopRecording();
      delete buffSDWriter;
    }

    void setup(void) {
      setWriteDataType(WriteDataType::INT16, &Serial, DEFAULT_SDWRITE_BYTES);
    }
    void setup(Print *_serial_ptr) {
      setSerial(_serial_ptr);
      setWriteDataType(WriteDataType::INT16, _serial_ptr, DEFAULT_SDWRITE_BYTES);
    }
    void setup(Print *_serial_ptr, const int _writeSizeBytes) {
      setSerial(_serial_ptr);
      setWriteDataType(WriteDataType::INT16, _serial_ptr, _writeSizeBytes);
    }

    void setSerial(Print *_serial_ptr) {  serial_ptr = _serial_ptr;  }
    void setWriteDataType(WriteDataType type) {
      Print *serial_ptr = &Serial1;
      int write_nbytes = DEFAULT_SDWRITE_BYTES;

      //get info from previous objects
      if (buffSDWriter) {
        serial_ptr = buffSDWriter->getSerial();
        write_nbytes = buffSDWriter->getWriteSizeBytes();
      }

      //make the full method call
      setWriteDataType(type, serial_ptr, write_nbytes);
    }
    void setWriteDataType(WriteDataType type, Print* serial_ptr, const int writeSizeBytes) {
      stopRecording();
      writeDataType = type;
      if (!buffSDWriter) {
        buffSDWriter = new BufferedSDWriter(serial_ptr, writeSizeBytes);
        //allocateBuffer(); //use default buffer size...or comment this out and let BufferedSDWrite create it last-minute
      }
    }
    void setWriteSizeBytes(const int n) {  //512Bytes is most efficient for SD
      if (buffSDWriter) buffSDWriter->setWriteSizeBytes(n);
    }
    int getWriteSizeBytes(void) {  //512Bytes is most efficient for SD
      if (buffSDWriter) return buffSDWriter->getWriteSizeBytes();
      return 0;
    }
    int setNumWriteChannels(int n) {
      if (buffSDWriter) return buffSDWriter->setNChanWAV(n);
      return n;
    }
    float setSampleRate_Hz(float fs_Hz) {
      if (buffSDWriter) return buffSDWriter->setSampleRateWAV(fs_Hz);
      return fs_Hz;
    }

    //tell BufferedSDWriter to create it's buffer right now, using the default
    //size or using the size given as an argument.  If the buffer has already
    //been created, it should delete the buffer before creating the new one
    int allocateBuffer(void) {
      if (!buffSDWriter) return buffSDWriter->allocateBuffer(); //use default buffer size
      return 0;
    }
    int allocateBuffer(const int nBytes) {
       if (!buffSDWriter) return buffSDWriter->allocateBuffer(nBytes);
      return 0;     
    }

    
    void prepareSDforRecording(void) {
      if (current_SD_state == STATE::UNPREPARED) {
        if (buffSDWriter) {
          buffSDWriter->init(); //part of SDWriter, which is the base for BufferedSDWriter_I16
          if (PRINT_FULL_SD_TIMING) buffSDWriter->setPrintElapsedWriteTime(true); //for debugging.  make sure time is less than (audio_block_samples/sample_rate_Hz * 1e6) = 2900 usec for 128 samples at 44.1 kHz
        }
        current_SD_state = STATE::STOPPED;
      }
    }

    int startRecording(void) {
      int return_val = 0;

      //check to see if the SD has been initialized
      if (current_SD_state == STATE::UNPREPARED) prepareSDforRecording();

      //check to see if SD is ready
      if (current_SD_state == STATE::STOPPED) {
        recording_count++;
        if (recording_count < 1000) {
          //make file name
          char fname[] = "AUDIOxxx.WAV";
          int hundreds = recording_count / 100;
          fname[5] = hundreds + '0';  //stupid way to convert the number to a character
          int tens = (recording_count - (hundreds*100)) / 10;  //truncates
          fname[6] = tens + '0';  //stupid way to convert the number to a character
          int ones = recording_count - (tens * 10) - (hundreds*100);
          fname[7] = ones + '0';  //stupid way to convert the number to a character

          //open the file
          return_val = startRecording(fname);
        } else {
          if (serial_ptr) serial_ptr->println("AudioSDWriter: start: Cannot do more than 999 files.");
        }
      } else {
        if (serial_ptr) serial_ptr->println("AudioSDWriter: start: not in correct state to start.");
        return_val = -1;
      }
      return return_val;
    }
    
    int startRecording(char* fname) {
      int return_val = 0;
      if (current_SD_state == STATE::STOPPED) {
        //try to open the file on the SD card
        if (openAsWAV(fname)) { //returns TRUE if the file opened successfully
          if (serial_ptr) {
            serial_ptr->print("AudioSDWriter: Opened ");
            serial_ptr->println(fname);
          }
          
          //start the queues.  Then, in the serviceSD, the fact that the queues
          //are getting full will begin the writing
          buffSDWriter->resetBuffer();
          current_SD_state = STATE::RECORDING;
          setStartTimeMillis();
          
        } else {
          if (serial_ptr) {
            serial_ptr->print("AudioSDWriter: start: Failed to open ");
            serial_ptr->println(fname);
          }
          return_val = -1;
        }
      } else {
        if (serial_ptr) serial_ptr->println("AudioSDWriter: start: not in correct state to start.");
        return_val = -1;
      }
      return return_val;
    }

    void stopRecording(void) {
      if (current_SD_state == STATE::RECORDING) {
        //if (serial_ptr) serial_ptr->println("stopRecording: Closing SD File...");

        //close the file
        close(); current_SD_state = STATE::STOPPED;

        //clear the buffer
        if (buffSDWriter) buffSDWriter->resetBuffer();
      }
    }

    //update is called by the Audio processing ISR.  This update function should
    //only service the recording queues so as to buffer the audio data.
    //The acutal SD writing should occur in the loop() as invoked by a service routine
    void update(void) {
      audio_block_f32_t *audio_blocks[4];

      //get the audio
      for (int Ichan=0; Ichan < numWriteChannels; Ichan++) audio_blocks[Ichan] = receiveReadOnly_f32(Ichan);

      //copy the audio to the bug write buffer
      if (current_SD_state == STATE::RECORDING) {
        //if (buffSDWriter) buffSDWriter->copyToWriteBuffer(audio_blocks,numWriteChannels);
        copyAudioToWriteBuffer(audio_blocks, numWriteChannels);
      }

      //release the audio blocks
      for (int Ichan=0; Ichan < numWriteChannels; Ichan++) {
        if (audio_blocks[Ichan]) AudioStream_F32::release(audio_blocks[Ichan]);
      }   
    }

    virtual void copyAudioToWriteBuffer(audio_block_f32_t *audio_blocks[], const int numChan) {
      static unsigned long last_audio_block_id[4];
      if (numChan == 0) return;
      
      //do any of the given audio blocks actually contain data
      int any_data = 0, nsamps = 0;
      for (int Ichan = 0; Ichan < numChan; Ichan++) {
        if (audio_blocks[Ichan]) { //looking for anything NOT null
          any_data++;  //this audio_block[Ichan] is not null, so count it
          nsamps = audio_blocks[Ichan]->length; //how long is it?
          //Serial.print("SDWriter: copyToWriteBuffer: good "); Serial.println(Ichan);
        }
      }
      if (any_data == 0) return;  //if there's no data, return;
      if (any_data < numChan) { // do we have all the channels?  If not, send error?
        Serial.print("AudioSDWriter: copyToWriteBuffer: only got "); Serial.print(any_data);
        Serial.println(" of ");  Serial.print(numChan);  Serial.println(" channels.");
      }

      //check to see if there have been any jumps in the data counters
      for (int Ichan = 0; Ichan < numChan; Ichan++) {
        if (audio_blocks[Ichan] != NULL) {
          if (((audio_blocks[Ichan]->id - last_audio_block_id[Ichan]) != 1) && (last_audio_block_id[Ichan] != 0)) {
            Serial.print("AudioSDWriter: chan "); Serial.print(Ichan);
            Serial.print(", data skip? This ID = "); Serial.print(audio_blocks[Ichan]->id);
            Serial.print(", Previous ID = "); Serial.println(last_audio_block_id[Ichan]);
          }
          last_audio_block_id[Ichan] = audio_blocks[Ichan]->id;
        }
      }

      //data looks good, prep for handoff
      float32_t *ptr_audio[numChan] = {}; //the braces cause it to init all to zero (null)
      for (int Ichan = 0; Ichan < numChan; Ichan++) { 
        if (audio_blocks[Ichan]) ptr_audio[Ichan] = audio_blocks[Ichan]->data;
      }

      //now push it into the buffer via the base class BufferedSDWriter
      if (buffSDWriter) buffSDWriter->copyToWriteBuffer(ptr_audio,nsamps,numChan);
    } 
    

    bool isFileOpen(void) {
      if (buffSDWriter) return buffSDWriter->isFileOpen();
      return false;
    }

    //this is what pulls data from the queues and sends to SD for writing.
    //should be invoked from loop(), not from an ISR
    int serviceSD(void) {
      if (buffSDWriter) return buffSDWriter->writeBufferedData();
      return false;
    }

  unsigned long getStartTimeMillis(void) { return t_start_millis; };
  unsigned long setStartTimeMillis(void) { return t_start_millis = millis(); };

  protected:
    audio_block_f32_t *inputQueueArray[2]; //two input channels
    BufferedSDWriter *buffSDWriter = 0;
    Print *serial_ptr = &Serial;
    unsigned long t_start_millis = 0;

    bool openAsWAV(char *fname) {
      if (buffSDWriter) return buffSDWriter->openAsWAV(fname);
      return false;
    }
    bool open(char *fname) {
      if (buffSDWriter) return buffSDWriter->open(fname);
      return false;
    }

    int close(void) {
      if (buffSDWriter) return buffSDWriter->close();
      return 0;
    }
};

#endif

