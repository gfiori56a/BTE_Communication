// ------- Led flash without suspensive delay() --------
#include "AFlib.h"
#define ledPin  9
#define MAXCNT  15

bool led = HIGH;
int led_cnt = 0;

FTIMER myTimer;
TDELTA dt;

void setup() {
  Serial.begin(115200);
  while (!Serial) {} // attesa seriale (USB) pronta

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, led);
  myTimer.start(1000);
  Serial.println("setup done!");
  dt.begin();
}
void loop() {
  if (led_cnt < MAXCNT)
  {
    if (myTimer.done() == true) {
      led_cnt++;
      led = !led;
      digitalWrite(ledPin, led);
      myTimer.start(1000);
    }
    if(led_cnt == MAXCNT) {
      Serial.print("time elapsed[ms]=");
      Serial.println(dt.end());
    }
  }
}

