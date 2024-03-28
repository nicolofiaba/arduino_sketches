// Sketch per funzionamento stagionatore
#include <Wire.h> /* I2C Library */
#include <BME280I2C.h> /* Temperature/Humidity sensor: Connect to SDA/SCL */
#include <U8g2lib.h> /* Oled display 128x64: Connect to SDA/SCL */
#include <EEPROM.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#define ON true
#define OFF false
#define RELAY_BUTTON_UMI 3
#define RELAY_AC_DEU 12
#define RELAY_AC_COOL 11
#define RELAY_AC_HEAT 10
#define RELAY_AC_UMI 9
#define RELAY_FAN_EXT 8
#define BUTTON_1 4
#define BUTTON_2 5
#define BUTTON_3 6
#define BUTTON_4 7
#define DAT 0
#define CLK 1
#define RST 2

float p, t, h;
// Define 4 state variables for the devices: 0 = "OFF", 1 = "ON"
bool umi_state  = 0;
bool deu_state  = 0;
bool cool_state  = 0;
bool heat_state  = 0;
bool fan_state = 0;
byte screen_state = 10;
// Set temperature and humidity: default values and deltas (They can be modified by the user, later on)

byte temp_set;
byte delta_heat_ON;
byte delta_heat_OFF;
byte delta_cool_ON;
byte delta_cool_OFF;
byte hum_set;
byte delta_hum_ON;
byte delta_hum_OFF;
byte delta_deu_ON;
byte delta_deu_OFF;

byte fan_start_hour_1;
byte fan_start_min_1;
byte fan_interval_min_1;

byte fan_start_hour_2;
byte fan_start_min_2;
byte fan_interval_min_2;

BME280I2C bme;
U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
ThreeWire myWire(DAT, CLK, RST); // DAT, CLK, RST pins
RtcDS1302<ThreeWire> Rtc(myWire);

struct arguments {
  float t_current;
  float h_current;
  byte t_set;
  byte h_set;
  byte dt_heat_on;
  byte dt_heat_off;
  byte dt_cool_on;
  byte dt_cool_off;
  byte dh_hum_on;
  byte dh_hum_off;
  byte dh_deu_on;
  byte dh_deu_off;
};

struct fan_arguments {
  byte start_1_hr;
  byte start_1_min;
  byte interval_1_min;
  byte start_2_hr;
  byte start_2_min;
  byte interval_2_min;
};

