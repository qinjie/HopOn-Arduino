/*
 * DEBUG = 1 to use function logMsg() to log output in serial monitor
 */
const int DEBUG = 0;

/*
 * RESET_MEMORY = 1 to reset EEPROM memory in Bean 
 * everytime battery is reinserted
 */
const int RESET_MEMORY = 1;

int speakerPin = 5;
int buttonPin = A1;

// Each Bean is attached with a bicycle serial
String llb_serial = "SG11114";

typedef enum BicycleState {
  free_state = 0,   // bicycle is not unlocked by anyone yet
  locked_state = 1,
  unlocked_state = 2
};

// actions are sent from mobile app
typedef enum Action {
  unlock_bicycle = 1,
  lock_bicycle = 2,
  return_bicycle = 3,
  get_state = 4
};

// Use this function to print variable into Serial Monitor
template <typename T>
inline void logMsg(T msg) {
  if (DEBUG) Serial.println(msg);
}
