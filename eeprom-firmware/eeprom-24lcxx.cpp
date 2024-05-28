#include <Wire.h>
#include "eeprom-24lcxx.h"

static const uint8_t EEPROM_ADDR = 0x50; // External address set to 0

// Reuse IO pins at bottom of socket
static const uint8_t addr0 = 12;
static const uint8_t addr1 = 13;
static const uint8_t addr2 = 14;
static const uint8_t power = 17;
static const uint8_t writeProtect = 16;
static const uint8_t scl = 5;
static const uint8_t sda = 6;

void EepromIo24LCxx::setup() {
  wireActive = false;
  disable();
}

void EepromIo24LCxx::init() {
  pinMode(power, OUTPUT);
  digitalWrite(power, HIGH); // Power on

  pinMode(writeProtect, OUTPUT);
  digitalWrite(writeProtect, HIGH); // Disable write

  // Set lower 3 bits of address to 0
  pinMode(addr0, OUTPUT);
  digitalWrite(addr0, LOW);
  pinMode(addr1, OUTPUT);
  digitalWrite(addr1, LOW);
  pinMode(addr2, OUTPUT);
  digitalWrite(addr2, LOW);

  Wire.begin();
  Wire.setClock(400000);
  wireActive = true;
}

uint8_t EepromIo24LCxx::readByte(uint16_t address) {
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((int) (address >> 8));
  Wire.write((int) (address & 0xFF));
  Wire.endTransmission();

  Wire.requestFrom(EEPROM_ADDR, (uint8_t) 1);

  uint8_t data = 0xFF;
  if (Wire.available()) data = Wire.read();
  return data;
}

void EepromIo24LCxx::writeByte(uint16_t address, uint8_t value) {
  digitalWrite(writeProtect, LOW); // Enable writes
  delayMicroseconds(1);
  
  Wire.beginTransmission(EEPROM_ADDR);
  Wire.write((int) (address >> 8));
  Wire.write((int) (address & 0xFF));

  // TODO: Could batch up to 32 bytes at a time here
  Wire.write(value);

  Wire.endTransmission();

  delay(5);
  digitalWrite(writeProtect, HIGH);
  delay(5);
}

void EepromIo24LCxx::disable() {
  if (wireActive)
    Wire.end();
  wireActive = false;

  // Power off
  digitalWrite(writeProtect, LOW);
  digitalWrite(power, LOW);

  // No outputs
  pinMode(addr0, INPUT);
  pinMode(addr1, INPUT);
  pinMode(addr2, INPUT);
  pinMode(power, INPUT);
  pinMode(writeProtect, INPUT);
  pinMode(scl, INPUT);
  pinMode(sda, INPUT);  
}