void setup() {
  pinMode(RELAY_BUTTON_UMI, OUTPUT);
  pinMode(RELAY_AC_UMI, OUTPUT);
  pinMode(RELAY_AC_DEU, OUTPUT);
  pinMode(RELAY_AC_COOL, OUTPUT);
  pinMode(RELAY_AC_HEAT, OUTPUT);
  pinMode(RELAY_FAN_EXT, OUTPUT);
  pinMode(BUTTON_1, INPUT);
  pinMode(BUTTON_2, INPUT);
  pinMode(BUTTON_3, INPUT);
  pinMode(BUTTON_4, INPUT);
  
  digitalWrite(RELAY_BUTTON_UMI, LOW);
  digitalWrite(RELAY_AC_UMI, LOW);
  digitalWrite(RELAY_AC_DEU, LOW);
  digitalWrite(RELAY_AC_COOL, LOW);
  digitalWrite(RELAY_AC_HEAT, LOW);
  digitalWrite(RELAY_FAN_EXT, LOW);

  oled.begin();

  bme.begin();
  /*
  bme.resetToDefaults();
  bme.writeOversamplingPressure(BMx280I2C::OSRS_P_x16);
  bme.writeOversamplingTemperature(BMx280I2C::OSRS_T_x16);
  bme.writeOversamplingHumidity(BMx280I2C::OSRS_H_x16);
  */
  Rtc.Begin();
  RtcDateTime currentTime = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetDateTime(currentTime);

  EEPROM.get(0, temp_set);
  EEPROM.get(1, delta_heat_ON);
  EEPROM.get(2, delta_heat_OFF);
  EEPROM.get(3, delta_cool_ON);
  EEPROM.get(4, delta_cool_OFF);
  EEPROM.get(5, hum_set);
  EEPROM.get(6, delta_hum_ON);
  EEPROM.get(7, delta_hum_OFF);
  EEPROM.get(8, delta_deu_ON);
  EEPROM.get(9, delta_deu_OFF);

  EEPROM.get(10, fan_start_hour_1);
  EEPROM.get(11, fan_start_min_1);
  EEPROM.get(12, fan_interval_min_1);
  EEPROM.get(13, fan_start_hour_2);
  EEPROM.get(14, fan_start_min_2);
  EEPROM.get(15, fan_interval_min_2);

  delay(500);
}
void loop() {
  bme.read(p, t, h);
  RtcDateTime now = Rtc.GetDateTime();

  fan_arguments fan_args = {fan_start_hour_1, fan_start_min_1, fan_interval_min_1, fan_start_hour_2, fan_start_min_2, fan_interval_min_2};
  ventilation(now, fan_args);

  arguments func_args = {t, h, temp_set, hum_set, delta_heat_ON, delta_heat_OFF, delta_cool_ON, delta_cool_OFF, delta_hum_ON, delta_hum_OFF, delta_deu_ON, delta_deu_OFF};

  bool button_state_1 = digitalRead(BUTTON_1); // Set T/H e SET
  bool button_state_2 = digitalRead(BUTTON_2); // - e Fan
  bool button_state_3 = digitalRead(BUTTON_3); // +
  bool button_state_4 = digitalRead(BUTTON_4); // Info e Home
  screen_state = state_selection(button_state_1, button_state_2, button_state_3, button_state_4, screen_state); // I use the function <state_selection> to select the value of screen_state

  switch (screen_state) {
    case 10:
      default_page(func_args, screen_state);
      break;
    case 20:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        temp_set -= 1;
        EEPROM.put(0, temp_set);
      } else if (button_state_3 == HIGH) {
        temp_set += 1;
        EEPROM.put(0, temp_set);
      }
      break;
    case 21:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_heat_ON -= 1;
        EEPROM.put(1, delta_heat_ON);
      } else if (button_state_3 == HIGH) {
        delta_heat_ON += 1;
        EEPROM.put(1, delta_heat_ON);
      }
      break;
    case 22:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_heat_OFF -= 1;
        EEPROM.put(2, delta_heat_OFF);
      } else if (button_state_3 == HIGH) {
        delta_heat_OFF += 1;
        EEPROM.put(2, delta_heat_OFF);
      }
      break;
    case 23:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_cool_ON -= 1;
        EEPROM.put(3, delta_cool_ON);
      } else if (button_state_3 == HIGH) {
        delta_cool_ON += 1;
        EEPROM.put(3, delta_cool_ON);
      }
      break;
    case 24:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_cool_OFF -= 1;
        EEPROM.put(4, delta_cool_OFF);
      } else if (button_state_3 == HIGH) {
        delta_cool_OFF += 1;
        EEPROM.put(4, delta_cool_OFF);
      }
      break;
    case 25:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        hum_set -= 1;
        EEPROM.put(5, hum_set);
      } else if (button_state_3 == HIGH) {
        hum_set += 1;
        EEPROM.put(5, hum_set);
      }
      break;
    case 26:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_hum_ON -= 1;
        EEPROM.put(6, delta_hum_ON);
      } else if (button_state_3 == HIGH) {
        delta_hum_ON += 1;
        EEPROM.put(6, delta_hum_ON);
      }
      break;
    case 27:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_hum_OFF -= 1;
        EEPROM.put(7, delta_hum_OFF);
      } else if (button_state_3 == HIGH) {
        delta_hum_OFF += 1;
        EEPROM.put(7, delta_hum_OFF);
      }
      break;
    case 28:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_deu_ON -= 1;
        EEPROM.put(8, delta_deu_ON);
      } else if (button_state_3 == HIGH) {
        delta_deu_ON += 1;
        EEPROM.put(8, delta_deu_ON);
      }
      break;
    case 29:
      set_parameter_page(func_args, screen_state);
      if (button_state_2 == HIGH) {
        delta_deu_OFF -= 1;
        EEPROM.put(9, delta_deu_OFF);
      } else if (button_state_3 == HIGH) {
        delta_deu_OFF += 1;
        EEPROM.put(9, delta_deu_OFF);
      }
      break;
    case 30:
      info_page(func_args, screen_state, now);
      break;
    case 31:
      set_fan_page(fan_args, screen_state);
      if (button_state_2 == HIGH) {
        fan_start_hour_1 -= 1;
        EEPROM.put(10, fan_start_hour_1);
      } else if (button_state_3 == HIGH) {
        fan_start_hour_1 += 1;
        EEPROM.put(10, fan_start_hour_1);
      }
      break;
    case 32:
      set_fan_page(fan_args, screen_state);
      if (button_state_2 == HIGH) {
        fan_start_min_1 -= 1;
        EEPROM.put(11, fan_start_min_1);
      } else if (button_state_3 == HIGH) {
        fan_start_min_1 += 1;
        EEPROM.put(11, fan_start_min_1);
      }
      break;
    case 33:
      set_fan_page(fan_args, screen_state);
      if (button_state_2 == HIGH) {
        fan_interval_min_1 -= 1;
        EEPROM.put(12, fan_interval_min_1);
      } else if (button_state_3 == HIGH) {
        fan_interval_min_1 += 1;
        EEPROM.put(12, fan_interval_min_1);
      }
      break;
    case 34:
      set_fan_page(fan_args, screen_state);
      if (button_state_2 == HIGH) {
        fan_start_hour_2 -= 1;
        EEPROM.put(13, fan_start_hour_2);
      } else if (button_state_3 == HIGH) {
        fan_start_hour_2 += 1;
        EEPROM.put(13, fan_start_hour_2);
      }
      break;
    case 35:
      set_fan_page(fan_args, screen_state);
      if (button_state_2 == HIGH) {
        fan_start_min_2 -= 1;
        EEPROM.put(14, fan_start_min_2);
      } else if (button_state_3 == HIGH) {
        fan_start_min_2 += 1;
        EEPROM.put(14, fan_start_min_2);
      }
      break;
    case 36:
      set_fan_page(fan_args, screen_state);
      if (button_state_2 == HIGH) {
        fan_interval_min_2 -= 1;
        EEPROM.put(15, fan_start_min_2);
      } else if (button_state_3 == HIGH) {
        fan_start_min_2 += 1;
        EEPROM.put(15, fan_start_min_2);
      }
      break;
  }
  // Call for the functions that are keeping T and H constant.
  keep_T_const(func_args);
  keep_H_const(func_args);

  delay(300);
}
// ______________________________________________ Functions __________________________________________________

