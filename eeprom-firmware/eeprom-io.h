#ifndef INCLUDE_EEPROM_IO_H
#define INCLUDE_EEPROM_IO_H

#include <Arduino.h>

class EepromIo {
public:
  virtual void setup() = 0;

  virtual void init() = 0;

  virtual uint8_t readByte(uint16_t addr) = 0;

  virtual void writeByte(uint16_t addr, uint8_t value) = 0;

  virtual void disable() = 0;
};

#endif
