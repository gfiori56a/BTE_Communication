// ------- HC SR04 Acquisition --------
#include "AFlib.h"

#define trigPin    8      // output to HC-SR04
#define echoPin    2      // input from HC-SR04
#define msPoll   100      // 0.1 second for minimum polling time 
#define threshold 20      // cm       
HC_SR04 obj;

void setup() {
  Serial.begin(115200);
  obj.setup(trigPin, echoPin);
  obj.setPolling(msPoll); 
  obj.setThrDistance(threshold);

  while (!Serial) {} // attesa seriale (USB) pronta
  delay(1000);
  Serial.println("setup done!");
}
void loop() {
  int distanza = 0;
  bool chg = obj.read(&distanza);
  //Serial.print("change=");
  //Serial.print(chg);
  if(chg) {
    Serial.print(" distance[cm]=");
    Serial.println(distanza);
  }
  delay(500);
}

