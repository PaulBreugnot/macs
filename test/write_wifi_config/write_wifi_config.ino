/*
 * EEPROM Write
 *
 * Stores values read from analog input 0 into the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later by another sketch.
 */

#include <EEPROM.h>

#define START_WIFI_MACS         1918
#define EEPROM_MAX 2047

String SSID = "openfactory42";
String pw = "openfactory.42";
uint8_t chk;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_MAX + 1);
  set_chk();
  save_wifi_config(SSID, pw, chk);
}

void loop() {
}

void set_chk(){
  uint8_t checksum = 0x00;
  for (uint8_t i = 0; i < SSID.length(); i++) {
    checksum ^= SSID.charAt(i);
  }
  for (uint8_t i = 0; i < pw.length(); i++) {
    checksum ^= pw.charAt(i);
  }
  chk = checksum;
}

// save the wifi data to eeprom
bool save_wifi_config(String SSID, String pw, uint8_t chk) {

  Serial.println("save wifi, data:");
  Serial.print("SSID:");
  Serial.print(SSID);
  Serial.println(".");
  Serial.print("PW:");
  Serial.print(pw);
  Serial.println(".");
  Serial.println(".");
  Serial.print("chk:");
  Serial.print(chk);
  Serial.println(".");
  delay(1000);
  
  Serial.println("set wifi config!!");

  uint16_t data_start = START_WIFI_MACS;

  // check if the submitted data are valid
  if (!check_wifi_config(SSID, pw, chk)) {
    Serial.println("save wifi, not valid");
    return false;
  }

  // length check
  if (SSID.length() > 20 || pw.length() > 21) {
    Serial.println("save wifi, data to long");
    return false;
  }

  //save the data
  // ssid
  for (uint8_t i = 0; i < SSID.length() && i < 20; i++) {
    EEPROM.write(data_start + i + 0, SSID.charAt(i));
  }
  for (uint8_t i = SSID.length(); i < 20; i++) {
    EEPROM.write(data_start + i + 0, 0x00);
  }
  // pw
  for (uint8_t i = 0; i < pw.length() && i < 21; i++) {
    EEPROM.write(data_start + i + 20, pw.charAt(i));
  }
  for(uint8_t i = pw.length(); i < 21; i++){
    EEPROM.write(data_start + i + 20, 0x00);
  }
  // checksum
  EEPROM.write(data_start + 41, chk);
  EEPROM.commit();
  
  Serial.println("done!!");
  return true;
}

// checks if the data from the given configuration is valid
bool check_wifi_config(String SSID, String pw, uint8_t chk) {
  uint8_t checksum = 0x00;
  for (uint8_t i = 0; i < SSID.length(); i++) {
    checksum ^= SSID.charAt(i);
  }
  for (uint8_t i = 0; i < pw.length(); i++) {
    checksum ^= pw.charAt(i);
  }

  if (checksum != chk) {

    Serial.println("check wifi data, data invalid");

    return false;
  }
  Serial.println("Checksum ok");

  return true;
}
