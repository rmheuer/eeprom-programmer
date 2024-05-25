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

  Serial.begin(9600);
}

void loop() {
  // Wait for command
  while (!Serial.available()) {
    delay(1);
  }

  char command = Serial.read();
  uint8_t page = Serial.read();

  uint16_t pageAddr = page << 8;

  if (command == 'r') {
    for (int i = 0; i < 256; i++) {
      uint16_t addr = pageAddr + i;
      uint8_t value = read(addr);
      Serial.write(value);
    }
  } else if (command == 'w') {
    for (int i = 0; i < 256; i++) {
      uint16_t addr = pageAddr + i;
      uint8_t value = Serial.read();
      write(addr, value);
    }
    Serial.write(0xa5); // Ack
  }
}
