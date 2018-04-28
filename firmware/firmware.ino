/* LED Patter
  === STARTUP ===
  red,green,red,green  -> i give you 10 sec to connect to me, before I start
  red on, green on -> I'm trying to connect to my server

  === UPDATE MODE ===
  green flashes 3 times (10Hz) -> I've found valid WiFi config 1 in EEPROM and now I'm scannning for it!
  green flashes 5 times (10Hz) -> I've found WiFi config 1 in scan results and try to conenct now!
  red flashes 3 times (10Hz) -> I've found valid WiFi config 2 in EEPROM and now I'm scannning for it!
  red flashes 5 times (10Hz) -> I've found WiFi config 2 in scan results and try to conenct now!
  red and green simulatnious, 2 fast blinks (10Hz) -> connected to wifi!
  red,green,red,greed fast (10sec, 20Hz) -> Waiting for the Wifi to save credentials AND give you time to add new credentials via serial
  red and green blink simultainous -> I'm ready for an update

  === MACS MODE (status can be combined) ===
  red and green simulatnious, flashes 3 times (10Hz) -> I've found valid WiFi config in EEPROM and now I'm scannning for it!
  red and green simulatnious, flashes 5 times (10Hz) -> I've found WiFi config in scan results and try to conenct now!
  red and green simulatnious, 2 fast flashes (10Hz)  -> connected to wifi!
  red blinking -> no connection to the MACS Server
  red solid -> card rejected
  green blinking -> connected to the MACS Server
  green solid -> card accepted
*/

#include "stdint.h"
#include "config.h"
#include "EEPROM.h"
#include "SPI.h"
#include "MFRC522.h"

// network
IPAddress HOSTNAME(192, 168, 188, 23);
uint32_t v = 20160214;

uint8_t keys_available = 0;
uint32_t keys[MAX_KEYS];

uint8_t currentTagBuf[TAGSTRINGSIZE];
uint8_t currentTagIndex = 0;
uint8_t connected = 0;
uint32_t currentTag = -1;
boolean cardPresent = false;

uint8_t current_relay_state = RELAY_DISCONNECTED;
uint8_t id = -1; //255, my own id
uint32_t last_key_update = 0;
uint32_t last_server_request = 0;
uint32_t relay_open_timestamp = 0;
uint32_t last_tag_read = 0;

BACKUP b; // report backup

// http server
HttpClient http; // Headers currently need to be set at init, useful for API keys etc.
http_header_t headers[] = {     { "Accept" , "*/*"},     { NULL, NULL } }; // NOTE: Always terminate headers will NULL
http_request_t request;
http_response_t response;

// RFID reader

MFRC522 rfid(SS_PIN, RST_PIN);   // Create MFRC522 instance.

// LEDs
LED green_led(GREEN_LED);
LED red_led(RED_LED);

//////////////////////////////// SETUP ////////////////////////////////
void setup() {
  // set adress pins
  //for (uint8_t i = 10; i <= MAX_JUMPER_PIN + 10; i++) { // A0..7 is 10..17, used to read my ID
  //  pinMode(i, INPUT_PULLUP);
  //}
  pinMode(RELAY_PIN, OUTPUT);         // driver for the relay

  Serial.begin(9600);
  EEPROM.begin(EEPROM_MAX + 1);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 card
  //rfid.PCD_SetAntennaGain(0x07 << 4 );

  // setup https client
  request.ip = HOSTNAME;
  request.port = HOSTPORT;

    // ############ MACS MODUS ############ //
#ifdef DEBUG_JKW_MAIN
    Serial.println("- MACS -");
#endif

    set_connected(0); // init as not connected
    /*if (update_ids(true)) { // true = force update, update_ids will initiate the connect
      set_connected(1);
    } else {
      set_connected(0, true); // force LED update for not connected
      read_EEPROM();
    }*/
    //For now, we only initialize keys from EEPROM
    read_EEPROM();
    // ############ MACS MODUS ############ //
}
//////////////////////////////// SETUP ////////////////////////////////


