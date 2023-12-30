

void setButtonText(String btnId, String text) {
  String str = "TEXT=BTN:" + btnId + ":"+text;
  Serial.println(str);
  ble_local.sendMessage(str);
}

void sendTympanRemoteJSON(void) {
  String tympanResponse = String("JSON={'icon':'tympan.png','pages':[{'title':'Treble Boost Demo','cards':[{'name':'Highpass Gain (dB)','buttons':[{'label':'-','cmd':'K','width':'4'},{'id':'gainIndicator','width':'4'},{'label':'+','cmd':'k','width':'4'}]},{'name':'Highpass Cutoff (Hz)','buttons':[{'label':'-','cmd':'T','width':'4'},{'id':'cutoffHz','width':'4'},{'label':'+','cmd':'t','width':'4'}]}]},{'title':'Globals','cards':[{'name':'Analog Input Gain (dB)','buttons':[{'id':'inpGain','width':'12'}]},{'name':'CPU Usage (%)','buttons':[{'label':'Start','cmd':'c','id':'cpuStart','width':'4'},{'id':'cpuValue','width':'4'},{'label':'Stop','cmd':'C','width':'4'}]}]}],'prescription':{'type':'BoysTown','pages':['serialMonitor']}}");
  //ble_local.sendMessage(String("Y"));
  ble_local.sendMessage(tympanResponse);
  //ble_local.sendString(tympanResponse);
}

void respondToChar(char c) {
  switch (c) {
    case 'j': case 'J':
      Serial.println("respondToChar: Detected 'J' (or 'j').  Sending TympanRemote layout");
      sendTympanRemoteJSON();
      break;
    case 'c':
      Serial.println("Sending CPU update...");
      setButtonText("cpuValue", String(12.3));
      break;
    case 'K':
      Serial.println("Sending gainIndicator of -6");
      setButtonText("gainIndicator", String(-6));
      break;
    case 'k':
      Serial.println("Sending gainIndicator of +6");
      setButtonText("gainIndicator", String(6));
      break;
    case 'T':
      Serial.println("Sending cutoffHz of 500");
      setButtonText("cutoffHz", String(500));
      break;
    case 't':
      Serial.println("Sending cutoffHz of 2000");
      setButtonText("cutoffHz", String(2000));
      break;
    default:
      Serial.println("Sending character: " + String(c));
      ble_local.sendMessage(String(c));
  }
}
