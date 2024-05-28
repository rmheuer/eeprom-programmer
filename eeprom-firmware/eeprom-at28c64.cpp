#include "eeprom-at28c64.h"

static const uint8_t writeEnable = 19;
static const uint8_t outputEnable = 20;
static const uint8_t shiftClock = 8;
static const uint8_t shiftData = 9;
static const uint8_t shiftEnable = 10;
static const uint8_t powerEnable = 21;
static const uint8_t ioPins[8] = {
  12, 13, 14, 6, 5, 16, 17, 18
};

static void setAddress(uint16_t address) {
  uint8_t lo = address & 0xFF;
  uint8_t hi = address >> 8;

  digitalWrite(shiftClock, LOW);
  shiftOut(shiftData, shiftClock, MSBFIRST, hi);
  shiftOut(shiftData, shiftClock, MSBFIRST, lo);
}

static void setIOMode(uint8_t mode) {
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(ioPins[i], mode);
  }
}

void EepromIoAT28C64::setup() {
  pinMode(powerEnable, OUTPUT);
  pinMode(shiftEnable, OUTPUT);
  pinMode(shiftClock, OUTPUT);
  pinMode(shiftData, OUTPUT);

  disable();
}

void EepromIoAT28C64::init() {
  digitalWrite(powerEnable, LOW); // Power on

  pinMode(writeEnable, OUTPUT);
  digitalWrite(writeEnable, HIGH); // No write

  pinMode(outputEnable, OUTPUT);
  digitalWrite(outputEnable, HIGH); // No read

  digitalWrite(shiftEnable, HIGH); // Shift registers enabled

  delay(100); // Start up
}

uint8_t EepromIoAT28C64::readByte(uint16_t address) {
  setAddress(address);

  digitalWrite(outputEnable, LOW);
  delayMicroseconds(1);

  uint8_t value = 0;
  for (uint8_t i = 0; i < 8; i++) {
    if (digitalRead(ioPins[i])) {
      value |= (1 << i);
    }
  }

  digitalWrite(outputEnable, HIGH);
  delayMicroseconds(1);

  return value;
}

void EepromIoAT28C64::writeByte(uint16_t address, uint8_t value) {
  setAddress(address);
  setIOMode(OUTPUT);
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(ioPins[i], (value & (1 << i)) != 0);
  }
  delayMicroseconds(1);

  // Pulse write enable
  // Must be between 100 and 1000 ns
  digitalWrite(writeEnable, LOW);
  delayMicroseconds(1);
  digitalWrite(writeEnable, HIGH);
  delayMicroseconds(1);
  
  // Give EEPROM time to finish write operation
  delay(10);

  setIOMode(INPUT);
}

void EepromIoAT28C64::disable() {
  digitalWrite(powerEnable, HIGH); // No power
  digitalWrite(shiftEnable, LOW); // No address data

  // Disable all possible outputs
  pinMode(writeEnable, INPUT);
  pinMode(outputEnable, INPUT);
  setIOMode(INPUT);
}