//////////////////////////////// MAIN LOOP ////////////////////////////////
// woop woop main loop
void loop() {
  // check if we found a tag
  if (checkCard(currentTagBuf, &currentTag)) {
    // if we found a tag, test it
    // if it works close relay,
    // if not - ask the server for an update and try again
    uint8_t tries = 2; // two tries to check the card
    while (tries) {
      // compares known keys, returns true if key is known
      if (access_test(currentTag)) {
        relay(RELAY_CONNECTED);
        green_led.on();
        tries = 0;
        // takes long
        //create_report(LOG_RELAY_CONNECTED, currentTag, 0); //Not used for now
        // 1. assuming that we are NOT connected, we reached this point and the create_report has success to reconnet than it will call set_connected()
        // this will turn red off (which is fine (was blinking = not connected)) and green to blink (ok), so we have to override it
        // 2. assuming that we are NOT connected, we reached this point and the create_report has NO success to reconnet than it will not call set_connected()
        // the red will keep blinking (ok) but we still want to show that this card was good, turn green on
        // 3. assuming that we are connected, we reached this point then create_report will not try to reconnect and the report is send just fine
        // the red will be off anywa (ok), we want to show that this card was good, turn green on
        // 4. assuming that we are connected, we reached this point then create_report will not try to reconnect, but the report failed, create_report will set us to not conneted
        // the red will be blinkin (ok), we want to show that this card was good, turn green on
        set_connected(connected, 1); // force to resume LED pattern
      } else {
        // if we have a card that is not known to be valid we should maybe check our database
        if (tries > 1) {

#ifdef DEBUG_JKW_MAIN
          Serial.println("Key not valid, requesting update from server");
#endif
          //Not used for now
          //update_ids(false); // unforced update


#ifdef DEBUG_JKW_MAIN
          if (tries > 0) {
            Serial.println("Trying once more if key is valid now");
          };
#endif

          tries -= 1;
        } else {

#ifdef DEBUG_JKW_MAIN
          Serial.println("key still not valid. :P");
#endif

          tries = 0;
          // takes long
          //create_report(LOG_LOGIN_REJECTED, currentTag, 0); //Not used for now
          red_led.on();
        }
      }
    }
  }


  // card moved away
  if (!cardPresent) {
    // open the relay as soon as the tag is gone
    if (current_relay_state == RELAY_CONNECTED) {
      uint32_t open_time_sec = relay(RELAY_DISCONNECTED);
      // last because it takes long
      //create_report(LOG_RELAY_DISCONNECTED, currentTag, open_time_sec); //Not used for now
    }

    set_connected(connected, 1); // force to resume LED pattern

    currentTag = -1;    // reset current user
    currentTagIndex = 0; // reset index counter for incoming bytes

  }

  // time based update the storage from the server (every 10 min?)
  if (last_key_update + DB_UPDATE_TIME < (millis() / 1000)) {
    //update_ids(false);  // unforced upate //Not used for now
  }
  
  //parse_wifi(); //Not used for now
 }
//////////////////////////////// MAIN LOOP ////////////////////////////////


//////////////////////////////// ACCESS TEST ////////////////////////////////
// callen from main loop as soon as a tag has been found to test if it matches one of the saved keys
bool access_test(uint32_t tag) {
#ifdef DEBUG_JKW_MAIN
  Serial.print("Tag ");
  Serial.print(tag);
  Serial.print(" found. Checking database (");
  Serial.print(keys_available);
  Serial.print(") for matching key");
  Serial.println("==============");
#endif

  for (uint16_t i = 0; i < MAX_KEYS && i < keys_available; i++) {

#ifdef DEBUG_JKW_MAIN
    Serial.print(i + 1);
    Serial.print(" / ");
    Serial.print(keys_available);
    Serial.print(" Compare current read tag ");
    Serial.print(tag);
    Serial.print(" to stored key ");
    Serial.print(keys[i]);
    Serial.println("");
#endif

    if (keys[i] == tag) {

#ifdef DEBUG_JKW_MAIN
      Serial.println("Key valid, closing relay");
#endif

      return true;
    }
  }

#ifdef DEBUG_JKW_MAIN
  Serial.println("==============");
#endif

  return false;
}
//////////////////////////////// ACCESS TEST ////////////////////////////////

//////////////////////////////// DRIVE THE RELAY ////////////////////////////////
// hardware controll, writing to the pin and log times
uint32_t relay(int8_t input) {
  if (input == 1) {

#ifdef DEBUG_JKW_MAIN
    Serial.println("Connecting relay!");
#endif

    digitalWrite(RELAY_PIN, HIGH);
    current_relay_state = RELAY_CONNECTED;
    relay_open_timestamp = millis() / 1000;
  } else {

#ifdef DEBUG_JKW_MAIN
    Serial.println("Disconnecting relay!");
#endif

    digitalWrite(RELAY_PIN, LOW);
    current_relay_state = RELAY_DISCONNECTED;
    return ((millis() / 1000) - relay_open_timestamp);
  }
}
//////////////////////////////// DRIVE THE RELAY ////////////////////////////////

