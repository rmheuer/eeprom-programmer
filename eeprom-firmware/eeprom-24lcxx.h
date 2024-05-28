#ifndef INCLUDE_EEPROM_24LCXX_H
#define INCLUDE_EEPROM_24LCXX_H

#include "eeprom-io.h"

class EepromIo24LCxx : public EepromIo {
public:
  void setup() override;

  void init() override;

  uint8_t readByte(uint16_t addr) override;

  void writeByte(uint16_t addr, uint8_t value) override;

  void disable() override;
private:
  bool wireActive;
};

#endif
