// 2024-07-28
// arBLE_Central.ino (client)
// ---------------------------------
// Bluetooth lw energy (BLE) test
// use 'ArduinoBLE' 1.3.6


#include "Arduino_LED_Matrix.h"
#include "AFlib.h"

#include <ArduinoBLE.h>

#define INPUT_SIMULATION false

#define inputPin 3
#define devUID   "19B10010-E8F2-537E-4F6C-D104768A1214"
#define ledUID   "19B10011-E8F2-537E-4F6C-D104768A1214"
#define devNAME  "arBLE Peripheral"

// ram di lavoro per display
uint32_t LED_RAM[] = {
	0x00000000,
	0x00000000,
	0x00000000
};

// pattern per display
uint32_t LED_BTC[] = {
  0xceea48a4,
  0x8c48a48a,
  0x48c4e000
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

// data for display_points()
int cnt_row = 0;
int cnt_bit = 0;

const uint32_t LED_CLEAN[] = {
	0x00000000,
	0x00000000,
	0x00000000
};

// 32 bit-mapped
uint32_t LED_SEQ[] = {
	0x80000000,  // 31
	0x40000000,  // 30
	0x20000000,  // 29
	0x10000000,  // 28
	0x08000000,  // 27
	0x04000000,  // 26
	0x02000000,  // 25
	0x01000000,  // 24
	0x00800000,  // 23
	0x00400000,  // 22
	0x00200000,  // 21
	0x00100000,  // 20
	0x00080000,  // 19
	0x00040000,  // 18
	0x00020000,  // 17
	0x00010000,  // 16
	0x00008000,  // 15
	0x00004000,  // 14
	0x00002000,  // 13
	0x00001000,  // 12
	0x00000800,  // 11
	0x00000400,  // 10
	0x00000200,  // 9
	0x00000100,  // 8
	0x00000080,  // 7
	0x00000040,  // 6
	0x00000020,  // 5
	0x00000010,  // 4
	0x00000008,  // 3
	0x00000004,  // 2
	0x00000002,  // 1
	0x00000001,  // 0
};

// istanza classe di libreria
ArduinoLEDMatrix matrix;

// variabili copia
int oldButtonState = LOW;
byte oldLedValue = 0;

FTIMER myTimer;
// ------------------------------------------------
// sequential point display on led-matrix
// ------------------------------------------------
void display_points() {
  if(cnt_row == 0 && cnt_bit == 0) {
    for(int j = 0; j < 3; j++) {
        LED_RAM[j] = 0;
    }
  }
  LED_RAM[cnt_row] |= LED_SEQ[cnt_bit];
  matrix.loadFrame(LED_RAM);
  cnt_bit++;
  if(cnt_bit >= 32) { cnt_bit = 0; cnt_row++; }
  if(cnt_row >= 3) cnt_row = 0;
}

// inizializzazione
void setup() {

  Serial.begin(115200);
  matrix.begin();
  matrix.loadFrame(LED_BTC);
  delay(3000);

  pinMode(inputPin, INPUT);

  while (!Serial) {
    // attesa seriale (USB) pronta
  }
  
  if(!BLE.begin()) {
    matrix.loadFrame(LED_E1);
    while(1);
  }

  BLE.scanForUuid(devUID);
  Serial.println("BLE Central ready, waiting for connections...");

  cnt_row = 0;
  cnt_bit = 0;
  matrix.loadFrame(LED_OK);
}

// chiamata a loop
void loop() 
{
  // check if a peripheral has been discovered
  BLEDevice peripheral = BLE.available();

  if (peripheral) {
    // discovered a peripheral, print out address, local name, and advertised service
    Serial.print("Found '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.println(peripheral.advertisedServiceUuid());

    if (peripheral.localName() != devNAME) {
      return;
    }

    // stop scanning
    BLE.stopScan();
    controlLed(peripheral);

    // peripheral disconnected, start scanning again
    BLE.scanForUuid(devUID);
    cnt_row = 0;
    cnt_bit = 0;
    matrix.loadFrame(LED_OK);
  }
}
// -------------------- main function ----------------------
void controlLed(BLEDevice peripheral) {
  // connect to the peripheral
  Serial.println("Connecting ...");

  if (peripheral.connect()) {
    Serial.println("Connected");
  } else {
    Serial.println("Failed to connect!");
    return;
  }

  // discover peripheral attributes
  Serial.println("Discovering attributes ...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Attributes discovered");
  } else {
    Serial.println("Attribute discovery failed!");
    peripheral.disconnect();
    return;
  }

  // retrieve the LED characteristic
  BLECharacteristic ledCharacteristic = peripheral.characteristic(ledUID);

  if (!ledCharacteristic) {
    Serial.println("Peripheral does not have LED characteristic!");
    peripheral.disconnect();
    return;
  } else if (!ledCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable LED characteristic!");
    peripheral.disconnect();
    return;
  }
  
  myTimer.start(5000);
  while (peripheral.connected()) {
    // while the peripheral is connected (internal loop!)
    // read the button pin
    int buttonState = digitalRead(inputPin);
    if(INPUT_SIMULATION == true) {
        buttonState = oldButtonState; // digitalRead(inputPin);
      if(myTimer.done() == true) {
        if(oldButtonState) buttonState = LOW; else buttonState = HIGH;
        myTimer.start(5000);
      }
    } 
    if (oldButtonState != buttonState) {
      // button changed
      oldButtonState = buttonState;

      if (buttonState) {
        Serial.println("button pressed: write 1");
        // button is pressed, write 0x01 to turn the LED on
        ledCharacteristic.writeValue((byte)0x01);
      } else {
        Serial.println("button released: write 0");
        // button is released, write 0x00 to turn the LED off
        ledCharacteristic.writeValue((byte)0x00);
      }
    } 
    // lettura stato led(relais)
    byte value = 0;
    ledCharacteristic.readValue(value);
    if (oldLedValue != value) {
      display_points();
      oldLedValue = value;
      if (value) {
        // first bit corresponds to the right button
        Serial.println("RELAIS ON");
      } else {
        Serial.println("RELAIS OFF");
      }
    }  
  }
  Serial.println("Peripheral disconnected");
  cnt_row = 0;
  cnt_bit = 0;
  matrix.loadFrame(LED_OK);
}
