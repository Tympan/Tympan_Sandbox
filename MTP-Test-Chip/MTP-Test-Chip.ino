#include "Arduino.h"

#include "SD.h"
#include "MTP.h"

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
SDClass sdx;

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

void printHelp(void) {
  Serial.println("MTP-Test-Chip: This is the help:");
  Serial.println("  Press 'h' or 'l' or '?' to get this help.");
  Serial.println("  Directory list from SD card:");    
  sdx.sdfs.ls();
}

void setup()
{
  Serial.begin(115200); delay(2000);
  Serial.println("MTP_test: begin setup()");

  #if !__has_include("usb_mtp.h")
    usb_mtp_configure();
  #endif
  storage_configure();

  // Set Time callback // needed for SDFat
  FsDateTime::callback = dateTime;

  // write a test file to the SD card
  Serial.println("writing test1.txt to SD card");
  const char *str = "test1.txt";
  if(sdx.exists(str)) sdx.remove(str);
  File file=sdx.open(str,FILE_WRITE_BEGIN);
  file.println("This is a test line");
  file.close();
  
  // get a directory listin from the SD
  printHelp();
}

void loop()
{
  if (Serial.available()) {
    char c = Serial.read();
    if ((c == 'h') || (c == 'l') || (c == '?')) {
      printHelp();
    }
  }
  mtpd.loop();
}
