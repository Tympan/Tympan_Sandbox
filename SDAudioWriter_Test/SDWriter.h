

/*
   Chip Audette, OpenAudio, Apr 2019

   MIT License.  Use at your own risk.
*/


#ifndef _SDWriter_h
#define _SDWriter_h

#include <SdFat_Gre.h>       //originally from https://github.com/greiman/SdFat  but class names have been modified to prevent collisions with Teensy Audio/SD libraries
#include <Print.h>

//some constants for the AudioSDWriter
const int DEFAULT_SDWRITE_BYTES = 512;  //minmum of 512 bytes is most efficient for SD.  Only used for binary writes
//const uint64_t PRE_ALLOCATE_SIZE = 40ULL << 20;// Preallocate 40MB file.

//SDWriter:  This is a class to make it easier to write blocks of bytes, ints, or floats
//  to the SD card.  It will write blocks of data of whatever the size, even if it is not
//  most efficient for the SD card.
//
//  To handle the interleaving of multiple channels and to handle conversion to the
//  desired write type (float32 -> int16) and to handle buffering so that the optimal
//  number of bytes are written at once, use one of the derived classes such as
//  BufferedSDWriter_I16 or BufferedSDWriter_F32
class SDWriter : public Print
{
  public:
    SDWriter() {};
    SDWriter(Print* _serial_ptr) {
      setSerial(_serial_ptr);
    };
    virtual ~SDWriter() {
      if (isFileOpen()) {
        close();
      }
    }

    void setup(void) {
      init();
    }
    virtual void init() {
      if (!sd.begin()) sd.errorHalt(serial_ptr, "SDWriter: begin failed");
    }

    bool openAsWAV(char *fname) {
      bool returnVal = open(fname);
      if (isFileOpen()) { //true if file is open
        flag__fileIsWAV = true;
        file.write(wavHeaderInt16(0),WAVheader_bytes); //initialize assuming zero length
      }
      return returnVal;
    }

    bool open(char *fname) {
      if (sd.exists(fname)) {  //maybe this isn't necessary when using the O_TRUNC flag below
        // The SD library writes new data to the end of the
        // file, so to start a new recording, the old file
        // must be deleted before new data is written.
        sd.remove(fname);
      }

      file.open(fname, O_RDWR | O_CREAT | O_TRUNC);
      //file.createContiguous(fname, PRE_ALLOCATE_SIZE); //alternative to the line above
      
      return isFileOpen();
    }


    int close(void) {
      //file.truncate();
      if (flag__fileIsWAV) {
        //re-write the header with the correct file size
        uint32_t fileSize = file.fileSize();//SdFat_Gre_FatLib version of size();
        file.seekSet(0); //SdFat_Gre_FatLib version of seek();
        file.write(wavHeaderInt16(fileSize),WAVheader_bytes); //write header with correct length
        file.seekSet(fileSize);
      }
      file.close();
      flag__fileIsWAV = false;
      return 0;
    }

    bool isFileOpen(void) {
      if (file.isOpen()) {
        return true;
      } else {
        return false;
      }
    }