//////////////////////////////// SCAN FOR TAG ON SERIAL ////////////////////////////////
// returns true if tag found, does the UART handling
bool checkCard(uint8_t *buf, uint32_t *tag) {
  boolean validTag;
  
  //Return true if a new card has been read
  //Return false if no card or same card.
  boolean isNewCardPresent0 = rfid.PICC_IsNewCardPresent();
  
  //if isNewCardPresent0 is true : it is sure that there is a new card.
  if (isNewCardPresent0) {
    validTag = readCard(buf, tag);
    Serial.println("New card");
    return cardPresent && validTag;
  }
  else {
    // if isNewCardPresent0 is false AND the second call to rfid.PICC_IsNewCardPresent() is false : card has been removed
    if ( cardPresent && ! rfid.PICC_IsNewCardPresent()) {
      green_led.off();
      red_led.off();
      Serial.println("Card Removed");
      cardPresent = false;
      return false;
    }
    if (!cardPresent) {
      // Even if isNewCardPresent0 is false, there can be a new card
      boolean isNewCardPresent = rfid.PICC_IsNewCardPresent();
      if ( ! isNewCardPresent) {
        cardPresent = false;
        return false;
      }
      else {
        validTag = readCard(buf, tag);
        Serial.println("New card");
        return cardPresent && validTag;
      }
    }
  }
  return false;
}

boolean readCard(uint8_t *buf, uint32_t *tag) {
  boolean readCardSerial = rfid.PICC_ReadCardSerial();
  
  // Verify if the UID has been readed
  if ( ! readCardSerial) {
    return false;
  }
  cardPresent = true;
  for (byte i = 0; i < 4; i++) {
    buf[i] = rfid.uid.uidByte[i];
  }
  return validate_tag(buf, tag);
}

//////////////////////////////// SCAN FOR TAG ON SERIAL ////////////////////////////////

//////////////////////////////// VERIFY CHECKSUM FOR TAG ////////////////////////////////
// just check if the data are corrumpeted or equal the checksum
// and convert them to the correct oriented unsigned long
bool validate_tag(uint8_t *buf, uint32_t *tag) {
  /*Serial.println("Validating tag...");
  uint8_t expected = 0;
  for (uint8_t i = 0; i < TAGSTRINGSIZE - 1; i++) {
    expected ^= buf[i];
  }

  if (expected == buf[TAGSTRINGSIZE - 1]) {
    // checksum correct, flip data around to get the uint32_t
    for (uint8_t i = 0; i < TAGSTRINGSIZE - 1; i++) {
      *tag = (*tag << 8) + buf[i];
    };
    Serial.println("Valid tag.");
    return true;
  }
  Serial.println("Wrong tag.");
  return false;*/

  //For now :
  for (uint8_t i = 0; i < TAGSTRINGSIZE ; i++) {
      *tag = (*tag << 8) + buf[i];
  };
  return true;
}
//////////////////////////////// VERIFY CHECKSUM FOR TAG ////////////////////////////////

//////////////////////////////// READ ID's FROM EEPROM ////////////////////////////////
// if we are running offline
bool read_EEPROM() {

#ifdef DEBUG_JKW_MAIN
  Serial.println("-- This is EEPROM read --");
#endif

  uint8_t temp;
  uint16_t num_keys = 0;
  uint16_t num_keys_check = 0;

  temp = EEPROM.read(KEY_NUM_EEPROM_HIGH);
  num_keys = temp << 8;
  temp = EEPROM.read(KEY_NUM_EEPROM_LOW);
  num_keys += temp;



#ifdef DEBUG_JKW_MAIN
  Serial.print("# of keys =");
  Serial.println(num_keys);
#endif


  temp = EEPROM.read(KEY_CHECK_EEPROM_HIGH);
  num_keys_check = temp << 8;
  temp = EEPROM.read(KEY_CHECK_EEPROM_LOW);
  num_keys_check += temp;

#ifdef DEBUG_JKW_MAIN
  Serial.print("# of keys+1 =");
  Serial.println(num_keys_check);
#endif

  if (num_keys_check == num_keys + 1) {
    keys_available = num_keys;
    for (uint16_t i = 0; i < num_keys; i++) {
      temp = EEPROM.read(i * 4 + 0);
      keys[i] = temp << 24;
      temp = EEPROM.read(i * 4 + 1);
      keys[i] += temp << 16;
      temp = EEPROM.read(i * 4 + 2);
      keys[i] += temp << 8;
      temp = EEPROM.read(i * 4 + 3);
      keys[i] += temp;

#ifdef DEBUG_JKW_MAIN
      Serial.print("Read key ");
      Serial.print(i);
      Serial.print("=");
      Serial.print(keys[i]);
      Serial.println(" from eeprom");
#endif
    }
  }

#ifdef DEBUG_JKW_MAIN
  Serial.println("-- End of EEPROM read --");
#endif
}
//////////////////////////////// READ ID's FROM EEPROM ////////////////////////////////

