/*
 * EEPROM Write
 *
 * Stores values read from analog input 0 into the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later by another sketch.
 */

#include <EEPROM.h>

#define KEY_NUM_EEPROM_HIGH     KEY_NUM_EEPROM_LOW-1
#define KEY_NUM_EEPROM_LOW      KEY_CHECK_EEPROM_HIGH-1
#define KEY_CHECK_EEPROM_HIGH   KEY_CHECK_EEPROM_LOW-1
#define KEY_CHECK_EEPROM_LOW    EEPROM_MAX
#define EEPROM_MAX 2047

/** the current address in the EEPROM (i.e. which byte we're going to write to next) **/
  uint16_t num_keys = 0;
  uint16_t num_keys_check = 0;
int addr;
uint32_t tagToWrite = 0x1A44FD90;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_MAX + 1);
  set_addr();
  write_new_tag();
  update_numKeys();
  EEPROM.commit();
}

void loop() {
}

void set_addr(){
  uint8_t temp;

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

  addr = num_keys*4;
}

void write_new_tag(){
  Serial.print("Tag to write : ");
  Serial.println(tagToWrite, HEX);
  EEPROM.write(addr, (tagToWrite>>24)&0xFF);
  EEPROM.write(addr + 1, (tagToWrite>>16)&0xFF);
  EEPROM.write(addr + 2, (tagToWrite>>8)&0xFF);
  EEPROM.write(addr + 3, (tagToWrite)&0xFF);
  num_keys+=1;
}

void update_numKeys(){
  Serial.print("Num_keys : ");
  Serial.println(num_keys);
  Serial.print("write:");
  Serial.println(KEY_NUM_EEPROM_LOW);

  Serial.print("KEY_NUM_EEPROM_HIGH : ");Serial.println((num_keys >> 8) & 0xff, BIN);
  Serial.print("KEY_NUM_EEPROM_LOW : ");Serial.println((num_keys) & 0xff, BIN);
  EEPROM.write(KEY_NUM_EEPROM_HIGH, (num_keys >> 8) & 0xff);
  EEPROM.write(KEY_NUM_EEPROM_LOW, (num_keys) & 0xff);
  // checksum
  Serial.print("write:");
  Serial.println(KEY_CHECK_EEPROM_LOW);

  Serial.print("KEY_CHECK_EEPROM_HIGH : ");Serial.println(((num_keys + 1) >> 8) & 0xff, BIN);
  Serial.print("KEY_CHECK_EEPROM_LOW : ");Serial.println(((num_keys + 1)) & 0xff, BIN);
  EEPROM.write(KEY_CHECK_EEPROM_HIGH, ((num_keys + 1) >> 8) & 0xff);
  EEPROM.write(KEY_CHECK_EEPROM_LOW, ((num_keys + 1)) & 0xff);
  EEPROM.commit();
}