    //This "write" is for compatibility with the Print interface.  Writing one
    //byte at a time is EXTREMELY inefficient and shouldn't be done
    virtual size_t write(uint8_t foo)  {
      size_t return_val = 0;
      if (file.isOpen()) {

        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime) {
          usec = 0;
        }
        file.write((byte *) (&foo), 1); //write one value
        return_val = 1;

        //write elapsed time only to USB serial (because only that is fast enough)
        if (flagPrintElapsedWriteTime) {
          Serial.print("SD, us=");
          Serial.println(usec);
        }
      }
      return return_val;
    }

    //write Byte buffer...the lowest-level call upon which the others are built.
    //writing 512 is most efficient (ie 256 int16 or 128 float32
    virtual size_t write(const uint8_t *buff, int nbytes) {
      size_t return_val = 0;
      if (file.isOpen()) {

        // write all audio bytes (512 bytes is most efficient)
        if (flagPrintElapsedWriteTime) {
          usec = 0;
        }
        file.write((byte *)buff, nbytes);
        return_val = nbytes;
        nBlocksWritten++;

        //write elapsed time only to USB serial (because only that is fast enough)
        if (flagPrintElapsedWriteTime) {
          Serial.print("SD, us=");
          Serial.println(usec);
        }
      }
      return return_val;
    }

    //write Int16 buffer.
    virtual size_t write(int16_t *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    //write float32 buffer
    virtual size_t write(float32_t *buff, int nsamps) {
      return write((const uint8_t *)buff, nsamps * sizeof(buff[0]));
    }

    void enablePrintElapsedWriteTime(void) {
      flagPrintElapsedWriteTime = true;
    }
    void disablePrintElapseWriteTime(void) {
      flagPrintElapsedWriteTime = false;
    }
    unsigned long getNBlocksWritten(void) {
      return nBlocksWritten;
    }
    void resetNBlocksWritten(void) {
      nBlocksWritten = 0;
    }
    virtual void setSerial(Print *ptr) {
      serial_ptr = ptr;
    }
    virtual Print* getSerial(void) {
      return serial_ptr;
    }
    
    int setNChanWAV(int nchan) { return WAV_nchan = nchan; };
    float setSampleRateWAV(float sampleRate_Hz) { return WAV_sampleRate_Hz = sampleRate_Hz; }
    void setParamsWAV(float sampleRate_Hz,int nchan) { setSampleRateWAV(sampleRate_Hz); setNChanWAV(nchan); }
   
    //modified from Walter at https://github.com/WMXZ-EU/microSoundRecorder/blob/master/audio_logger_if.h
    char * wavHeaderInt16(const uint32_t fsize) { return wavHeaderInt16(WAV_sampleRate_Hz, WAV_nchan, fsize);  }
    char* wavHeaderInt16(const float32_t sampleRate_Hz, const int nchan, const uint32_t fileSize) {
      //const int fileSize = bytesWritten+44;
      
      int fsamp = (int) sampleRate_Hz;
      int nbits=16;
      int nbytes=nbits/8;
      int nsamp=(fileSize-WAVheader_bytes)/(nbytes*nchan);
      
      static char wheader[48]; // 44 for wav
    
      strcpy(wheader,"RIFF");
      strcpy(wheader+8,"WAVE");
      strcpy(wheader+12,"fmt ");
      strcpy(wheader+36,"data");
      *(int32_t*)(wheader+16)= 16;// chunk_size
      *(int16_t*)(wheader+20)= 1; // PCM 
      *(int16_t*)(wheader+22)=nchan;// numChannels 
      *(int32_t*)(wheader+24)= fsamp; // sample rate 
      *(int32_t*)(wheader+28)= fsamp*nbytes; // byte rate
      *(int16_t*)(wheader+32)=nchan*nbytes; // block align
      *(int16_t*)(wheader+34)=nbits; // bits per sample 
      *(int32_t*)(wheader+40)=nsamp*nchan*nbytes; 
      *(int32_t*)(wheader+4)=36+nsamp*nchan*nbytes; 
    
      return wheader;
    }


  protected:
    //SdFatSdio sd; //slower
    SdFatSdioEX sd; //faster
    SdFile_Gre file;
    boolean flagPrintElapsedWriteTime = false;
    elapsedMicros usec;
    unsigned long nBlocksWritten = 0;
    Print* serial_ptr = &Serial;
    //WriteDataType writeDataType = WriteDataType::INT16;
    bool flag__fileIsWAV = false;
    const int WAVheader_bytes = 44;
    float WAV_sampleRate_Hz = 44100.0;
    int WAV_nchan = 2;

};

