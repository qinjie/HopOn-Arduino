#include <MD5.h>
#include "pitch.h"

int state = 0;
int speakerPin = 5;
int melody[] = {NOTE_C4, NOTE_G3,NOTE_G3, NOTE_A3, NOTE_G3,0, NOTE_B3, NOTE_C4};
int noteDurations[] = {4, 8, 8, 4,4,4,4,4 };
String llb_serial = "SG11111";
// 0: free, 1: locked, 2: unlocked

void setup() {
        Serial.begin(9600);
        Serial.println("Started");
}

void loop() {
        while (Serial.available()==0) {
        
        }
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
//        Serial.println(String(data_array[0]));
//        Serial.println(String(data_array[1]));
//        Serial.println(String(data_array[2]));
        String hash_in = String(data_array[0]);
        String user_in = String(data_array[1]);
        String action_in = String(data_array[2]);
        String hash_out = llb_serial + "," + user_in;
        int len = user_in.length() + llb_serial.length() + 2;
        char arrayhash[len];
        hash_out.toCharArray(arrayhash, len);
        unsigned char* hash=MD5::make_hash(arrayhash);
        char *md5str = MD5::make_digest(hash, 16);
        free(hash);
//        Serial.println(md5str);
        if (String(md5str) == hash_in){
          if (action_in == "0" && (state == 0 || state == 1)){
            for (int thisNote = 0; thisNote < 8; thisNote++) {
              int noteDuration = 1000/noteDurations[thisNote];
              tone(speakerPin, melody[thisNote],noteDuration);
              int pauseBetweenNotes = noteDuration * 1.30;
              delay(pauseBetweenNotes);
              noTone(speakerPin);
            }
            state = 2;
            Serial.println("Unlocked!");
            Bean.setLed(0, 255, 0);
            Bean.sleep(2000);   
            Bean.setLed(0, 0, 0);
          }
          if (action_in == "1" && state == 2){
            for (int thisNote = 0; thisNote < 8; thisNote++) {
              int noteDuration = 1000/noteDurations[thisNote];
              tone(speakerPin, melody[thisNote],noteDuration);
              int pauseBetweenNotes = noteDuration * 1.30;
              delay(pauseBetweenNotes);
              noTone(speakerPin);
            }
            state = 1;
            Serial.println("Locked!");
            Bean.setLed(255, 0, 0);
            Bean.sleep(2000);  
            Bean.setLed(0, 0, 0);
          }
          if (action_in == "2" && (state == 1 || state == 2)){
            for (int thisNote = 0; thisNote < 8; thisNote++) {
              int noteDuration = 1000/noteDurations[thisNote];
              tone(speakerPin, melody[thisNote],noteDuration);
              int pauseBetweenNotes = noteDuration * 1.30;
              delay(pauseBetweenNotes);
              noTone(speakerPin);
            }
            state = 0;
            Serial.println("Returned!");
            Bean.setLed(0, 0, 255);
            Bean.sleep(2000);  
            Bean.setLed(0, 0, 0);
          }
        }
}
