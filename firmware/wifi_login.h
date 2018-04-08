#include "config.h"
#include "Arduino.h"
#include "stdint.h"

bool set_update_login();
bool set_macs_login();
bool set_login(uint8_t mode);
bool get_wifi_config(uint8_t id, String *_SSID, String *_pw, int *_type);
bool check_wifi_config(String SSID,String pw,uint8_t type,uint8_t chk);
bool parse_wifi();
bool is_wifi_connected();
