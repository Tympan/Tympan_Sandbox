

#define BLE_SERIAL Serial7

void printHelp(void) {
  Serial.println("Tympan Help:");
  Serial.println("    : h : print this help");
  Serial.println("    : All characters (including 'h') are sent to BLE");
}

void setup() {
  // put your setup code here, to run once:
  BLE_SERIAL.begin(115200/2);delay(1000);
  Serial.println("Serial Echo: starting...");
  printHelp();
}

void loop() {

  // put your main code here, to run repeatedly:
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'h') printHelp();
    Serial.println("Tympan sending to nRF: " + String(c));
    BLE_SERIAL.print(c);
  }

  //echo characters from Serial1 to the USB serial
  //(include some waiting to ensure we get the whole message before printing the end-of-line)
  bool flag_serviceSerial1 = BLE_SERIAL.available();
  if (flag_serviceSerial1) {
    Serial.print("Received from nRF: ");
    while (flag_serviceSerial1) {
      while (BLE_SERIAL.available()) {
        char c = BLE_SERIAL.read();
        Serial.write(c);
      }
      delay(10);
      flag_serviceSerial1 = BLE_SERIAL.available();
    }
    Serial.println();
  }

}
