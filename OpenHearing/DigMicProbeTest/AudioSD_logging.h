

#ifndef _AudioSD_logging_h
#define _AudioSD_logging_h

#include <AudioSDPlayer_F32.h>
#include <AudioSDWriter_F32.h>

class LogEntry {
  public:
    uint32_t time_msec = 0UL;
    uint32_t buff_bytes_before = 0;
    uint32_t buff_bytes_after = 0;
    uint16_t dur_msec = 0;
    uint16_t bytes_transferred = 0;
 
    String toString(void) {
      return String(time_msec) + ", " + String(dur_msec) + ", " + String(buff_bytes_before) + ", " + String(bytes_transferred) + ", " + String(buff_bytes_after);
    }
};

class SD_Log {
  public:
    constexpr static uint32_t N_LOG = 5000;
    uint32_t write_ind = 0;
    LogEntry log_array[N_LOG];

    void resetLog(void) { write_ind = 0; 
      for (uint32_t i = 0; i < N_LOG; i++) {
        log_array[write_ind].time_msec = 0;
        log_array[write_ind].dur_msec = 0;
        log_array[write_ind].buff_bytes_before = 0;
        log_array[write_ind].bytes_transferred = 0;
        log_array[write_ind].buff_bytes_after = 0;      
      }
    }

    //add entry at current write index
    int addEntry(const uint32_t _time_msec, const uint16_t _dur_msec, uint32_t _buff_bytes_before, uint16_t _bytes_transferred, const unsigned long _buff_bytes_after) {
      if (write_ind < (N_LOG-1)) {
        log_array[write_ind].time_msec = _time_msec;
        log_array[write_ind].dur_msec = _dur_msec;
        log_array[write_ind].buff_bytes_before = _buff_bytes_before;
        log_array[write_ind].bytes_transferred = _bytes_transferred;
        log_array[write_ind].buff_bytes_after = _buff_bytes_after;
        if (write_ind <= (N_LOG-1)) write_ind++;
        return 0;
      }
      return -1;
    }

    //get the next available filename for the log...to be called in setup()
    static String findAndSetNextFilenameForLog(SdFs *sd_ptr, const String fname_pre) { 
      int recording_count = 0; 
      String out_fname = String();

      //this code was largely copied from the Tympan_Library AudioSDWriter::startRecording()
      bool done = false;
      while ((!done) && (recording_count < 998)) {
        
        recording_count++;
        if (recording_count < 1000) {
          //make file name
          String fname = fname_pre + String("xxx.TXT");
          int offset = fname_pre.length();
          int hundreds = recording_count / 100;
          fname[offset+0] = hundreds + '0';  //stupid way to convert the number to a character
          int tens = (recording_count - (hundreds*100)) / 10;  //truncates
          fname[offset+1] = tens + '0';  //stupid way to convert the number to a character
          int ones = recording_count - (tens * 10) - (hundreds*100);
          fname[offset+2] = ones + '0';  //stupid way to convert the number to a character

          //does the file exist?
          if (sd_ptr->exists(fname)) {
            //loop around again
            done = false;
          } else {
            //this is our winner!
            out_fname = String(fname);
            done = true;
          }
        } else {
          Serial.println(F("findAndSetNextFilenameForLog: Cannot do more than 999 files."));
          done = true; //don't loop again
        } //close if (recording_count)
          
      } //close the while loop
      return out_fname;
    }

    //print the log to SD card
    String writeLogToSD(SdFs *sdfs, const String fname_pre) {
      //create a file and open it
      String log_filename = findAndSetNextFilenameForLog(sdfs, fname_pre);
      if (log_filename.length() > 0) {
        FsFile myfile = sdfs->open(log_filename, O_WRITE | O_CREAT);  //will append to the end of the file
        for (uint32_t i=0; i < write_ind; i++) myfile.println(log_array[i].toString());
        myfile.close(); 
      }
      return log_filename;
    }
};

class AudioSDPlayer_F32_logging : public AudioSDPlayer_F32 {
  public:
    AudioSDPlayer_F32_logging(SdFs * _sd,const AudioSettings_F32 &settings) : AudioSDPlayer_F32(_sd, settings) {};
    int serviceSD(void) {
      //get the current time
      uint32_t buffBytesBefore = getNumBuffBytes();
      unsigned long cur_millis = millis();
      unsigned long start_micros = micros();
      int ret_val = AudioSDPlayer_F32::serviceSD();
      uint32_t dur_msec = (micros() - start_micros)/1000UL;
      uint32_t bytes_read = AudioSDPlayer_F32::getNBytesLastReadFromSD();
      if (bytes_read > 0) sd_log.addEntry((uint32_t)cur_millis, (uint16_t)dur_msec, buffBytesBefore, (uint16_t)bytes_read, getNumBuffBytes());
      return ret_val;
    }

    void resetLog(void) { sd_log.resetLog(); Serial.println("AudioSDWriter_F32_UI_logging: reset log"); }
    String writeLogToSD(void) { return sd_log.writeLogToSD(sd_ptr, String("SD_readLog_"));  }

  protected:
    SD_Log sd_log;
};

class AudioSDWriter_F32_UI_logging : public AudioSDWriter_F32_UI {
  public:
    AudioSDWriter_F32_UI_logging(SdFs * _sd,const AudioSettings_F32 &settings) : AudioSDWriter_F32_UI(_sd, settings) {};
    int serviceSD_withWarnings(AudioInputI2SQuad_F32 &i2s_in) {
      //get the current time
      uint32_t bytes_before = getNumUnfilledSamplesInBuffer()*2;
      unsigned long cur_millis = millis();
      unsigned long start_micros = micros();
      int bytes_written = AudioSDWriter_F32_UI::serviceSD_withWarnings(i2s_in);
      uint32_t dur_msec = (micros() - start_micros)/1000UL;
      if (bytes_written > 0) sd_log.addEntry((uint32_t)cur_millis,(uint16_t)dur_msec, bytes_before,  (uint16_t)bytes_written, getNumUnfilledSamplesInBuffer()*2);
      return bytes_written;
    }

    void resetLog(void) { sd_log.resetLog(); Serial.println("AudioSDWriter_F32_UI_logging: reset log"); }
    String writeLogToSD(void) { return sd_log.writeLogToSD(sd, String("SD_writeLog_"));  }

  protected:
    SD_Log sd_log;
};


#endif