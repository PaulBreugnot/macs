/*
 * EEPROM Clear
 *
 * Sets all of the bytes of the EEPROM to 0.
 * This example code is in the public domain.

 */

#include <EEPROM.h>

#define EEPROM_MAX 2047

void setup()
{
  EEPROM.begin(EEPROM_MAX + 1);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < EEPROM_MAX + 1; i++)
    EEPROM.write(i, 0);

  // turn the LED on when we're done
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  EEPROM.end();
}

void loop()
{
}
