
#include "ESP8266WiFi.h"

#define WIFI_MACS               0
#define WIFI_UPDATE_1           1
#define WIFI_UPDATE_2           2
#define UPDATE                  1

uint8_t connected = 0;

void setup() {
  Serial.begin(9600);
  goto_update_mode();

}

void loop() {
  // put your main code here, to run repeatedly:

}

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
      Serial.print("check SSID for ");
      Serial.println(SSID);
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
          Serial.print("found ");
          Serial.println(SSID_to_search);
          delay(100);
          found = true;
        }
      }
      return found;
    }
};

void goto_update_mode() {
  connected = 0;
  // satrt loop that will set wifi data and connect to cloud,
  // and if anything fails start again, until there is an update
  while (1) {
    // set_update_login will return true, if we've read a valid config from
    // the EEPROM memory AND that WIFI was in range AND the module has saved the login
    if (set_update_login()) {
      Serial.println("set update login done");
      uint8_t i = 0;

      // backup, if connect didn't work, repeat it
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
      }

      // stay in update mode forever
      while (WiFi.status() == WL_CONNECTED) {
        if (i != millis() / 1000) {

          Serial.print(i);
          Serial.print(": ");

          // as soon as we are connected, swtich to blink mode to make it visible
          if (!connected) {
            connected = 1;
          }

          // check incomming data, unlikely here, because at this point we are already connected to an update wifi
          //parse_wifi();

          Serial.println("NodeMCU connected");
          i = millis() / 1000;
        } // i!=millis()/1000
        delay(200); // don't go to high as blink will look odd
      } // end while(WiFi.ready())
      // reaching this point tells us that we've set the wifi login, tried to connect but lost the connection, as the wifi is not (longer) ready
    } // if(set_update_login())
  } // end while(1)
  // ############ UPDATE MODUS ############ //
}

bool set_update_login() {
  return set_login(UPDATE);
}

bool set_login(uint8_t mode) {
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

char* SSID = "Livebox-4cac";
char* pw = "9265A6797F9378E2EE74F47278";
  int type;
  bool try_backup = true;
  uint8_t wifi_offset;
  uint8_t max_loop;
  FindSSID ssidFinder;
  uint8_t config;
  //EEPROM.begin(512);

  WiFi.disconnect();

  // prepare loop
  if (mode == UPDATE) { // in update mode we'll try both configs, WIFI_UPDATE_1 and WIFI_UPDATE_2
    max_loop = 2;
    wifi_offset = WIFI_UPDATE_1;
  } else {  // in macs mode we'll just try WIFI_MACS
    max_loop = 1;
    wifi_offset = WIFI_MACS;
  }

  for (config = 0; config < max_loop; config++) {
    //if (get_wifi_config(wifi_offset + config, &SSID, &pw, &type)) {
    if (1) {
      //Serial.println("SSID found in EEPROM");
      delay(1000);

      if (ssidFinder.check_SSID_in_range(SSID)) {
        Serial.println("Wifi Found");

        //Here : only set wifi confid. Useless with NodeMCU
        //WiFi.begin(SSID, pw, type);
        //TODO : set security type and key
        //WiFi.softAP(SSID_char); //for open network
        break;
      };
      delay(1000);
    };
  };
  /*for (int i = 0; i < 200 && (WiFi.SSID().length() == 0); i++) {
    // take new info
    parse_wifi();
    // set info
    delay(50);
    }*/

  if (SSID != "") {
    // set IP
    if (mode != UPDATE) {
      /*IPAddress myAddress(192, 168, 188, 100 + get_my_id());
        IPAddress netmask(255, 255, 255, 0);
        IPAddress gateway(192, 168, 188, 254);
        IPAddress dns(192, 168, 188, 254);
        WiFi.config(myAddress, dns, gateway, netmask);*/
    }

    // finally connect

    WiFi.begin(SSID, pw);
    int i = 0;
    while (i < 200 && WiFi.status() != WL_CONNECTED) {
      delay(50); // wait 59
      i++;
    }

    return true;
  }
  return false;
}


