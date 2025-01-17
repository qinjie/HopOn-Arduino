#include <MD5.h>
#include<EEPROM.h>
#include "pitch.h"
#include "Communicate.h"

int buttonState = 0;  // HIGH: off, LOW: on

// Different buzzer sounds for different actions
int melodyUnlock[] = {NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};
int melodyLock[] = {NOTE_C4, NOTE_B3, 0, NOTE_G3, NOTE_A3, NOTE_G3, NOTE_G3, NOTE_C4};
int melodyReturn[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_A3, NOTE_G3, NOTE_G3, NOTE_C4}; 
int noteDurations[] = {4, 8, 8, 4,4,4,4,4 };

// Initial state of bicycle is free
BicycleState state = free_state;

String hash_in, user_in, action_in;
Action action;
String savedHash, savedUserId;

// MD5 Hash is stored in EEPROM, from byte 0 to byte 32
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

// UserId is stored in EEPROM, from byte 64 to byte 80
String readUserIdEEPROM() {
  String res = "";
  for (int addr = 64; addr < 80; ++addr) {
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

// bicycle state is stored in EEPROM, at byte 99
BicycleState readBicycleStateEEPROM() {
  return static_cast<BicycleState>(EEPROM.read(99));
}
void writeBicycleStateEEPROM(BicycleState state) {
  EEPROM.write(99, state);
}

// Erase all values in EEPROM
void resetEEPROM() {
  for (int addr = 0; addr < 100; ++addr) {
    EEPROM.write(addr, '\0');
  }
  savedHash = "";
  savedUserId = "";
}

// Setup function runs only ONCE when sketch is started
void setup() {
  Serial.begin(56600);
  pinMode(speakerPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(buttonPin, HIGH);

  Bean.enableMotionEvent(ANY_MOTION_EVENT);
  Bean.enableMotionEvent(LOW_G_EVENT);
  Bean.enableMotionEvent(HIGH_G_EVENT);
  Bean.enableMotionEvent(FLAT_EVENT);
  Bean.enableMotionEvent(ORIENT_EVENT);
  Bean.enableMotionEvent(SINGLE_TAP_EVENT);
  Bean.enableMotionEvent(DOUBLE_TAP_EVENT);
  uint8_t mode = Bean.getAccelerometerPowerMode();
  if (mode != VALUE_LOW_POWER_10MS) {
    Bean.accelRegisterWrite(REG_POWER_MODE_X11, VALUE_LOW_POWER_10MS);
  }
  
  if (RESET_MEMORY) resetEEPROM();
  
  savedHash = readHashEEPROM();
  savedUserId = readUserIdEEPROM();
  state = readBicycleStateEEPROM();
  respondState();
  logMsg("saved ." + savedHash + "." + savedUserId + "."); 
  logMsg("Started");
  
  Bean.setLed(0, 0, 0);
  tone(speakerPin, 2000, 200);
  delay(300);
  noTone(speakerPin);
}

// Returns bicycle state to mobile app
void respondState() {
  switch (state) {
    case free_state: Serial.print("0"); break;
    case locked_state: Serial.print("1"); break;
    case unlocked_state: Serial.print("2"); break;
  }
}

void unlockBicycle() {
  state = unlocked_state;
  
  soundBuzzer(unlock_bicycle);
  // Blink Green
  Bean.setLed(0, 255, 0);
  Bean.sleep(1000);
  Bean.setLed(0, 0, 0);

  writeBicycleStateEEPROM(state);
  respondState();
  logMsg("Unlocked!");
}

void lockBicycle() {
  state = locked_state;
  
  soundBuzzer(lock_bicycle);
  // Blink Red
  Bean.setLed(255, 0, 0);
  Bean.sleep(1000);
  Bean.setLed(0, 0, 0);

  writeBicycleStateEEPROM(state);
  respondState();
  logMsg("Locked!");
}

void returnBicycle() {
  state = free_state;
  
  soundBuzzer(return_bicycle);
  // Blink Blue
  Bean.setLed(0, 0, 255);
  Bean.sleep(1000);
  Bean.setLed(0, 0, 0);
  
  resetEEPROM();
  respondState();
  logMsg("Returned!");
}

// Loop function runs every 2 seconds
void loop() {
  logMsg("loop");

   // Check if received message is valid
  if (validProtocol()) {
    logMsg("valid protocol");
    
    // Save hash and user id if this is first time some user unlocks bicycle
    if (savedHash.length() == 0 && action != get_state) {
      writeHashEEPROM(hash_in);
      writeUserIdEEPROM(user_in);
      savedHash = hash_in;
      savedUserId = user_in;
    }

    // Call function based on received action
    if (action == unlock_bicycle 
      && (state == free_state || state == locked_state)) unlockBicycle();
    else if (action == lock_bicycle && state == unlocked_state) lockBicycle();
    else if (action == return_bicycle 
      && (state == unlocked_state || state == locked_state)) returnBicycle();
    else if (action == get_state) respondState();
  } else {  // If no message is received
    // Check if button is pressed to lock bicycle
    buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && state == unlocked_state) {
      lockBicycle();
    } 

    // Check if bicycle is moving illegally
    if (state == free_state || state == locked_state) {
      bool isMoving = Bean.checkMotionEvent(ANY_MOTION_EVENT)
        || Bean.checkMotionEvent(FLAT_EVENT)
        || Bean.checkMotionEvent(ORIENT_EVENT)
        || Bean.checkMotionEvent(SINGLE_TAP_EVENT)
        || Bean.checkMotionEvent(DOUBLE_TAP_EVENT)
        || Bean.checkMotionEvent(HIGH_G_EVENT)
        || Bean.checkMotionEvent(LOW_G_EVENT);
      if (isMoving) {
        Serial.print("-1");
        alertBuzzer();
      }
    } 
  }
  Bean.sleep(1500);
}

/*
 * Checks if received message is in correct format
 * Format: MD5("<bicycleSerial>,<userId>"),<userId>,<action>
 * Message has three parts, seperated by comma
 * - Part 1: Hash MD5 of string "<bicycleSerial>,<userId>" 
 * (bicycleSerial and userId is seperated by comma).
 * Hash MD5 must have length = 32
 * - Part 2: userId. userId has length = 16
 * - Part 3: action (from enum Action)
 */
bool validProtocol() {
  String data = Serial.readString();
  int firstComma = data.indexOf(',');
  if (firstComma == -1) return false;
  hash_in = data.substring(0, firstComma);
  int secondComma = data.indexOf(',', firstComma + 1);
  if (secondComma == -1) return false;
  user_in = data.substring(firstComma + 1, secondComma);
  action_in = data.substring(secondComma + 1);
  // Seperate message into hash_in (Hash MD5), user_in (userId), action_in (action)
  logMsg("data = " + hash_in + "." + user_in + "." + action_in);

  // Check action is valid
  int temp = action_in.toInt();
  if (temp == 0) return false;
  else {
    action = static_cast<Action>(temp);
    if (action != unlock_bicycle && action != lock_bicycle && action != return_bicycle && action != get_state) 
      return false;
  }

  if (savedHash.length() == 32) {
    return (hash_in == savedHash && user_in == savedUserId);
  } else {
    String hash_out = llb_serial + "," + user_in;
    unsigned char* hash=MD5::make_hash((char*)hash_out.c_str());
    char *md5str = MD5::make_digest(hash, 16);
    logMsg("check " + String(md5str) + "------" + user_in);
    bool result = String(md5str) == hash_in;
    free(hash);
    free(md5str);
    return result;
  }
}

void soundBuzzer(Action action) {
  for (int thisNote = 0; thisNote < 8; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    switch (action) {
      case unlock_bicycle:
        tone(speakerPin, melodyUnlock[thisNote], noteDuration); break;
      case lock_bicycle:
        tone(speakerPin, melodyLock[thisNote], noteDuration); break;
      case return_bicycle:
        tone(speakerPin, melodyReturn[thisNote], noteDuration); break;
    }
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    noTone(speakerPin);
  }
}

void alertBuzzer() {
  for (int thisNote = 0; thisNote < 4; thisNote++) {
    if (thisNote & 1) { // is odd note
      Bean.setLed(0, 0, 255);
    } else {  // is even note
      Bean.setLed(255, 0, 0);
    }
    tone(speakerPin, 2000, 300);
    delay(400);
    noTone(speakerPin);
  }
  Bean.setLed(0, 0, 0);
}

