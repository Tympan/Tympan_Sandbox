#ifndef BottleBuoy_BLE_h
#define BottleBuoy_BLE_h

//some externals that are defined in another file in this sketch
extern BLE_nRF52&    ble_nRF52;


//For bottlebuoy, we need to setup the BLE connection differently than the standard BLE setup used
//in the main Tympan_Library.  So, we're going to write our own setup routine for the BLE module 
//and never call the Tympan_Library's version of the BLE setup function
//
//As a model, I'm copiying from Tympan_Library\src\BLE\ble_nRF52.cpp, the begin() method
//
String newMacAddress = "C342D3594245";  //set the MAC address that JElliot's phone requires for a bottle
String newBleName = "Bott";  //set the BLE Name that JElliot's phone requires for a bottle
int battery_ble_service_id = BLE_nRF52::BLESVC_BATT;               //the bottle emits this service, so we will too
int bottle_ble_service_id = BLE_nRF52::BLESVC_LEDBUTTON_4BYTE;  //this is the service that bottlebuoy needs
int bottle_ble_volume_char_id = 0; //bottle buoy empty "volume"
int bottle_ble_temperature_char_id = 1; //bottle buoy "temperature"??
void manuallySetupAndBeginBLE(void) {
  String reply;     //we'll need this later (probably)
  bool enable;      //we'll need this later
  int ret_err_code; //we'll need this later

  //clear the incoming Serial buffer
  HardwareSerial *serialFromBLE = &Serial7;  //Tympan Rev F using Serial 7.  Other Revs using different Serial.
	delay(2000); while (serialFromBLE->available()) serialFromBLE->read();

  //make sure that the communication to the BLE module is working...let's try to get the BLE firwmare rev
  String version_str;	
  ret_err_code = ble_nRF52.version(&version_str);
	if (ret_err_code != 0) { 
    Serial.println("manuallySetupAndBeginBLE: Error in getting BLE version.  Comm to BLE module not working? Continuing...");
  } else {
    Serial.println("manuallySetupAndBeginBLE: BLE firmware =" + version_str);
	}

  //Change the MAC address advertised by the BLE module, if desired
  ret_err_code = ble_nRF52.setBleMac(newMacAddress);

  //activate the preset for the BLE battery status service 
  enable = true;
  ret_err_code = ble_nRF52.enableServiceByID(battery_ble_service_id, enable);

  //activate the preset for the BLE service that our phone app expects
  enable = true;
  ret_err_code = ble_nRF52.enableServiceByID(bottle_ble_service_id, enable);

  //set the module to advertise the BLE service (as required by the Creare phone app)
  ret_err_code = ble_nRF52.enableAdvertiseServiceByID(bottle_ble_service_id);

  //and, crucially, now begin the module's BLE service
	ble_nRF52.sendCommand("BEGIN ","1");delay(5);
  ble_nRF52.recvReply(&reply); 
	if (!ble_nRF52.doesStartWithOK(reply)) Serial.println("manuallySetupAndBeginBLE: the BEGIN command failed."); 

  //set the BLE name *after* the BLE has been begun?
  //
  //Change the BLE Name advertised by the BLE module (that our phone app expects)
  ret_err_code = ble_nRF52.setBleName(newBleName);

  return;
}


// Function to send the floating-point volume value out via BLE
uint32_t floatToBytes(float32_t value) {  uint32_t result;  memcpy(&result, &value, sizeof(value)); return result; }
void sendFloat32ToBLE(int service_id, int char_id, float32_t val_float) {
  String reply;  //we'll need this later

  //prepare to send data values to our target BLE Service (defined by bottle_ble_service_id) and our target BLE Characteristic (defined by bottle_ble_volume_char_id value)
  int n_bytes = 4;
  String params_str = String(" ") + String(service_id) + " " + String(char_id) + " " + String(n_bytes) + " "; //just start and end with a aspace
    
  //get the 4 bytes of the float32 volume value
  uint32_t vol_uint32 = floatToBytes(val_float);
  uint8_t byte_array[n_bytes];
  for (uint8_t Ibyte = 0; Ibyte < n_bytes; ++Ibyte) byte_array[Ibyte]=(uint8_t)(0x000000FF & (vol_uint32 >> (Ibyte*8)));  //lower bytes first

  //send via BLE NOTIFY
  String command_str = "BLENOTIFY";
  //Serial.print("floatToBytes: Full Command: " + command_str + params_str + ": "); for (int i=0; i<n_bytes;i++) { Serial.print("0x"); Serial.print(data_uint8[i],HEX); Serial.print(" ");}; Serial.println();
  ble_nRF52.sendCommand(command_str + params_str, byte_array, n_bytes);   
  delay(2);ble_nRF52.recvReply(&reply); if (!ble_nRF52.doesStartWithOK(reply)) Serial.println("sendFloat32ToBLE: failed send BLE data via " + command_str + ". Reply = " + reply);

  //send via BLE WRITE
  String command2_str = "BLEWRITE";
  ble_nRF52.sendCommand(command2_str + params_str, byte_array, n_bytes); 
  delay(2);ble_nRF52.recvReply(&reply); if (!ble_nRF52.doesStartWithOK(reply)) Serial.println("sendFloat32ToBLE: failed send BLE data via " + command2_str + ". Reply = " + reply);

}


#endif