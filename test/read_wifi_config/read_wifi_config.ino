#include <EEPROM.h>

#define START_WIFI_MACS         1918
#define EEPROM_MAX 2047

char* SSID;
char* pw;
uint8_t chk;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_MAX + 1);
  get_wifi_config(SSID, pw);
}

void loop() {
}

// read data from EEPROM, check them and set them if the check is passed
bool get_wifi_config(char *_SSID, char *_pw) {
  Serial.println("get wifi config!!");
  uint16_t data_start = START_WIFI_MACS;

  uint8_t SSID[21];
  uint8_t pw[22];
  uint8_t chk = 0x00;
  uint8_t read_char = 0x00;
  uint8_t p = 0x00;

  // read ssid
  bool all_FF = true;
  read_char = 0x01; // avoid instand stop
  for (uint8_t i = 0; i < 20 && read_char != 0x00; i++) {
    read_char = EEPROM.read(data_start + i);
    SSID[i] = read_char;
    p = i;

    if (read_char != 0xFF) {
      all_FF = false;
    }
  }

  SSID[p + 1] = 0x00;

  // read pw
  read_char = 0x01; // avoid instand stop
  for (uint8_t i = 0; i < 21 && read_char != 0x00; i++) {
    read_char = EEPROM.read(data_start + i + 20);
    pw[i] = read_char;
    p = i;
  }
  pw[p + 1] = 0x00;

  chk = EEPROM.read(data_start + 41);


  // a bug in the system can erase all EEPROM info.
  // it is connected to a brown out situation
  // in this case the hole eeprom page is 0xFF
  // the only thing we can do is to return the default
  // wifi config, which we'll do below
  //TODO : check what happen when there is a problem with EEPROM and set default WiFi config.
  /*if (all_FF) {
    //#ifdef DEBUG_JKW_WIFI
    //Serial.println("invalid wifi data FF");
    //#endif
    Serial.println("invalid wifi data FF");
    //return false;

    memset(SSID, 0x00, 21);
    memset(pw, 0x00, 21);

    if (id == WIFI_MACS) {
      memcpy(SSID, "macs", 4);
      memcpy(pw, "6215027094", 10);
      type = 3; // wpa2
      chk = 0x17;
    } else if (id == WIFI_UPDATE_1) {
      memcpy(SSID, "ajlokert", 8);
      memcpy(pw, "qweqweqwe", 9);
      type = 3; // wpa2
      chk = 0x60;
    } else if (id == WIFI_UPDATE_2) {
      memcpy(SSID, "shop", 4);
      memcpy(pw, "abcdefgh", 8);
      type = 2; // WPA
      chk = 0x0E;
    }
  }*/

  Serial.println("set wifi, data:");
  Serial.print("SSID:");
  Serial.print((const char*)SSID);
  Serial.println(".");
  delay(1000);
  Serial.print("PW:");
  Serial.print((const char*)pw);
  Serial.println(".");
  delay(1000);
  Serial.print("chk:");
  Serial.print(chk);
  Serial.println(".");
  delay(1000);
  if (!check_wifi_config((const char*)SSID, (const char*)pw, chk)) {
    Serial.println("set wifi, data invalid");
    _SSID = "";
    _pw = "";

    return false;
  }

  _SSID = (char*)SSID;
  _pw = (char*)pw;

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
