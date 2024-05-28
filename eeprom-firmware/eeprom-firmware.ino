#include "eeprom-io.h"
#include "eeprom-at28c64.h"
#include "eeprom-24lcxx.h"

#define TYPE_AT28C64 0
#define TYPE_24LCXX 1

const uint8_t activityLed = 11;

EepromIoAT28C64 ioAT28C64;
EepromIo24LCxx io24LCxx;

EepromIo* io = nullptr;

void setup() {
  ioAT28C64.setup();
  io24LCxx.setup();

  pinMode(activityLed, OUTPUT);
  digitalWrite(activityLed, LOW); // Inactive

  Serial.begin(9600);
}

uint8_t nextSerial() {
  while (!Serial.available())
    delay(1);
  return Serial.read();
}

#define WAIT_OR_TIMEOUT \
  while (!Serial.available()) { \
    if (millis() - lastCmdTime > 10000) \
      goto timeout; \
    delay(1); \
  }

void loop() {
  // Wait for device init command
  while (nextSerial() != 'd');
  uint8_t eepromType = nextSerial();
  switch (eepromType) {
    case TYPE_AT28C64:
      io = &ioAT28C64;
      break;
    case TYPE_24LCXX:
      io = &io24LCxx;
      break;
    default:
      return;
  }

  // Initialize device
  digitalWrite(activityLed, HIGH);
  io->init();

  uint64_t lastCmdTime = millis();
  while (true) {
    WAIT_OR_TIMEOUT;
    uint8_t command = Serial.read();
    lastCmdTime = millis();

    if (command == 'r') { // Read page
      WAIT_OR_TIMEOUT;
      uint16_t pageAddr = Serial.read() << 8;
      for (int i = 0; i < 256; i++) {
        uint16_t addr = pageAddr + i;
        uint8_t value = io->readByte(addr);
        Serial.write(value);
      }
    } else if (command == 'w') { // Write page
      WAIT_OR_TIMEOUT;
      uint16_t pageAddr = Serial.read() << 8;
      for (int i = 0; i < 256; i++) {
        uint16_t addr = pageAddr + i;
        WAIT_OR_TIMEOUT;
        uint8_t value = Serial.read();
        io->writeByte(addr, value);
      }

      // Indicate we're done and ready for more data
      Serial.write(0xa5);
    } else if (command == 'q') { // Quit
      break;
    }
  }

  timeout:
  io->disable();
  digitalWrite(activityLed, LOW);
  io = nullptr;
}
