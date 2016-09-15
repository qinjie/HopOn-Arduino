#include <MD5.h>
#include<EEPROM.h>
#include "pitch.h"

int state = 0;
int speakerPin = 5;
int melody[] = {NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};
int noteDurations[] = {4, 8, 8, 4,4,4,4,4 };
String llb_serial = "SG11111";
String hash_in, user_in, action_in;
String savedHash, savedUserId;
// State = { 0: free, 1: locked, 2: unlocked }
// Action = { 0: unlock, 1: lock, 2: return }

String readHashEEPROM() {
  String res = "";
  for (int addr = 0; addr < 32; ++addr) {
    char val = EEPROM.read(addr);
    if (val != '\0') res += val;
    else break;
  }
  return res;
}

void writeHashEEPROM(String hash) {
  for (int addr = 0; addr < 32; ++addr) {
    EEPROM.write(addr, hash.charAt(addr));
  }
}

String readUserIdEEPROM() {
  String res = "";
  for (int addr = 64; ; ++addr) {
    char val = EEPROM.read(addr);
    if (val != '\0') res += val;
    else break;
  }
  return res;
}

void writeUserIdEEPROM(String userId) {
  for (int i = 0; i < userId.length(); ++i) {
    EEPROM.write(64 + i, userId.charAt(i));
  }
}

int readBikeStateEEPROM() {
  return EEPROM.read(99);
}

void writeBikeStateEEPROM(int bikeState) {
  EEPROM.write(99, bikeState);
}

void resetEEPROM() {
  for (int addr = 0; addr < 100; ++addr) {
    EEPROM.write(addr, '\0');
  }
}

void setup() {
  Serial.begin(9600);
  
  Bean.enableMotionEvent(ORIENT_EVENT);
  uint8_t mode = Bean.getAccelerometerPowerMode();
  if (mode != VALUE_LOW_POWER_10MS) {
    Bean.accelRegisterWrite(REG_POWER_MODE_X11, VALUE_LOW_POWER_10MS);
  }

//  resetEEPROM();
  // Get hash and user id from EEPROM
  savedHash = readHashEEPROM();
  savedUserId = readUserIdEEPROM();
  state = readBikeStateEEPROM();
  Serial.println(state);
  Serial.println("savedHash ." + savedHash + ".");
  Bean.setScratchNumber(1, state);
  
  Serial.println("Started");
  Bean.setLed(0, 0, 0);
}

void loop() {
  Serial.println("loop");
  if (validProtocol()) {
    // Save hash and user id
    if (savedHash.length() == 0) {
      writeHashEEPROM(hash_in);
      writeUserIdEEPROM(user_in);
    }
    
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

      writeBikeStateEEPROM(state);
      Bean.setScratchNumber(1, state);
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

      writeBikeStateEEPROM(state);
      Bean.setScratchNumber(1, state);
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
      
      resetEEPROM();
      Bean.setScratchNumber(1, state);
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
  if (savedHash.length() == 32) {
    return (hash_in == savedHash && user_in == savedUserId);
  } else {
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

