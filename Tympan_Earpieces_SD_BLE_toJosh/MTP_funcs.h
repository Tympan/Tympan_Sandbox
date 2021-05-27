
#ifndef MTP_funcs_h
#define MTP_funcs_h


#if (USE_MTP)


#if defined(__IMXRT1062__)  //Teensy 4.x
  // following only as long usb_mtp is not included in cores 
  #if !__has_include("usb_mtp.h")
    #include "usb1_mtp.h"
  #endif
#else
  #ifndef BUILTIN_SCCARD
    #define BUILTIN_SDCARD 254
  #endif
  void usb_mtp_configure(void) {}
#endif

MTPStorage_SD storage;
MTPD    mtpd(&storage);

// define SD card parameters
const char *sd_str="sdio"; // edit to reflect your configuration
const int cs = BUILTIN_SDCARD; // edit to reflect your configuration
//SDClass sdx;
extern SDClass sdx;  //defined in the main file

void storage_configure()
{
  if(!sdx.sdfs.begin(SdioConfig(FIFO_SDIO))) { 
    Serial.printf("SDIO Storage failed or missing");  Serial.println();
  } else {
    storage.addFilesystem(sdx, sd_str);
    uint64_t totalSize = sdx.totalSize();
    uint64_t usedSize  = sdx.usedSize();
    Serial.printf("SDIO Storage %s ",sd_str);
    Serial.print(totalSize); Serial.print(" "); Serial.println(usedSize);
  }  
}

// Call back for file timestamps.  Only called for file create and sync(). needed by SDFat-beta
#include "TimeLib.h"
void dateTime(uint16_t* date, uint16_t* time, uint8_t* ms10) {
  *date = FS_DATE(year(), month(), day());
  *time = FS_TIME(hour(), minute(), second());
  *ms10 = second() & 1 ? 100 : 0;
}

//put this function in setup()
void setup_MTP(void) {
  #if !__has_include("usb_mtp.h")
    usb_mtp_configure();
  #endif
  storage_configure();

  // Set Time callback // needed for SDFat
  FsDateTime::callback = dateTime;
}

#endif  //if USE_MTP
#endif  //ifndef MTP_funcs_h
