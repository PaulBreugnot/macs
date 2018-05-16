#include "config.h"
#include "Arduino.h"
#include "stdint.h"

bool set_wifi_login();
bool set_login(uint8_t mode);
bool get_wifi_config(uint8_t id, String *SSID, String *pw);
bool check_wifi_config(String SSID, String pw, uint8_t chk);
bool parse_wifi();
bool is_wifi_connected();
