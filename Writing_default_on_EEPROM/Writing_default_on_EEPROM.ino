#include <EEPROM.h>

void setup() {
  Serial.begin(9600);
  byte temp_set = 15;
  byte delta_heat_ON = 3;
  byte delta_heat_OFF = 1;
  byte delta_cool_ON = 3;
  byte delta_cool_OFF = 1;

  byte hum_set = 60;
  byte delta_hum_ON = 3;
  byte delta_hum_OFF = 2;
  byte delta_deu_ON = 3;
  byte delta_deu_OFF = 2;

  Serial.println(F("Writing in the EEPROM..."));
  EEPROM.put(0, temp_set);
  EEPROM.put(1, delta_heat_ON);
  EEPROM.put(2, delta_heat_OFF);
  EEPROM.put(3, delta_cool_ON);
  EEPROM.put(4, delta_cool_OFF);

  EEPROM.put(5, hum_set);
  EEPROM.put(6, delta_hum_ON);
  EEPROM.put(7, delta_hum_OFF);
  EEPROM.put(8, delta_deu_ON);
  EEPROM.put(9, delta_deu_OFF);

  Serial.println(F("Done!"));
}

void loop() {
}
