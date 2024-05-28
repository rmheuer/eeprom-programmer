#ifndef INCLUDE_EEPROM_AT28C64_H
#define INCLUDE_EEPROM_AT28C64_H

#include "eeprom-io.h"

class EepromIoAT28C64 : public EepromIo {
public:
  void setup() override;

  void init() override;

  uint8_t readByte(uint16_t addr) override;

  void writeByte(uint16_t addr, uint8_t value) override;

  void disable() override;
};

#endif
