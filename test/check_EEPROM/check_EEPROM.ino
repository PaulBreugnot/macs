#include "EEPROM.h"

#define MAX_KEYS 478 // max number of keys, total nr +1 = 479
#define KEY_NUM_EEPROM_HIGH     KEY_NUM_EEPROM_LOW-1
#define KEY_NUM_EEPROM_LOW      KEY_CHECK_EEPROM_HIGH-1
#define KEY_CHECK_EEPROM_HIGH   KEY_CHECK_EEPROM_LOW-1
#define KEY_CHECK_EEPROM_LOW    EEPROM_MAX
#define EEPROM_MAX 2047

uint32_t keys[MAX_KEYS];

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_MAX + 1);
  read_EEPROM();
}

void loop() {
}

bool read_EEPROM() {

  Serial.println("-- This is EEPROM read --");

  uint8_t temp;
  uint16_t num_keys = 0;
  uint16_t num_keys_check = 0;

  temp = EEPROM.read(KEY_NUM_EEPROM_HIGH);
  num_keys = temp << 8;
  temp = EEPROM.read(KEY_NUM_EEPROM_LOW);
  num_keys += temp;

  Serial.print("# of keys =");
  Serial.println(num_keys);

  temp = EEPROM.read(KEY_CHECK_EEPROM_HIGH);
  num_keys_check = temp << 8;
  temp = EEPROM.read(KEY_CHECK_EEPROM_LOW);
  num_keys_check += temp;

  Serial.print("# of keys+1 =");
  Serial.println(num_keys_check);

  if (num_keys_check == num_keys + 1) {
    for (uint16_t i = 0; i < num_keys; i++) {
      temp = EEPROM.read(i * 4 + 0);
      keys[i] = temp << 24;
      temp = EEPROM.read(i * 4 + 1);
      keys[i] += temp << 16;
      temp = EEPROM.read(i * 4 + 2);
      keys[i] += temp << 8;
      temp = EEPROM.read(i * 4 + 3);
      keys[i] += temp;

      Serial.print("Read key ");
      Serial.print(i);
      Serial.print("=");
      Serial.print(keys[i]);
      Serial.println(" from eeprom");
    }
  }

  Serial.println("-- End of EEPROM read --");
}