//BufferedSDWriter_I16:  This is a drived class from SDWriter.  This class assumes that
//  you want to write Int16 data to the SD card.  You may give this class data that is
//  either Int16 or Float32.  This class will also handle interleaving of two input
//  channels.  This class will also buffer the data until the optimal (or desired) number
//  of samples have been accumulated, which makes the SD writing more efficient.
class BufferedSDWriter_I16 : public SDWriter
{
  public:
    BufferedSDWriter_I16() : SDWriter() {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES);
    };
    BufferedSDWriter_I16(Print* _serial_ptr) : SDWriter(_serial_ptr) {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES );
    };
    BufferedSDWriter_I16(Print* _serial_ptr, const int _writeSizeBytes) : SDWriter(_serial_ptr) {
      setWriteSizeBytes(_writeSizeBytes);
    };
    ~BufferedSDWriter_I16(void) {
      delete write_buffer;
    }

    void setWriteSizeBytes(const int _writeSizeBytes) {
      setWriteSizeSamples(_writeSizeBytes / nBytesPerSample);
    }
    void setWriteSizeSamples(const int _writeSizeSamples) {
      //ensure even number greater than 0
      writeSizeSamples = max(2, 2 * int(_writeSizeSamples / 2));

      //create write buffer
      if (write_buffer != 0) {
        delete write_buffer;  //delete the old buffer
      }
      //write_buf fer = new int16_t[writeSizeSamples];
      write_buffer = new int16_t[bufferLengthSamples];

      //reset the buffer index
      bufferWriteInd = 0;
    }
    int getWriteSizeBytes(void) {
      return (getWriteSizeSamples() * nBytesPerSample);
    }
    int getWriteSizeSamples(void) {
      return writeSizeSamples;
    }

    virtual void copyToWriteBuffer(audio_block_f32_t *audio_blocks[], const int numChan) {
      static unsigned long last_audio_block_id = 0;
      if (!write_buffer) return;
      if (numChan == 0) return;

      //is there any good data?
      int any_data = 0;
      int nsamps = 0;
      for (int Ichan = 0; Ichan < numChan; Ichan++) {
        if (audio_blocks[Ichan]) { //looking for anything NOT null
          any_data++; 
          nsamps = audio_blocks[Ichan]->length;
          //Serial.print("SDWriter: copyToWriteBuffer: good "); Serial.println(Ichan);
        }
      }
      if (any_data == 0) return;  //if there's no data, return;
      if (any_data < numChan) {
        Serial.print("SDWriter: copyToWriteBuffer: only got ");
        Serial.print(any_data);
        Serial.println(" of ");
        Serial.print(numChan);
        Serial.println(" channels.");
      }

      
      if (((audio_blocks[0]->id - last_audio_block_id) != 1) && (last_audio_block_id != 0)) {
        Serial.print("SD Writer: data skip? This ID = ");
        Serial.print(audio_blocks[0]->id);
        Serial.print(", Previous ID = ");
        Serial.println(last_audio_block_id);
      }
      last_audio_block_id = audio_blocks[0]->id;
      


      //how much data will we write
      int estFinalWriteInd = bufferWriteInd + (numChan*nsamps);

      //will we pass by the read index?
      bool flag_moveReadIndexToEndOfWrite = false;
      if ((bufferWriteInd < bufferReadInd) && (estFinalWriteInd > bufferReadInd)) {
        Serial.println("BufferedSDWriter_I16: WARNING: writing past the read index.");
        flag_moveReadIndexToEndOfWrite = true;
      }

      //is there room to put the data into the buffer or will we hit the end
      if ( estFinalWriteInd >= bufferLengthSamples) { //is there room?
        //no there is not room
        bufferEndInd = bufferWriteInd; //save the end point of the written data
        bufferWriteInd = 0;  //reset

        //recheck to see if we're going to pass by the read buffer index
        estFinalWriteInd = bufferWriteInd + (numChan*nsamps);
        if ((bufferWriteInd < bufferReadInd) && (estFinalWriteInd > bufferReadInd)) {
          Serial.println("BufferedSDWriter_I16: WARNING: writing past the read index.");
          flag_moveReadIndexToEndOfWrite = true;
        }
      }

      //now interleave the data into the buffer
      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
        for (int Ichan = 0; Ichan < numChan; Ichan++) {
          if (audio_blocks[Ichan]) { //not null
            //convert the F32 to Int16 and interleave
            write_buffer[bufferWriteInd++] = (int16_t)(audio_blocks[Ichan]->data[Isamp] * 32767.0);
          } else {
            write_buffer[bufferWriteInd++] = 0;
          }
        }
      }

      //handle the case where we just wrote past the read index.  Push the read index ahead.
      if (flag_moveReadIndexToEndOfWrite) bufferReadInd = bufferWriteInd;
    }

    //write buffered data if enough has accumulated
    virtual int writeBufferedData(void) {
      if (!write_buffer) return -1;
      int return_val = 0;
      
      //if the write pointer has wrapped around, write the data
      if (bufferWriteInd < bufferReadInd) { //if the buffer has wrapped around
        //Serial.println("BuffSDI16: writing to end of buffer");
        return_val += write((byte *)(write_buffer+bufferReadInd), 
            (bufferEndInd-bufferReadInd)*sizeof(write_buffer[0]));
        bufferReadInd = 0;  //jump back to beginning of buffer
      }

      //do we have enough data to write again?  If so, write the whole thing
      if ((bufferWriteInd - bufferReadInd) >= writeSizeSamples) {   
          //Serial.println("BuffSDI16: writing buffered data");
          //return_val = write((byte *)(write_buffer+buffer, writeSizeSamples * sizeof(write_buffer[0]));
          return_val += write((byte *)(write_buffer+bufferReadInd), 
            (bufferWriteInd-bufferReadInd)*sizeof(write_buffer[0]));
          bufferReadInd = bufferWriteInd;  //increment to end of where it wrote
      }
      return return_val;
    }

    virtual int interleaveAndWrite(int16_t *chan1, int16_t *chan2, int nsamps) {
      //Serial.println("BuffSDI16: interleaveAndWrite given I16...");
      Serial.println("BuffSDI16: interleave and write (I16 inputs).  UPDATE ME!!!");
      if (!write_buffer) return -1;
      int return_val = 0;

      //interleave the data and write whenever the write buffer is full
      //Serial.println("BuffSDI16: buffering given I16...");
      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
        write_buffer[bufferWriteInd++] = chan1[Isamp];
        write_buffer[bufferWriteInd++] = chan2[Isamp];

        //do we have enough data to write our block to SD?
        if (bufferWriteInd >= writeSizeSamples) {
          //Serial.println("BuffSDI16: writing given I16...");
          return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
          bufferWriteInd = 0;  //jump back to beginning of buffer
        }
      }
      return return_val;
    }

    virtual int interleaveAndWrite(float32_t *chan1, float32_t *chan2, int nsamps) {
      //Serial.println("BuffSDI16: interleaveAndWrite given F32...");
      if (!write_buffer) return -1;
      int return_val = 0;

      //interleave the data into the buffer
      //Serial.println("BuffSDI16: buffering given F32...");
      if ( (bufferWriteInd+nsamps) >= bufferLengthSamples) { //is there room?
        bufferEndInd = bufferWriteInd; //save the end point of the written data
        bufferWriteInd = 0;  //reset
      }
      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
        //convert the F32 to Int16 and interleave
        write_buffer[bufferWriteInd++] = (int16_t)(chan1[Isamp] * 32767.0);
        write_buffer[bufferWriteInd++] = (int16_t)(chan2[Isamp] * 32767.0);
      }

      //if the write pointer has wrapped around, write the data
      if (bufferWriteInd < bufferReadInd) { //if the buffer has wrapped around
        //Serial.println("BuffSDI16: writing given F32...");
        return_val = write((byte *)(write_buffer+bufferReadInd), 
            (bufferEndInd-bufferReadInd)*sizeof(write_buffer[0]));
        bufferReadInd = 0;  //jump back to beginning of buffer
      }

      //do we have enough data to write again?  If so, write the whole thing
      if ((bufferWriteInd - bufferReadInd) >= writeSizeSamples) {   
          //Serial.println("BuffSDI16: writing given F32...");
          //return_val = write((byte *)(write_buffer+buffer, writeSizeSamples * sizeof(write_buffer[0]));
          return_val = write((byte *)(write_buffer+bufferReadInd), 
            (bufferWriteInd-bufferReadInd)*sizeof(write_buffer[0]));
          bufferReadInd = bufferWriteInd;  //increment to end of where it wrote
      }
      return return_val;
    }

    //write one channel of int16 as int16
    virtual int writeOneChannel(int16_t *chan1, int nsamps) {
      if (write_buffer == 0) return -1;
      int return_val = 0;

      if (nsamps == writeSizeSamples) {
        //special case where everything is the right size...it'll be fast!
        return write((byte *)chan1, writeSizeSamples * sizeof(chan1[0]));
      } else {
        //do the buffering and write when the buffer is full

        for (int Isamp = 0; Isamp < nsamps; Isamp++) {
          //convert the F32 to Int16 and interleave
          write_buffer[bufferWriteInd++] = chan1[Isamp];

          //do we have enough data to write our block to SD?
          if (bufferWriteInd >= writeSizeSamples) {
            return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
            bufferWriteInd = 0;  //jump back to beginning of buffer
          }
        }
        return return_val;
      }
      return return_val;
    }

    //write one channel of float32 as int16
    virtual int writeOneChannel(float32_t *chan1, int nsamps) {
      if (write_buffer == 0) return -1;
      int return_val = 0;

      //interleave the data and write whenever the write buffer is full
      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
        //convert the F32 to Int16 and interleave
        write_buffer[bufferWriteInd++] = (int16_t)(chan1[Isamp] * 32767.0);

        //do we have enough data to write our block to SD?
        if (bufferWriteInd >= writeSizeSamples) {
          return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
          bufferWriteInd = 0;  //jump back to beginning of buffer
        }
      }
      return return_val;
    }

  protected:
    int writeSizeSamples = 0;
    int16_t* write_buffer = 0;
    int32_t bufferWriteInd = 0;
    int32_t bufferReadInd = 0;
    const int nBytesPerSample = 2;
    #define bufferLengthBytes 150000
    int32_t bufferLengthSamples = bufferLengthBytes / nBytesPerSample;
    int32_t bufferEndInd = bufferLengthBytes / nBytesPerSample;
    
};

