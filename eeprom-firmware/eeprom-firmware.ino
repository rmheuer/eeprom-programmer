const uint8_t writeEnable = 20;
const uint8_t outputEnable = 21;
const uint8_t shiftClock = 11; // Also activity light - LED pin
const uint8_t shiftData = 10;
const uint8_t ioPins[8] = {
  12, 13, 14, 15, 16, 17, 18, 19
};

void setAddress(uint16_t address) {
  uint8_t lo = address & 0xFF;
  uint8_t hi = address >> 8;

  digitalWrite(shiftClock, LOW);
  shiftOut(shiftData, shiftClock, MSBFIRST, hi);
  shiftOut(shiftData, shiftClock, MSBFIRST, lo);
}

void setIOMode(uint8_t mode) {
  for (uint8_t i = 0; i < 8; i++) {
    pinMode(ioPins[i], mode);
  }
}

uint8_t read(uint16_t address) {
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

void write(uint16_t address, uint8_t value) {
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

void setup() {
  pinMode(writeEnable, OUTPUT);
  digitalWrite(writeEnable, HIGH); // Disable write

  pinMode(outputEnable, OUTPUT);
  digitalWrite(outputEnable, HIGH); // Disable read
  
  pinMode(shiftClock, OUTPUT);
  pinMode(shiftData, OUTPUT);

  for (uint8_t i = 0; i < 8; i++) {
    pinMode(ioPins[i], INPUT);
  }


  // Test
  Serial.begin(9600);
  delay(7000);

  Serial.println("Writing...");

  for (uint8_t i = 0; i < 16; i++) {
    write(i, i);
  }

  delay(1000);

  Serial.println("Reading...");
  for (uint16_t base = 0; base < 256 * 8; base += 16) {
    uint8_t data[16];
    for (uint16_t offset = 0; offset <= 15; offset += 1) {
      data[offset] = read(base + offset);
    }

    char buf[80];
    sprintf(buf, "%03x:  %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
            base, data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],
            data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]);

    Serial.println(buf);
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
