

#define BLE_SERIAL Serial7
#define MESSAGE_LENGTH 256
#define GREEN_LED 15
#define RED_LED 16

unsigned char from_nRF[MESSAGE_LENGTH];
size_t from_nRFbyteCounter;
bool ledState = false;
unsigned long toggleTime = 600;
unsigned long lastToggle;

void printHelp(void) {
  Serial.println("Tympan Help:");
  Serial.println("    : h : print this help");
  Serial.println("    : All characters (including 'h') are sent to BLE");
}

void setup() {
  pinMode(RED_LED,OUTPUT); digitalWrite(RED_LED,ledState);
  pinMode(GREEN_LED,OUTPUT); digitalWrite(GREEN_LED,!ledState);
  lastToggle = millis();
  
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
    BLE_SERIAL.write(c);  // print(c);
  }

  //echo characters from Serial1 to the USB serial
  // bool flag_serviceSerial1 = BLE_SERIAL.available();
  if (BLE_SERIAL.available()) {
    from_nRFbyteCounter = BLE_SERIAL.readBytesUntil('\n', from_nRF, MESSAGE_LENGTH-1);
    Serial.print("Received "); Serial.print(from_nRFbyteCounter); Serial.println(" bytes from nRF:");
    for(int i=0; i<from_nRFbyteCounter; i++){
      Serial.write(from_nRF[i]);
    }
    Serial.println();
  }

  toggleLEDs(millis());

}

void toggleLEDs(unsigned long m){
  if(m > lastToggle+toggleTime){
    lastToggle = m;
    ledState = !ledState;
    digitalWrite(GREEN_LED,ledState);
    digitalWrite(RED_LED, !ledState);
  }
}