//BufferedSDWriter_F32:  This is a drived class from SDWriter.  This class assumes that
//  you want to write float32 data to the SD card.  This class will handle interleaving
//  of two input channels.  This class will also buffer the data until the optimal (or
//  desired) number of samples have been accumulated, which makes the SD writing more efficient.
class BufferedSDWriter_F32 : public SDWriter
{
  public:
    BufferedSDWriter_F32() : SDWriter() {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES);
    };
    BufferedSDWriter_F32(Print* _serial_ptr) : SDWriter(_serial_ptr) {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES);
    };
    BufferedSDWriter_F32(Print* _serial_ptr, const int _writeSizeBytes) : SDWriter(_serial_ptr) {
      setWriteSizeBytes(DEFAULT_SDWRITE_BYTES);
    };
    ~BufferedSDWriter_F32(void) {
      delete write_buffer;
    }

    void setWriteSizeBytes(const int _writeSizeBytes) {
      setWriteSizeSamples( _writeSizeBytes * nBytesPerSample);
    }
    void setWriteSizeSamples(const int _writeSizeSamples) {
      //ensure even number greater than 0
      writeSizeSamples = max(2, 2 * int(_writeSizeSamples / nBytesPerSample) );

      //create write buffer
      if (write_buffer != 0) {
        delete write_buffer;  //delete the old buffer
      }
      write_buffer = new float32_t[writeSizeSamples];

      //reset the buffer index
      bufferWriteInd = 0;
    }
    int getWriteSizeBytes(void) {
      return (getWriteSizeSamples() * nBytesPerSample);
    }
    int getWriteSizeSamples(void) {
      return writeSizeSamples;
    }

    //
    virtual void copyToWriteBuffer(audio_block_f32_t *audio_blocks[], const int numChan) {
      if (!write_buffer) return;
      Serial.print("BufferedSDWriter_F32: copyToBuffer: *** WRITE CODE HERE***");
    }

    virtual int writeBufferedData(void) {
      if (!write_buffer) return -1;
      Serial.print("BufferedSDWriter_F32: writeBufferedData: *** WRITE CODE HERE***");
      return -1;
    }

    //write two channels of float32 as float32
    virtual int interleaveAndWrite(float32_t *chan1, float32_t *chan2, int nsamps) {
      if (write_buffer == 0) return -1;
      int return_val = 0;

      //interleave the data and write whenever the write buffer is full
      for (int Isamp = 0; Isamp < nsamps; Isamp++) {
        write_buffer[bufferWriteInd++] = chan1[Isamp];
        write_buffer[bufferWriteInd++] = chan2[Isamp];

        //do we have enough data to write our block to SD?
        if (bufferWriteInd >= writeSizeSamples) {
          return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
          bufferWriteInd = 0;  //jump back to beginning of buffer
        }
      }
      return return_val;
    }

    //write one channel of float32 as float32
    virtual int writeOneChannel(float32_t *chan1, int nsamps) {
      if (write_buffer == 0) return -1;
      int return_val = 0;

      if (nsamps == writeSizeSamples) {
        //special case where everything is the right size...it'll be fast!
        return write((byte *)chan1, writeSizeSamples * sizeof(chan1[0]));
      } else {
        //do the buffering and write when the buffer is full

        for (int Isamp = 0; Isamp < nsamps; Isamp++) {
          //convert the F32 to Int16 and interleave
          write_buffer[bufferWriteInd++] = chan1[Isamp];

          //do we have enough data to write our block to SD?
          if (bufferWriteInd >= writeSizeSamples) {
            return_val = write((byte *)write_buffer, writeSizeSamples * sizeof(write_buffer[0]));
            bufferWriteInd = 0;  //jump back to beginning of buffer
          }
        }
        return return_val;
      }
      return return_val;
    }

  protected:
    int writeSizeSamples = 0;
    float32_t *write_buffer = 0;
    int bufferWriteInd = 0;
    const int nBytesPerSample = 4;
};

#endif