int state_selection(bool button_state_1, bool button_state_2, bool button_state_3, bool button_state_4, byte screen_state) {

  if (button_state_1 == HIGH && screen_state == 10) { // If you are on HOME and you press the first button, you go to T/H selection page.
    screen_state = 20;
  } else if (button_state_2 == HIGH && screen_state == 10) { // Homepage to Fan (Set hour 1)
    screen_state = 31;
  } else if (button_state_4 == HIGH && screen_state == 31) { // Fan (Set min 1)
    screen_state = 32;
  } else if (button_state_4 == HIGH && screen_state == 32) { // Fan (Set interval 1)
    screen_state = 33;
  } else if (button_state_4 == HIGH && screen_state == 33) { // Fan (Set hour 2)
    screen_state = 34;
  } else if (button_state_4 == HIGH && screen_state == 34) { // Fan (Set min 2)
    screen_state = 35;
  } else if (button_state_4 == HIGH && screen_state == 35) { // Fan (Set interval 2)
    screen_state = 36;
  } else if (button_state_4 == HIGH && screen_state == 36) { // Return to home
    screen_state = 10;

  } else if (button_state_4 == HIGH && screen_state == 10) { // Homepage to Info
    screen_state = 30;
  } else if (button_state_4 == HIGH && screen_state == 20) {
    screen_state = 21;
  } else if (button_state_4 == HIGH && screen_state == 21) {
    screen_state = 22;
  } else if (button_state_4 == HIGH && screen_state == 22) {
    screen_state = 23;
  } else if (button_state_4 == HIGH && screen_state == 23) {
    screen_state = 24;
  } else if (button_state_4 == HIGH && screen_state == 24) {
    screen_state = 25;
  } else if (button_state_4 == HIGH && screen_state == 25) {
    screen_state = 26;
  } else if (button_state_4 == HIGH && screen_state == 26) {
    screen_state = 27;
  } else if (button_state_4 == HIGH && screen_state == 27) {
    screen_state = 28;
  } else if (button_state_4 == HIGH && screen_state == 28) {
    screen_state = 29;
  } else if (button_state_4 == HIGH && screen_state == 29) {
    screen_state = 10;
  } else if (button_state_1 == HIGH  && (screen_state == 20 || screen_state == 21 || screen_state == 22 || screen_state == 23 ||
                                         screen_state == 24 || screen_state == 25 || screen_state == 26 || screen_state == 27 ||
                                         screen_state == 28 || screen_state == 29 || screen_state == 30 || screen_state == 31 ||
                                         screen_state == 32 || screen_state == 33 || screen_state == 34 || screen_state == 35 ||
                                         screen_state == 36)) {
    screen_state = 10;
  }
  return screen_state;
}
// OLED menu drawing functions:
void default_page(arguments& args, byte screen_state) {
  //oled.clearBuffer();
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_8x13_tr);
    oled.drawStr(7, 14, "T:");
    oled.drawStr(7, 30, "H:");
    // Print current temperature and humidity values (showing 1 decimal).
    oled.setCursor(25, 14);
    oled.print(args.t_current, 1);
    oled.setCursor(65, 14);
    oled.print(F("C"));
    oled.setCursor(25, 30);
    oled.print(args.h_current, 1);
    oled.print(F("%"));
    // Write T+ T- H+ H- on top of humi/deum cooler/heater state boxes
    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(59, 8, "o"); // degree symbol for T_current
    oled.drawStr(92, 8, "T+");
    oled.drawStr(112, 8, "T-");
    oled.drawStr(92, 37, "H+");
    oled.drawStr(112, 37, "H-");
    // Write T_set, H_set, dt, dh values
    oled.setCursor(7, 43);
    oled.print(F("T"));
    oled.setFont(u8g2_font_4x6_tr);
    oled.print(F("set"));

    oled.drawStr(50, 41, "+"); // +/- symbol for T_set
    oled.drawStr(50, 45, "-");
    oled.drawStr(63, 38, "o"); // degree symbol for T_set

    oled.drawStr(50, 52, "+"); // +/- symbol for H_set
    oled.drawStr(50, 56, "-");

    oled.setFont(u8g2_font_6x10_tr);
    oled.drawStr(68, 43, "C");
    oled.drawStr(67, 54, "%");

    oled.print( F(": ") );
    oled.print(args.t_set);
    
    oled.setCursor(7, 54);
    oled.print( F("H") );
    oled.setFont(u8g2_font_4x6_tr);
    oled.print( F("set") );
    oled.setFont(u8g2_font_6x10_tr);
    oled.print( F(": "));
    oled.print(args.h_set);

    oled.setFont(u8g2_font_4x6_tr);
    oled.setCursor(56, 51);
    oled.print(args.dh_deu_on);
    oled.setCursor(56, 56);
    oled.print(args.dh_hum_on);
    oled.setCursor(56, 39);
    oled.print(args.dt_cool_on);
    oled.setCursor(56, 45);
    oled.print(args.dt_heat_on);

    // Screen selection white bar + vertical line
    oled.drawLine(83, 0, 83, 70);

    drawBar(screen_state);

    drawStateBox(89, 11, heat_state, "T+");
    drawStateBox(109, 11, cool_state, "T-");
    drawStateBox(89, 40, umi_state, "H+");
    drawStateBox(109, 40, deu_state, "H-");

    //oled.sendBuffer();
  } while ( oled.nextPage() );
}
void info_page(arguments& args, byte screen_state, RtcDateTime now) {
  oled.firstPage();
  do {
    oled.setFont(u8g2_font_4x6_tr);
    oled.setCursor(1, 8);
    oled.print( F("Fase: Stagionatura") );
    oled.setCursor(1, 17);
    oled.print( F("Prodotto: Guanciale") );
    oled.setCursor(1, 26);
    oled.print( F("Inizio: 24/4/24") );
    oled.setCursor(1, 35);
    oled.print( F("Temperatura:") );
    oled.print(args.t_set);
    oled.setCursor(1, 44);
    oled.print(F("Umidita':") );
    oled.print(args.h_set);

    oled.setCursor(1, 53);
    oled.print("Ora: ");
    oled.print(now.Hour());
    oled.print(":");
    oled.print(now.Minute());

    drawBar(screen_state);
  } while (oled.nextPage());
}
void set_fan_page(fan_arguments& args, byte screen_state) {
  oled.firstPage();
  do {
    switch (screen_state) {
      case 31: // HOUR 1
        oled.drawFrame(0, 0, 64, 11); 
        break;
      case 32: // MIN 1
        oled.drawFrame(0, 13, 64, 11);
        break;
      case 33: // INTERVAL 1
        oled.drawFrame(0, 26, 64, 11);
        break;
      case 34: // HOUR 2
        oled.drawFrame(65, 0, 63, 11);
        break;
      case 35: // MIN 2
        oled.drawFrame(65, 13, 63, 11);
        break;
      case 36: // INTERVAL 2
        oled.drawFrame(65 ,26, 63, 11);
        break;
    }
    oled.setFont(u8g2_font_6x10_tr);
    switch (fan_state) {
      case 0:
        oled.drawStr(42, 50, "FAN OFF");
        break;
      case 1:
        oled.drawStr(47, 50, "FAN ON");
        break;
    }
    writeFanPage(args);
    drawBar(screen_state);

  } while ( oled.nextPage() );
}
void set_parameter_page(arguments& args, byte screen_state) {
  oled.firstPage();
  do {
    switch (screen_state) {
      case 20: // TEMP
        oled.drawFrame(8, 0, 50, 11); 
        break;
      case 21: // HEAT ON
        oled.drawFrame(8, 11, 50, 11);
        break;
      case 22: // HEAT OFF
        oled.drawFrame(8, 22, 50, 11);
        break;
      case 23: // COOL ON
        oled.drawFrame(8, 33, 50, 11);
        break;
      case 24: // COOL OFF
        oled.drawFrame(8, 44, 50, 11);
        break;
      case 25: // HUM
        oled.drawFrame(68, 0, 50, 11);
        break;
      case 26: // UMI ON
        oled.drawFrame(68, 11, 50, 11);
        break;
      case 27: // UMI OFF
        oled.drawFrame(68, 22, 50, 11);
        break;
      case 28: // DEU ON
        oled.drawFrame(68, 33, 50, 11);
        break;
      case 29: // DEU OFF
        oled.drawFrame(68, 44, 50, 11);
        break;
    }
    writeParPage(args);
    drawBar(screen_state);

  } while ( oled.nextPage() );
}
void drawBar(byte screen_state) {
  oled.drawBox(0, 57, 130, 7);
  oled.setCursor(3, 63);
  oled.setFont(u8g2_font_4x6_tr);
  oled.setColorIndex(0);
  switch (screen_state) {
    case 10:
      oled.print( F("Set T/H") );
      oled.setCursor(110, 63);
      oled.print( F("Info") );
      oled.drawStr(62, 63, "Fan");
      break;
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26: 
    case 27:
    case 28: 
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
        oled.print(F("Home") );
        oled.setCursor(46, 63);
        oled.print( F("-") );
        oled.setCursor(80, 63);
        oled.print( F("+") );
        oled.setCursor(110, 63);
        oled.print( F("Next") );
        break;
    case 29:
    case 36:
      oled.print(F("Home") );
      oled.setCursor(46, 63);
      oled.print( F("-") );
      oled.setCursor(80, 63);
      oled.print( F("+") );
      oled.setCursor(110, 63);
      oled.print( F("Exit") );
      break;
    case 30:
      oled.print( F("Home") );
      oled.setCursor(110, 63);
      break;
  }
  oled.setColorIndex(1);
}
void drawStateBox(int x, int y, bool device_state, char* symbol ) {
  oled.setFont(u8g2_font_4x6_tr);
  char* OnStr = "On";
  char* OffStr = "Off";
  const unsigned int dim = 15;
  if (device_state == 1) {
    oled.drawBox(x, y, dim, dim); // H+ ON == WHITE BOX
    oled.setColorIndex(0);  // Imposta il colore a nero
    oled.drawStr(x + 4, y + 10, OnStr); // Scrive il testo all'interno del rettangolo
    oled.setColorIndex(1);  // Imposta il colore a bianco
  } else {
    oled.drawFrame(x, y, dim, dim); // T+ ON == BLACK BOX
    oled.drawStr(x + 2, y + 10, OffStr); // Scrive il testo all'interno del rettangolo
  }
  // Write T+ T- H+ H- on top of humi/deum cooler/heater state boxes
  oled.setFont(u8g2_font_6x10_tr);
  oled.drawStr(x + 3, y - 3, symbol);
}
void writeParPage(arguments& args){
  oled.setFont(u8g2_font_4x6_tr);

  oled.drawStr(10, 8, "TEMP:");
  oled.setCursor(48, 8);
  oled.print(args.t_set);

  oled.drawStr(10, 19, "HEAT ON:");
  oled.setCursor(48, 19);
  oled.print(args.dt_heat_on);

  oled.drawStr(10, 30, "HEAT OFF:");
  oled.setCursor(48, 30);
  oled.print(args.dt_heat_off);

  oled.drawStr(10, 41, "COOL ON:");
  oled.setCursor(48, 41);
  oled.print(args.dt_cool_on);

  oled.drawStr(10, 52, "COOL OFF:");
  oled.setCursor(48, 52);
  oled.print(args.dt_cool_off);

  oled.drawStr(70, 8, "HUM:");
  oled.setCursor(105, 8);
  oled.print(args.h_set);

  oled.drawStr(70, 19, "UMI ON:");
  oled.setCursor(105, 19);
  oled.print(args.dh_hum_on);

  oled.drawStr(70, 30, "UMI OFF:");
  oled.setCursor(105, 30);
  oled.print(args.dh_hum_off);

  oled.drawStr(70, 41, "DEU ON:");
  oled.setCursor(105, 41);
  oled.print(args.dh_deu_on);

  oled.drawStr(70, 52, "DEU OFF:");
  oled.setCursor(105, 52);
  oled.print(args.dh_deu_off);
}
void writeFanPage(fan_arguments& args){
  oled.setFont(u8g2_font_6x10_tr);
  oled.setCursor(2, 9);
  oled.print("Hour 1: "); 
  oled.print(args.start_1_hr);

  oled.setCursor(2, 22);
  oled.print("Min 1: "); 
  oled.print(args.start_1_min);

  oled.setCursor(2, 35);
  oled.print("dt 1: "); 
  oled.print(args.interval_1_min);

  oled.setCursor(67, 9);
  oled.print("Hour 2: "); 
  oled.print(args.start_2_hr);

  oled.setCursor(67, 22);
  oled.print("Min 2: "); 
  oled.print(args.start_2_min);

  oled.setCursor(67, 35);
  oled.print("dt 2: "); 
  oled.print(args.interval_2_min);
}
// TEMPERATURE control with REFRIGERATOR and LIGHT BULB
void keep_T_const(arguments& args) {
  if ( args.t_current >= args.t_set + args.dt_cool_on && cool_state == 0) {
    cool_control(ON);
    cool_state = 1;
  } else if ( args.t_current <= args.t_set + args.dt_cool_off && cool_state == 1) {
    cool_control(OFF);
    cool_state = 0;
  } else if ( args.t_current <= args.t_set - args.dt_heat_on && heat_state == 0) {
    heat_control(ON);
    heat_state = 1;
  } else if ( args.t_current >= args.t_set - args.dt_heat_off && heat_state == 1) {
    heat_control(OFF);
    heat_state = 0;
  } else {
    //Nothing to do
  }
}
// HUMIDITY control with HUMIDIFIER and DEHUMIDIFIER
void keep_H_const(arguments& args) {
  if ( args.h_current >= args.h_set + args.dh_deu_on && deu_state == 0) {
    deu_control(ON);
    deu_state = 1;
  } else if ( args.h_current <= args.h_set + args.dh_deu_off && deu_state == 1) {
    deu_control(OFF);
    deu_state = 0;
  } else if ( args.h_current <= args.h_set - args.dh_hum_on && umi_state == 0) {
    umi_control(ON);
    umi_state = 1;
  } else if ( args.h_current >= args.h_set - args.dh_hum_off && umi_state == 1) {
    umi_control(OFF);
    umi_state = 0;
  } else {
    //Nothing to do
  }
}
void ventilation(RtcDateTime now, fan_arguments& args) {
  if ( (now.Hour() == args.start_1_hr && (now.Minute() >= args.start_1_min && now.Minute() <= args.start_1_min + args.interval_1_min)) || 
       (now.Hour() == args.start_2_hr && (now.Minute() >= args.start_2_min && now.Minute() <= args.start_2_min + args.interval_2_min))) {
    digitalWrite(RELAY_FAN_EXT, HIGH);
    fan_state = 1;
  } else {
    digitalWrite(RELAY_FAN_EXT, LOW);
    fan_state = 0;
  }
}
// Definition of the 4 functions for the device control: used to turn them ON or OFF.
void umi_control(bool command) {
  if (command == ON) {
    // Turn ON the humidifier AC outlet
    digitalWrite(RELAY_AC_UMI, HIGH);
    delay(500);

    // Double short press on humidifier button to turn it ON (with continuous working flow)
    digitalWrite(RELAY_BUTTON_UMI, HIGH);
    delay(200);
    digitalWrite(RELAY_BUTTON_UMI, LOW);
    delay(200);
    digitalWrite(RELAY_BUTTON_UMI, HIGH);
    delay(200);
    digitalWrite(RELAY_BUTTON_UMI, LOW);

  } else if (command == OFF) {
    // Long press on humidifier button to turn it OFF
    digitalWrite(RELAY_BUTTON_UMI, HIGH);
    delay(2100);
    digitalWrite(RELAY_BUTTON_UMI, LOW);
    delay(200);

    // Turn OFF the humidifier AC outlet
    digitalWrite(RELAY_AC_UMI, LOW);

  } else {
    return;
  }
}
void cool_control(bool command) {
  if (command == ON) {
    // Turn ON the refrigerator AC outlet
    digitalWrite(RELAY_AC_COOL, HIGH);
  } else if (command == OFF) {
    // Turn OFF the refrigerator AC outlet
    digitalWrite(RELAY_AC_COOL, LOW);
  } else {
    return;
  }
}
void deu_control(bool command) {
  if (command == ON) {
    // Turn ON the dehumidifier AC outlet
    digitalWrite(RELAY_AC_DEU, HIGH);
  } else if (command == OFF) {
    // Turn OFF the dehumidifier AC outlet
    digitalWrite(RELAY_AC_DEU, LOW);
  } else {
    return;
  }
}
void heat_control(bool command) {
  if (command == ON) {
    // Turn ON the heater AC outlet
    digitalWrite(RELAY_AC_HEAT, HIGH);
  } else if (command == OFF) {
    // Turn OFF the heater AC outlet
    digitalWrite(RELAY_AC_HEAT, LOW);
  } else {
    return;
  }
}