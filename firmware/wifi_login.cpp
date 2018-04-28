
#include "stdint.h"
#include "config.h"
#include "EEPROM.h"

class FindSSID
{
    char *SSID_to_search;
    bool found;

  public:
    /**
       Scan WiFi Access Points and retrieve the strongest one.
    */
    bool check_SSID_in_range(char *SSID)
    {
#ifdef DEBUG_JKW_WIFI
      //Serial.print("check SSID for ");
      //Serial.println(SSID);
#endif
      // initialize data
      found = false;
      SSID_to_search = SSID;

      // avoid scanning for invaid data
      if (strlen(SSID) == 0) {
        return false;
      }
      
      byte numSSID = WiFi.scanNetworks();
      for (int thisNet = 0; thisNet < numSSID; thisNet++) {
        if (strcmp(WiFi.SSID(thisNet).c_str(), SSID_to_search) == 0) {
#ifdef DEBUG_JKW_WIFI
        Serial.print("found ");
          Serial.println(SSID_to_search);
          delay(100);
#endif
          found = true;
        }
      }
      // perform the scan#
#ifdef DEBUG_JKW_WIFI
      //Serial.println("EOS!!!!");
#endif
      return found;
    }
};

bool set_wifi_login() {
  //  ... ok ... complicated
  //
  //  if we have a blank device this will happen:
  //  1. Both LEDs will toggle for 10sec (saving WiFi credentials (not applicable here) or waiting for input)
  //
  //  if we have a config that is OUT of reach:
  //  1. (MACS=Both LEDs)/(UPDATE1=green LED)/(UPDATE2=red LED) will flash 3x (MACS=simultaneously) to show that the config has been read
  //  2. Green off, red on will show the start of the WiFi scanning
  //  3. per WiFi that has been found the green and red will toggle, just to show activity
  //  4. Both LEDs are switched off
  //  5. Both LEDs will toggle for 10sec (saving WiFi credentials (not applicable here) or waiting for input) 20Hz
  //
  //  if we have a config that is IN reach:
  //  1. (MACS=Both LEDs)/(UPDATE1=green LED)/(UPDATE2=red LED) will flash 3x (MACS=simultaneously) to show that the config has been read
  //  2. Green off, red on will show the start of the WiFi scanning
  //  3. per WiFi that has been found the green and red will toggle, just to show activity
  //  4. Both LEDs are switched off
  //  5. (MACS=Both LEDs)/(UPDATE1=green LED)/(UPDATE2=red LED) will toggle 5x (WiFi found) 10Hz
  //  6. Both LEDs will toggle for 10sec or until WiFi data are saved (saving WiFi credentials or waiting for input) 20Hz
  //  7. (MACS=Both LEDs)/(UPDATE1=green LED)/(UPDATE2=red LED) will toggle 2x (WiFi connected) 10Hz
  //

  char* SSID;
  char* pw;
  bool try_backup = true;
  uint8_t wifi_offset;
  uint8_t max_loop;
  FindSSID ssidFinder;
  uint8_t config;
  //EEPROM.begin(512);

  WiFi.disconnect();
    if (get_wifi_config(wifi_offset, SSID, pw)) {
      Serial.println("SSID found in EEPROM");
      delay(1000);

      if (ssidFinder.check_SSID_in_range(SSID)) {
        Serial.println("Wifi Found");
      };
      delay(1000);
    };
  for (int i = 0; i < 200 && (WiFi.SSID().length() == 0); i++) {
    // take new info
    parse_wifi();
    // set info
    delay(50);
    }

  if (SSID != "") {
    // set IP
      IPAddress myAddress(192, 168, 188, 100 + get_my_id());
      IPAddress netmask(255, 255, 255, 0);
      IPAddress gateway(192, 168, 188, 254);
      IPAddress dns(192, 168, 188, 254);
      WiFi.config(myAddress, dns, gateway, netmask);

    // finally connect
    if(pw != ""){
    WiFi.begin(SSID, pw);
    }
    else{
      //Open Network
      WiFi.begin(SSID);
    }
    int i = 0;
    while (i < 200 && WiFi.status() != WL_CONNECTED) {
      delay(50); // wait 59
      i++;
    }

    return true;
  }
  return false;
}

bool is_wifi_connected() {
  return WiFi.status() == WL_CONNECTED;
};


