// ------- numbers on led-display (0-999) --------

#include "Arduino_LED_Matrix.h"
#include "AFlib.h"

#define inputPin 3
MATRIX_RAM ram;

// ram di lavoro per display
uint32_t * LED_RAM;

int cnt; 
ArduinoLEDMatrix matrix;
#define MAX_COUNT 96
void setup() {
  Serial.begin(115200);
  while (!Serial) {} // attesa seriale (USB) pronta

  pinMode(inputPin, INPUT);

  LED_RAM = ram.getRam();
  ram.setNumber(-1, 0);
  matrix.begin();
  matrix.loadFrame(LED_RAM);
  delay(2000);
  Serial.println("setup done!");
  cnt = 0;
}
void loop() {

  if(digitalRead(inputPin)) {
    ram.setNumber(cnt, MAX_COUNT);
  } else {
    ram.fillPoints(cnt);
  } 
  matrix.loadFrame(LED_RAM);
  cnt++;
  if(cnt > MAX_COUNT) {
    cnt = 0;
    delay(2000);
  }  
  delay(500);
}