//////////////////////////////// UPDATE ID's ////////////////////////////////
// sends a request to the amazon server, this server should be later changed to
// be the local Raspberry pi. It will call the get_my_id() function
// return true if http request was ok
// false if not - you might want to set a LED if it returns false
bool update_ids(bool forced) {
  // avoid flooding
  if (last_key_update + MIN_UPDATE_TIME > millis() / 1000 && last_key_update > 0) {

#ifdef DEBUG_JKW_MAIN
    Serial.println("db read blocked, too frequent");
#endif

    return false;
  }

  if (!is_wifi_connected()) {

#ifdef DEBUG_JKW_MAIN
    Serial.println("no ping");
#endif

    if (!set_wifi_login()) {
      set_connected(0, true);
      return false;
    }
  }

  last_key_update = millis() / 1000;

  // request data
  uint32_t now = millis();
  request.path = "/m2m.php?v=" + String(v) + "&mach_nr=" + String(get_my_id()) + "&forced=";
  if (forced) {
    request.path = request.path + "1";
  } else {
    request.path = request.path + "0";
  }

  // Get request
  http.get(request, response, headers);
  int statusCode = response.status;

#ifdef DEBUG_JKW_MAIN
  Serial.print("db request took ");
  Serial.print(millis() - now);
  Serial.println(" ms");
  delay(1000);
  Serial.println("Requested:");
  Serial.println(request.path);
  delay(1000);
  Serial.println("Recevied:");
  Serial.println(response.body);
#endif


  // check status
  if (statusCode != 200) {
    set_connected(0);

#ifdef DEBUG_JKW_MAIN
    Serial.println("No response from server");
#endif

    Serial.println("No response from server");

    return false;
  }

  // check length
  if (response.body.length() == 0) {

#ifdef DEBUG_JKW_MAIN
    Serial.println("Empty response");
#endif
  }

  // connection looks good
  set_connected(1, true); // force update LEDs as the reconnect might have overwritten the LED mode

  // check if we've received a "no update" message from the server
  // if we are unforced we'll just leave our EEPROM as is.
  // otherweise we'll go on
  //Serial.println("response length:");
  //Serial.println(response.length());
  //Serial.print(response.charAt(0));
  //Serial.println(response.charAt(1));

  if (!forced && response.body.length() >= 2) {
    if (response.body.charAt(0) == 'n' && response.body.charAt(1) == 'u') {
      // we received a 'no update'

#ifdef DEBUG_JKW_MAIN
      Serial.println("No update received");
#endif
      b.try_fire();
      return true;
    }
  }

  // clear all existing keys and then, import keys from request
  keys_available = 0;
  uint16_t current_key = 0;
  for (uint16_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
    keys[i] = 0;
  }

  for (uint16_t i = 0; i < response.body.length(); i++) {
    Serial.print(response.body.charAt(i));

    if (response.body.charAt(i) == ',') {
      if (current_key < MAX_KEYS) {
        Serial.print("write:");
        Serial.println(current_key * 4 + 3);
        // store to EEPROM
        EEPROM.write(current_key * 4 + 0, (keys[current_key] >> 24) & 0xff);
        EEPROM.write(current_key * 4 + 1, (keys[current_key] >> 16) & 0xff);
        EEPROM.write(current_key * 4 + 2, (keys[current_key] >> 8) & 0xff);
        EEPROM.write(current_key * 4 + 3, (keys[current_key]) & 0xff);

        current_key++;
      }
      EEPROM.commit();
    } else if (response.body.charAt(i) >= '0' && response.body.charAt(i) <= '9') { // zahl
      keys[current_key] = keys[current_key] * 10 + (response.body.charAt(i) - '0');
    }
  }
  keys_available = current_key;


  // log number of keys to the eeprom
  Serial.print("write:");
  Serial.println(KEY_NUM_EEPROM_LOW);

  EEPROM.write(KEY_NUM_EEPROM_HIGH, (keys_available >> 8) & 0xff);
  EEPROM.write(KEY_NUM_EEPROM_LOW, (keys_available) & 0xff);
  // checksum
  Serial.print("write:");
  Serial.println(KEY_CHECK_EEPROM_LOW);
  EEPROM.write(KEY_CHECK_EEPROM_HIGH, ((keys_available + 1) >> 8) & 0xff);
  EEPROM.write(KEY_CHECK_EEPROM_LOW, ((keys_available + 1)) & 0xff);
  EEPROM.commit();

#ifdef DEBUG_JKW_MAIN
  Serial.print("Total received keys for my id(");
  Serial.print(get_my_id());
  Serial.print("):");
  Serial.println(keys_available);
  for (uint16_t i = 0; i < keys_available; i++) {
    Serial.print("Valid Database Key Nr ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(keys[i]);
    Serial.println("");
  };
#endif

  b.try_fire(); // try to submit old reports
  return true;
}
//////////////////////////////// UPDATE ID's ////////////////////////////////

//////////////////////////////// CREATE REPORT ////////////////////////////////
// create a log entry on the server for the action performed
void create_report(uint8_t event, uint32_t badge, uint32_t extrainfo) {
  if (!fire_report(event, badge, extrainfo)) {
    b.add(event, badge, extrainfo);
  } else {
    b.try_fire();
  }
}

bool fire_report(uint8_t event, uint32_t badge, uint32_t extrainfo) {
  bool ret = true;

  if (!is_wifi_connected()) {

#ifdef DEBUG_JKW_MAIN
    Serial.println("no ping");
#endif

    if (set_wifi_login()) {
      set_connected(1); // this could potentially destroy our LED pattern? TODO
    } else {
      set_connected(0);
      return false; // pointless to go on
    }
  }

  if (event == LOG_RELAY_CONNECTED) {
    request.path = "/history.php?logme&badge=" + String(badge) + "&mach_nr=" + String(get_my_id()) + "&event=Unlocked";
  } else if (event == LOG_LOGIN_REJECTED) {
    request.path = "/history.php?logme&badge=" + String(badge) + "&mach_nr=" + String(get_my_id()) + "&event=Rejected";
  } else if (event == LOG_RELAY_DISCONNECTED) {
    request.path = "/history.php?logme&badge=" + String(badge) + "&mach_nr=" + String(get_my_id()) + "&event=Locked&timeopen=" + String(extrainfo);
  } else if (event == LOG_NOTHING) {
    request.path = "/history.php";
  } else {
    return false;
  }

#ifdef DEBUG_JKW_MAIN
  Serial.print("calling:");
  Serial.println(request.path);
#endif

  uint32_t now = millis();

  http.get(request, response, headers);
  int statusCode = response.status;

  if (statusCode == 200) {
    set_connected(1);
  } else if (statusCode != 200) {
    set_connected(0);
    ret = false;
  }

#ifdef DEBUG_JKW_MAIN
  Serial.print("db request took ");
  Serial.print(millis() - now);
  Serial.println(" ms");
#endif
  return ret;
}
//////////////////////////////// CREATE REPORT ////////////////////////////////

//////////////////////////////// SET CONNECTED ////////////////////////////////
// set the LED pattern
void set_connected(int status) {
  set_connected(status, false);
};

void set_connected(int status, bool force) {
  if (status == 1 && (connected == 0 || force)) {
    connected = 1;
  } else if (status == 0 && (connected == 1 || force)) {
    connected = 0;
  }
}

//////////////////////////////// SET CONNECTED ////////////////////////////////

//////////////////////////////// GET MY ID ////////////////////////////////
// shall later on read the device jumper and return that number
// will only do the interation with the pins once for performance
uint8_t get_my_id() {
  if (id == (uint8_t) - 1) {
    id = 0;

#ifdef DEBUG_JKW_MAIN
    Serial.print("ID never set, reading");
#endif

    for (uint8_t i = 10 + MAX_JUMPER_PIN; i >= 10; i--) { // A0..7 is 10..17
      id = id << 1;
      if (!digitalRead(i)) {
        id++;
      };
    }

#ifdef DEBUG_JKW_MAIN
    Serial.print(" id for this device as ");
    Serial.println(id);
#endif

  }
  return id;
}
//////////////////////////////// GET MY ID ////////////////////////////////


