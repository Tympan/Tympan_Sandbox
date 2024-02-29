
// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// This code will run on the **nRF52** to allow the TympanRemote app see the UART-like service that we're offering
// over BLE.  This code will likely be part of our nRF52 firmware.
//
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef nRF52_BLEUART_TYMPAN_h
#define nRF52_BLEUART_TYMPAN_h

/* 
  Class: BLEUart_Tympan
  Created: Chip Audette, OpenAudio  Feb 2024
  Purpose: Extend the Adafuit_nF52_Arduino/BLEUart class to allow setting of the characteristic UUIDs

  License: MIT License, Use at your own risk
*/

#include <bluefruit.h> //for BLUUart

class BLEUart_Tympan : public BLEUart
{
  public:
    void setCharacteristicUuid(BLECharacteristic &ble_char) { 
      _txd = ble_char;  //_txd is in BLEUart
      _rxd = ble_char;  //_rxd is in BLEUart
    };
};

#endif