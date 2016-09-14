#include <MD5.h>
#include "pitch.h"

int state = 0;
int speakerPin = 5;
int melody[] = {NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};
int noteDurations[] = {4, 8, 8, 4,4,4,4,4 };
String llb_serial = "SG11113";
String hash_in, user_in, action_in;
// State = { 0: free, 1: locked, 2: unlocked }
// Action = { 0: unlock, 1: lock, 2: return }

void setup() {
  Serial.begin(9600);
  
  Bean.enableMotionEvent(ORIENT_EVENT);
  uint8_t mode = Bean.getAccelerometerPowerMode();
  if (mode != VALUE_LOW_POWER_10MS) {
    Bean.accelRegisterWrite(REG_POWER_MODE_X11, VALUE_LOW_POWER_10MS);
  }
  
  Serial.println("Started");
  Bean.setLed(0, 0, 0);
}

void loop() {
  Serial.println("loop");
  if (validProtocol()) {
    
    // Unlock when bicycle is free or locked
    if (action_in == "0" && (state == 0 || state == 1)){
      soundBuzzer();

      // Bicycle is unlocked
      state = 2;
      Serial.println("Unlocked!");

      // Blink Green
      Bean.setLed(0, 255, 0);
      Bean.sleep(2000);   
      Bean.setLed(0, 0, 0);
    }

    // Lock when bicycle is unlocked
    if (action_in == "1" && state == 2){
      soundBuzzer();

      // Bicycle is locked
      state = 1;
      Serial.println("Locked!");

      // Blink Red
      Bean.setLed(255, 0, 0);
      Bean.sleep(2000);  
      Bean.setLed(0, 0, 0);
    }

    // Return when bicycle is locked or unlocked
    if (action_in == "2" && (state == 1 || state == 2)){
      soundBuzzer();

      // Bicycle is free
      state = 0;
      Serial.println("Returned!");

      // Blink Blue
      Bean.setLed(0, 0, 255);
      Bean.sleep(2000);  
      Bean.setLed(0, 0, 0);
    }
  }

  // When bicycle is free or locked
  if (state == 0 || state == 1) {
    bool isMoving = Bean.checkMotionEvent(ORIENT_EVENT);
    while (isMoving) {
      alertBuzzer();
      isMoving = Bean.checkMotionEvent(ORIENT_EVENT);
    }
  }
  Bean.sleep(0xFFFFFFFF);
}

bool validProtocol() {
  String data = Serial.readString();
  char arraydata[46];
  char data_array[3][46];
  data.toCharArray(arraydata, 46);
  int count = 0;
  int index = 0;
  for (int i=0; i< 46; i++){
    if (data[i] != '\0'){
      if (data[i] == ','){
        data_array[index][count] = '\0';
        count = 0;
        index++;
      }
      else{
        data_array[index][count] = data[i];
        count++;
      }
    }
    else{
      break;
    }
  }
  data_array[index][count] = '\0';
  hash_in = String(data_array[0]);
  user_in = String(data_array[1]);
  action_in = String(data_array[2]);
  String hash_out = llb_serial + "," + user_in;
  int len = user_in.length() + llb_serial.length() + 2;
  char arrayhash[len];
  hash_out.toCharArray(arrayhash, len);
  unsigned char* hash=MD5::make_hash(arrayhash);
  char *md5str = MD5::make_digest(hash, 16);
  free(hash);
  Serial.println("check " + String(md5str));
  return String(md5str) == hash_in;
}

void soundBuzzer() {
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    tone(speakerPin, melody[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(speakerPin);
  }
}

void alertBuzzer() {
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    if (thisNote & 1) { // is odd note
      Bean.setLed(0, 0, 255);
    } else {  // is even note
      Bean.setLed(255, 0, 0);
    }
    tone(speakerPin, melody[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(speakerPin);
  }
  Bean.setLed(0, 0, 0);
}

