// ------- Test Matrix inversion --------
#include "Arduino_LED_Matrix.h"
#include "AFlib.h"
#define inPin  3

uint32_t * LED_RAM;
uint32_t LED_E27[] = {
  0xeef82282,
  0x4ec48848,
  0x84ee4000
};

uint32_t LED_E27I[] = {
  0x27721,
  0x12112372,
  0x41441f77
};

MATRIX_RAM ram;
ArduinoLEDMatrix matrix;

bool btState;
bool btState_cpy;
char buf[255];

void setup() {
  Serial.begin(115200);
  while (!Serial) {} // attesa seriale (USB) pronta
  LED_RAM = ram.getRam();
  matrix.begin();

  btState = (bool) digitalRead(inPin);
  btState_cpy = !btState;

  pinMode(inPin, INPUT);
  Serial.println("setup done!");
}
void loop() {
  btState = (bool) digitalRead(inPin);
  if (btState_cpy != btState) 
  {
    btState_cpy = btState;
    if (btState) {
      Serial.println("INVERSION");
      ram.updown(LED_E27);
      sprintf(buf, "Ref[0]=%08X Computed=%08X\nRef[1]=%08X Computed=%08X\nRef[2]=%08X Computed=%08X\n", 
         LED_E27I[0], LED_RAM[0], LED_E27I[1], LED_RAM[1], LED_E27I[2], LED_RAM[2]);
      Serial.println(buf);

    } else {
      Serial.println("STANDARD");
      ram.writeRam(LED_E27);    
    }
    matrix.loadFrame(LED_RAM);
  }
  delay(100);
}