// read data from EEPROM, check them and set them if the check is passed
bool get_wifi_config(uint8_t id, char *_SSID, char *_pw) {
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

  //#ifdef DEBUG_JKW_WIFI
  //Serial.println("set wifi, data:");
  //Serial.print("SSID:");
  //Serial.print((const char*)SSID);
  //Serial.println(".");
  //delay(1000);
  //Serial.print("PW:");
  //Serial.print((const char*)pw);
  //Serial.println(".");
  //delay(1000);
  //Serial.print("type:");
  //Serial.print(type+'0');
  //Serial.println(".");
  //delay(1000);
  //Serial.print("chk:");
  //Serial.print(chk);
  //Serial.println(".");
  //delay(1000);
  //#endif
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

    //#ifdef DEBUG_JKW_WIFI
    //Serial.println("set wifi, data invalid");
    //#endif
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

#ifdef DEBUG_JKW_WIFI
    Serial.println("check wifi data, data invalid");
#endif

    return false;
  }

#ifdef DEBUG_JKW_WIFI
  //Serial.println("Checksum ok");
#endif

  return true;
}

// save the wifi data to eeprom
bool save_wifi_config(uint8_t id, String SSID, String pw, uint8_t chk) {

#ifdef DEBUG_JKW_WIFI
  Serial.println("save wifi, data:");
  Serial.print("SSID:");
  Serial.print(SSID);
  Serial.println(".");
  Serial.print("PW:");
  Serial.print(pw);
  Serial.println(".");
  Serial.print("chk:");
  Serial.print(chk);
  Serial.println(".");
  delay(1000);
#endif
  Serial.println("set wifi config!!");

  uint16_t data_start = START_WIFI_MACS;

  // check if the submitted data are valid
  if (!check_wifi_config(SSID, pw, chk)) {
#ifdef DEBUG_JKW_WIFI
    Serial.println("save wifi, not valid");
#endif
    return false;
  }

  // length check
  if (SSID.length() > 20 || pw.length() > 21) {
#ifdef DEBUG_JKW_WIFI
    Serial.println("save wifi, data to long");
#endif
    return false;
  }

  //save the data
  // ssid
  for (uint8_t i = 0; i < SSID.length() && i < 20; i++) {
    EEPROM.write(data_start + i + 0, SSID.charAt(i));
  }
  if (SSID.length() < 20) {
    EEPROM.write(data_start + SSID.length() + 0, 0x00);
  }
  // pw
  for (uint8_t i = 0; i < pw.length() && i < 21; i++) {
    EEPROM.write(data_start + i + 20, pw.charAt(i));
  }
  if (pw.length() < 21) {
    EEPROM.write(data_start + pw.length() + 20, 0x00);
  }
  // checksum
  EEPROM.write(data_start + 41, chk);
  EEPROM.commit();
  
  Serial.println("done!!");
  return true;
}

// read config from serial port for wifi
bool parse_wifi() {
  if (!Serial.available()) {
    return false;
  }

  uint8_t in = 0x00;
  uint8_t tab_count = 0;
  uint8_t SSID[20];
  uint8_t pw[21];
  uint8_t chk = 0x00;
  uint8_t id = 0x00;
  uint8_t p = 0x00;


  // buffer is 64 byte (id(1)+<tab>+SSID(20)+<tab>+pw(20)+<tab>+type(1)+<tab>+chk(1)+<tab>)=48
  // e.g. 00 09 6d 61 63 73 09 36 32 31 35 30 32 37 30 39 34 09 03 09 17 09
  // e.g. 01 09 61 6a 6c 6f 6b 65 72 74 09 71 77 65 71 77 65 71 77 65 09 03 09 60 09
  // e.g. 02 09 73 68 6f 70 09 61 62 63 64 65 66 67 68 09 02 09 0E 09
  //Serial.print("available:");
  //Serial.println(Serial.available());
  delay(1000); // give buffer time to fill

  while (Serial.available()) {
    in = Serial.read();
    //Serial.print("read ");
    //Serial.print(in);
    //Serial.println(".");


    if (in == 0x09) { // which is tab, our delimitter
      if (tab_count == 1) {
        SSID[p] = 0x00;
      } else if (tab_count == 2) {
        pw[p] = 0x00;
      }

      p = 0x00;
      tab_count++;
      //Serial.print("tab is now ");
      //Serial.println(tab_count);


      // to identify if a macs unit is present i'll send a "i<tab>" and the unit will responde "MACS"
      if (tab_count == 1 && id == 'i') {
        tab_count = 0;
        Serial.println("MACS");
      }

      if (tab_count == 5) {
        //Serial.println("try to save");
        if (save_wifi_config(id, (const char*)SSID, (const char*)pw, chk)) {
          Serial.println("saved");
          return true;
        } else {
          Serial.println("error");
          return false;
        }
        tab_count = 0;
      }
    } else if (tab_count == 0) {
      id = in;
    } else if (tab_count == 1) {
      SSID[p] = in;
      if (p < 20) {
        p++;
      };
    } else if (tab_count == 2) {
      pw[p] = in;
      if (p < 20) {

        p++;
      };
    } else if (tab_count == 3) {
      chk = in;
    }

  }
  return false;
  //Serial.println("while end");
}

