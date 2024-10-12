// 2024-08-13
// arBLE_Peripheral.ino (server)
// ---------------------------------
// Bluetooth lw energy (BLE) test
// use 'ArduinoBLE' 1.3.7


#include "Arduino_LED_Matrix.h"
#include <ArduinoBLE.h>
#include <AFlib.h>

#define inputPin 3       // digital input
#define relePin  9       // relay

#define echoPin  2       // HC-SR04
#define trigPin  8       // HC-SR04

// ram di lavoro per display
uint32_t * LED_RAM;

uint32_t LED_BTP[] = {
  0xceea49a4,
  0x9c4ea48a,
  0x48c48000
};

uint32_t LED_E1[] = {
  0xe408c084,
  0xe408408,
  0x40e40000
};
uint32_t LED_E2[] = {
  0xee082082,
  0xec08808,
  0x80ee0000
};

uint32_t LED_E3[] = {
  0xee082082,
  0xec08208,
  0x20ee0000
};

uint32_t LED_OK[] = {
  0x7a44a,
  0x84b04b04,
  0xa87a4000
};

// istanza classe di libreria
ArduinoLEDMatrix matrix;
HC_SR04 ain;
#define MAX_POINTS 96
MATRIX_RAM ram;

// BLE
BLEService ledService("19B10010-E8F2-537E-4F6C-D104768A1214"); // create service
// create switch characteristic and allow remote device to read and write
BLEByteCharacteristic ledCharacteristic("19B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite);
// create button characteristic and allow remote device to get notifications
BLEByteCharacteristic buttonCharacteristic("19B10012-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);
// create button characteristic and allow remote device to get notifications
BLEIntCharacteristic distanceCharacteristic("19B10013-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

// inizializzazione
void setup() {

  Serial.begin(115200);
  matrix.begin();
  matrix.loadFrame(LED_BTP);
  delay(3000);

  pinMode(inputPin, INPUT);
  pinMode(relePin, OUTPUT);
  digitalWrite(relePin, HIGH);

  // sensor HC-SR04 management
  ain.setup(trigPin,echoPin);
  ain.setPolling(10);
  ain.setThrDistance(30);

  LED_RAM = ram.getRam();

  while (!Serial) {
    ; // attesa seriale (USB) pronta
  }
  if(!BLE.begin()) {
    matrix.loadFrame(LED_E1);
    while(1);
  }

  // set the local name peripheral advertises
  BLE.setLocalName("arBLE Peripheral");
  // set the UUID for the service this peripheral advertises:
  BLE.setAdvertisedService(ledService);

  // add the characteristics to the service
  ledService.addCharacteristic(ledCharacteristic);
  ledService.addCharacteristic(buttonCharacteristic);
  ledService.addCharacteristic(distanceCharacteristic);

  // add the service
  BLE.addService(ledService);

  ledCharacteristic.writeValue(0);
  buttonCharacteristic.writeValue(0);
  distanceCharacteristic.writeValue(-1);
  // start advertising
  BLE.advertise();
  Serial.println("BLE Peripheral ready, waiting for connections...");

  matrix.loadFrame(LED_OK);
}

// chiamata a loop
void loop() 
{
  static int ledValue = 0;
  static int cnt = 0;
  static int show = 1;
  BLE.poll();

  // read the current button pin state
  char buttonValue = digitalRead(inputPin);

  // has the value changed since the last read
  bool buttonChanged = (buttonCharacteristic.value() != buttonValue);

  if (buttonChanged) {
    // button state changed, update characteristics
    Serial.print("Digital input = ");
    Serial.println((long) buttonValue);
    ledCharacteristic.writeValue(buttonValue);
    buttonCharacteristic.writeValue(buttonValue);
  }
  int distance = -1;
  bool chg = ain.read(&distance);
  if(chg) {
    show |= 1;
    distanceCharacteristic.writeValue(distance);
    Serial.print("Distance[cm]=");
    Serial.println(distance);
  }    
  if (ledCharacteristic.written() || buttonChanged) {
    show |= 2;
    cnt++;
    if (cnt > MAX_POINTS) cnt = 0;
    // update LED, either central has written to characteristic or button state has changed
    if (ledCharacteristic.value() || buttonValue) {
      Serial.println("RELAIS on");
      digitalWrite(relePin, LOW);
      ledValue = 1;
    } else {
      Serial.println("RELAIS off");
      digitalWrite(relePin, HIGH);
      ledValue = 0;
    }
  }
  // display
  if(show) {
    if(show & 2) {
      ram.fillPoints(cnt);
    } else {
      ram.setNumber(distance, 300);
    }
    matrix.loadFrame(LED_RAM);
  }
  show = 0; 
}

