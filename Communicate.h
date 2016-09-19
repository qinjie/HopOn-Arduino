const int DEBUG = 0;
const int RESET_MEMORY = 1;
int speakerPin = 5;
int buttonPin = A1;
String llb_serial = "SG11111";

typedef enum BicycleState {
  free_state = 0,
  locked_state = 1,
  unlocked_state = 2
};

typedef enum Action {
  unlock_bicycle = 1,
  lock_bicycle = 2,
  return_bicycle = 3,
  get_state = 4
};

template <typename T>
inline void logMsg(T msg) {
  if (DEBUG) Serial.println(msg);
}
