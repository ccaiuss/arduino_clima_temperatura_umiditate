#include <Arduino.h>
#include <SimpleDHT.h>
#include <elapsedMillis.h>
#include <RTClib.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
RTC_DS1307 rtc;

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 16 chars and 2 line display
elapsedMillis timerTest, timerDht22, timerSalvare, timerRtc;

#include "Button.h"
#define ON 0 // inversare citire pin din HIGHT to LOW
#define OFF 1

#define TEMP_MINIM -40
#define TEMP_MAXIM 40
#define HUM_MINIM 0
#define HUM_MAXIM 100
#define HISTE 10 // limita histeresis pentru temp si hum

#define BTN_ENTER 7 // definire pini butoane
#define BTN_LEFT 8
#define BTN_RIGHT 9
#define BTN_UP 10
#define BTN_DOWN 11

#define PIN_RELEU_1 2
#define PIN_RELEU_2 3
#define PIN_RELEU_3 4
#define PIN_RELEU_4 5

Button enter(7);
Button left(8);
Button right(9);
Button up(10);
Button down(11);

int pag = 1;
int nrPag = 5;
int lineFocus = 0;
int ora = 0;
int minute = 0;
int pinDHT22 = 6;
float temp = NAN;
float hum = NAN;
SimpleDHT22 dht22(pinDHT22);

struct data
{
  float tempZi1 = 10, tempZi2 = 10;
  float tempNoapte1 = 10, tempNoapte2 = 10;
  int oraStartZi = 8, oraSfarsitZi = 22;
  int humZi3 = 50, humZi4 = 50;
  int humNoapte3 = 50, humNoapte4 = 50;
  int histe1 = 2, histe2 = 2, histe3 = 2, histe4 = 2;
  int R1 = OFF, R2 = OFF, R3 = OFF, R4 = OFF;
} data;

boolean salvare = false;

byte rFocus[] = {B00000, B00100, B00010, B11111, B11111, B00010, B00100, B00000};
enum Menu
{
  START,
  RELEU_1,
  RELEU_2,
  RELEU_3,
  RELEU_4,
  SETARI,
  STOP
} menu;
enum CicluOre
{
  ZI,
  NOAPTE
} cicluOre;
void lcdPrint(int col, int row, String text, int val = -100)
{
  lcd.setCursor(col, row);
  lcd.print(text);
  lcd.setCursor(col + text.length(), row);
  if (val > -100)
    lcd.print(val);
}
void pinWrite(int pin, int mod)
{
  switch (pin)
  {
  case PIN_RELEU_1:
    if (data.R1 != mod)
    {
      data.R1 = mod;
      salvare = true;
      if (pag == START)
      {
        menu = START;
      }
    }
    break;
  case PIN_RELEU_2:
    if (data.R2 != mod)
    {
      data.R2 = mod;
      salvare = true;
      if (pag == START)
      {
        menu = START;
      }
    }
    break;
  case PIN_RELEU_3:
    if (data.R3 != mod)
    {
      data.R3 = mod;
      salvare = true;
      if (pag == START)
      {
        menu = START;
      }
    }
    break;
  case PIN_RELEU_4:
    if (data.R4 != mod)
    {
      data.R4 = mod;
      salvare = true;
      menu = START;
    }
    break;
  }
  digitalWrite(pin, mod);
}
void setup()
{
  Serial.begin(9600);
  rtc.begin();
  EEPROM.get(0, data); // comentati la prima incarcare pt a se memora setarile de fabrica

  if (!rtc.isrunning())
  {
    Serial.println("RTC is off!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  // rtc.adjust(DateTime(2022, 3, 03, 18, 45, 0)); // year,month,day,hour,min,sec
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, rFocus);
  menu = START;
  pag = START;

  pinMode(PIN_RELEU_1, OUTPUT);
  pinMode(PIN_RELEU_2, OUTPUT);
  pinMode(PIN_RELEU_3, OUTPUT);
  pinMode(PIN_RELEU_4, OUTPUT);

  digitalWrite(PIN_RELEU_1, data.R1);
  digitalWrite(PIN_RELEU_2, data.R2);
  digitalWrite(PIN_RELEU_3, data.R3);
  digitalWrite(PIN_RELEU_4, data.R4);

  int nrIncercari = 0;
  while (dht22.read2(&temp, &hum, NULL) != SimpleDHTErrSuccess)
  {
    Serial.print("incercari:");

    delay(1000);
    nrIncercari++;
    if (nrIncercari == 3)
    {
      break;
    }
  }
}

void loop()
{
  DateTime time = rtc.now();
  if (timerRtc > 1000)
  {
    if (pag == START)
    {
      lcdPrint(0, 0, "                    ");
      lcdPrint(6, 0, time.timestamp(DateTime::TIMESTAMP_TIME));
    }
    timerRtc = 0;
  }
  if (timerDht22 > 3000)
  {
    if (dht22.read2(&temp, &hum, NULL) == SimpleDHTErrSuccess)
    {
      if (pag == START)
      {
        lcdPrint(0, 1, "                    ");
        lcdPrint(0, 1, "TEMP:" + String(temp));
        lcdPrint(11, 1, "HUM:" + String(hum));
      }
    }
    timerDht22 = 0;
  } // end citire temperatura
  if (time.hour() >= data.oraStartZi && time.hour() <= data.oraSfarsitZi)
  {
    cicluOre = ZI;
  }
  else
  {
    cicluOre = NOAPTE;
  }

  switch (cicluOre)
  {
  case ZI:
    if (!isnan(temp) && !isnan(hum))
    {
      // releu 1
      if (temp <= data.tempZi1 - data.histe1)
      {
        pinWrite(PIN_RELEU_1, ON);
      }
      else if (temp >= data.tempZi1)
      {
        pinWrite(PIN_RELEU_1, OFF);
      }
      // releu 2
      if (temp <= data.tempZi2 - data.histe2)
      {
        pinWrite(PIN_RELEU_2, ON);
      }
      else if (temp >= data.tempZi2)
      {
        pinWrite(PIN_RELEU_2, OFF);
      }
      // releu 3
      if (hum <= data.humZi3 - data.histe3)
      {
        pinWrite(PIN_RELEU_3, ON);
      }
      else if (hum >= data.humZi3)
      {
        pinWrite(PIN_RELEU_3, OFF);
      }
      // releu 4
      if (hum <= data.humZi4 - data.histe4)
      {
        pinWrite(PIN_RELEU_4, ON);
      }
      else if (hum >= data.humZi4)
      {
        pinWrite(PIN_RELEU_4, OFF);
      }
    }
    else
    {
      pinWrite(PIN_RELEU_1, OFF);
      pinWrite(PIN_RELEU_2, OFF);
      pinWrite(PIN_RELEU_3, OFF);
      pinWrite(PIN_RELEU_4, OFF);
    }
    break; // end ciclu ZI

  case NOAPTE:

    if (!isnan(temp) && !isnan(hum))
    {
      // releu 1
      if (temp <= data.tempNoapte1 - data.histe1)
      {
        pinWrite(PIN_RELEU_1, ON);
      }
      else if (temp >= data.tempNoapte1)
      {
        pinWrite(PIN_RELEU_1, OFF);
      }
      // releu 2
      if (temp <= data.tempNoapte2 - data.histe2)
      {
        pinWrite(PIN_RELEU_2, ON);
      }
      else if (temp >= data.tempNoapte2)
      {
        pinWrite(PIN_RELEU_2, OFF);
      }
      // releu 3
      if (hum <= data.humNoapte3 - data.histe3)
      {
        pinWrite(PIN_RELEU_3, ON);
      }
      else if (hum >= data.humNoapte3)
      {
        pinWrite(PIN_RELEU_3, OFF);
      }
      // releu 4
      if (hum <= data.humNoapte4 - data.histe4)
      {
        pinWrite(PIN_RELEU_4, ON);
      }
      else if (hum >= data.humNoapte4)
      {
        pinWrite(PIN_RELEU_4, OFF);
      }
    }
    else
    {
      pinWrite(PIN_RELEU_1, OFF);
      pinWrite(PIN_RELEU_2, OFF);
      pinWrite(PIN_RELEU_3, OFF);
      pinWrite(PIN_RELEU_4, OFF);
    }
    break; // end ciclu noapte
  }        // end ciclu ore
  if (pag != 0)
  {
    if (up.check() == ON)
    {
      if (pag == 1)
      {
        if (lineFocus == 1)
        {
          if (data.tempZi1 < TEMP_MAXIM)
            data.tempZi1++;
        }
        if (lineFocus == 2)
        {
          if (data.tempNoapte1 < TEMP_MAXIM)
            data.tempNoapte1++;
        }
        if (lineFocus == 3)
        {
          if (data.histe1 < HISTE)
            data.histe1++;
        }
      }

      if (pag == 2)
      {
        if (lineFocus == 1)
        {
          if (data.tempZi2 < TEMP_MAXIM)
            data.tempZi2++;
        }
        if (lineFocus == 2)
        {
          if (data.tempNoapte2 < TEMP_MAXIM)
            data.tempNoapte2++;
        }
        if (lineFocus == 3)
        {
          if (data.histe2 < HISTE)
            data.histe2++;
        }
      }
      if (pag == 3)
      {
        if (lineFocus == 1)
        {
          if (data.humZi3 < HUM_MAXIM)
            data.humZi3++;
        }
        if (lineFocus == 2)
        {
          if (data.humNoapte3 < HUM_MAXIM)
            data.humNoapte3++;
        }
        if (lineFocus == 3)
        {
          if (data.histe3 < HISTE)
            data.histe3++;
        }
      }
      if (pag == 4)
      {
        if (lineFocus == 1)
        {
          if (data.humZi4 < HUM_MAXIM)
            data.humZi4++;
        }
        if (lineFocus == 2)
        {
          if (data.humNoapte4 < HUM_MAXIM)
            data.humNoapte4++;
        }
        if (lineFocus == 3)
        {
          if (data.histe4 < HISTE)
            data.histe4++;
        }
      }
      if (pag == 5)
      {
        if (lineFocus == 0)
        {
          if (data.oraStartZi < 23 && data.oraStartZi + 1 < data.oraSfarsitZi)
            data.oraStartZi++;
        }
        if (lineFocus == 1)
        {
          if (data.oraSfarsitZi < 23 && data.oraSfarsitZi + 1 > data.oraStartZi)
            data.oraSfarsitZi++;
        }
        if (lineFocus == 2)
        {
          if (ora >= 0 && ora <= 23)
          {
            ora++;
            Serial.println(ora);
            rtc.adjust(DateTime(time.year(), time.month(), time.day(), ora, time.minute(), 0));
          }
        }
        if (lineFocus == 3)
        {
          if (minute >= 0 && minute <= 59)
          {
            minute++;
            rtc.adjust(DateTime(time.year(), time.month(), time.day(), time.hour(), minute, 0));
          }
        }
      }
      menu = static_cast<Menu>(pag);
      Serial.print("UP: ");
      Serial.print(pag);
      Serial.print(" line Focus: ");
      Serial.println(lineFocus);
      salvare = true;
    }

    if (down.check() == ON)
    {
      if (pag == 1)
      {
        if (lineFocus == 1)
        {
          if (data.tempZi1 > TEMP_MINIM)
            data.tempZi1--;
        }
        if (lineFocus == 2)
        {
          if (data.tempNoapte1 > TEMP_MINIM)
            data.tempNoapte1--;
        }
        if (lineFocus == 3)
        {
          if (data.histe1 > 1)
            data.histe1--;
        }
      }
      if (pag == 2)
      {
        if (lineFocus == 1)
        {
          if (data.tempZi2 > TEMP_MINIM)
            data.tempZi2--;
        }
        if (lineFocus == 2)
        {
          if (data.tempNoapte2 > TEMP_MINIM)
            data.tempNoapte2--;
        }
        if (lineFocus == 3)
        {
          if (data.histe2 > 1)
            data.histe2--;
        }
      }
      if (pag == 3)
      {
        if (lineFocus == 1)
        {
          if (data.humZi3 > HUM_MINIM)
            data.humZi3--;
        }
        if (lineFocus == 2)
        {
          if (data.humNoapte3 > HUM_MINIM)
            data.humNoapte3--;
        }
        if (lineFocus == 3)
        {
          if (data.histe3 > 1)
            data.histe3--;
        }
      }
      if (pag == 4)
      {
        if (lineFocus == 1)
        {
          if (data.humZi4 > HUM_MINIM)
            data.humZi4--;
        }
        if (lineFocus == 2)
        {
          if (data.humNoapte4 > HUM_MINIM)
            data.humNoapte4--;
        }
        if (lineFocus == 3)
        {
          if (data.histe4 > 1)
            data.histe4--;
        }
      }
      if (pag == 5)
      {
        if (lineFocus == 0)
        {
          if (data.oraStartZi > 0 && data.oraStartZi < data.oraSfarsitZi + 1)
            data.oraStartZi--;
        }
        if (lineFocus == 1)
        {
          if (data.oraSfarsitZi > 0 && data.oraSfarsitZi > data.oraStartZi + 1)
            data.oraSfarsitZi--;
        }
        if (lineFocus == 2)
        {
          if (ora > 0)
          {
            ora--;
            rtc.adjust(DateTime(time.year(), time.month(), time.day(), ora, time.minute(), 0));
          }
        }
        if (lineFocus == 3)
        {
          if (minute > 0)
          {
            minute--;
            rtc.adjust(DateTime(time.year(), time.month(), time.day(), time.hour(), minute, 0));
          }
        }
      }
      menu = static_cast<Menu>(pag);
      Serial.print("DOWN: ");
      Serial.print(pag);
      Serial.print(" line Focus: ");
      Serial.println(lineFocus);

      salvare = true;
    } // end down

    if (enter.check() == ON)
    {
      lineFocus++;
      if (lineFocus >= 0)
      {
        menu = static_cast<Menu>(pag);
      }
      if (lineFocus > 3)
      {
        if (pag != 5)
        {
          lineFocus = 1;
        }
        else if (pag == 5)
        {
          lineFocus = 0;
        }
      }
      Serial.print(F("ENTER: "));
      Serial.println(lineFocus);
    }
  }
  if (left.check() == ON)
  {
    pag--;
    if (pag != 5)
    {
      lineFocus = 1;
    }
    else if (pag == 5)
    {
      lineFocus = 0;
    }
    if (pag >= 0)
    {
      menu = static_cast<Menu>(pag);
    }
    else
    {
      pag = nrPag;
      menu = static_cast<Menu>(pag);
    }

    Serial.println(F("Left button pressed"));
    Serial.println("NR PAG:" + String(nrPag));
  }
  if (right.check() == ON)
  {
    pag++;
    if (pag != 5)
    {
      lineFocus = 1;
    }
    else if (pag == 5)
    {
      lineFocus = 0;
    }

    if (pag <= nrPag)
    {
      menu = static_cast<Menu>(pag);
    }
    else
    {
      pag = 0;
      menu = static_cast<Menu>(pag);
    }
    Serial.println(F("right button pressed"));
  }
  switch (menu)
  {
  case SETARI:

    lcd.clear();
    lcdPrint(1, 0, "Ora start zi: ", data.oraStartZi);
    lcdPrint(1, 1, "Ora sfarsit zi: ", data.oraSfarsitZi);
    lcdPrint(1, 2, "Ceas ora: " + String(ora));
    lcdPrint(1, 3, "Ceas minute: " + String(minute));
    if (lineFocus >= 0)
    {
      lcd.setCursor(0, lineFocus);
      lcd.write(0);
    }
    menu = STOP;
    break;

  case START:
    lcd.clear();
    lcdPrint(0, 1, "TEMP:" + String(temp));
    lcdPrint(11, 1, "HUM:" + String(hum));

    if (data.R1 == ON)
    {
      lcdPrint(1, 2, "R 1: ON");
    }
    else
    {
      lcdPrint(1, 2, "R 1: OFF");
    }
    if (data.R3 == ON)
    {
      lcdPrint(11, 2, "R 3: ON");
    }
    else
    {
      lcdPrint(11, 2, "R 3: OFF");
    }
    if (data.R2 == ON)
    {
      lcdPrint(1, 3, "R 2: ON");
    }
    else
    {
      lcdPrint(1, 3, "R 2: OFF");
    }
    if (data.R4 == ON)
    {
      lcdPrint(11, 3, "R 4: ON");
    }
    else
    {
      lcdPrint(11, 3, "R 4: OFF");
    }
    menu = STOP;

    break;
  case RELEU_1:
    lcd.clear();
    lcdPrint(2, 0, "SETARI RELEU 1");
    lcdPrint(1, 1, "TEMP ZI: ", data.tempZi1);
    lcdPrint(1, 2, "TEMP NOAPTE: ", data.tempNoapte1);
    lcdPrint(1, 3, "HISTERESIS: ", data.histe1);
    if (lineFocus > 0)
    {
      lcd.setCursor(0, lineFocus);
      lcd.write(0);
    }
    menu = STOP;
    break;
  case RELEU_2:
    lcd.clear();
    lcdPrint(2, 0, "SETARI RELEU 2");
    lcdPrint(1, 1, "TEMP ZI: ", data.tempZi2);
    lcdPrint(1, 2, "TEMP NOAPTE: ", data.tempNoapte2);
    lcdPrint(1, 3, "HISTERESIS: ", data.histe2);
    if (lineFocus > 0)
    {
      lcd.setCursor(0, lineFocus);
      lcd.write(0);
    }
    menu = STOP;
    break;
  case RELEU_3:
    lcd.clear();
    lcdPrint(2, 0, "SETARI RELEU 3");
    lcdPrint(1, 1, "HUM ZI: ", data.humZi3);
    lcdPrint(1, 2, "HUM NOAPTE: ", data.humNoapte3);
    lcdPrint(1, 3, "HISTERESIS: ", data.histe3);
    if (lineFocus > 0)
    {
      lcd.setCursor(0, lineFocus);
      lcd.write(0);
    }
    menu = STOP;
    break;
  case RELEU_4:
    lcd.clear();
    lcdPrint(2, 0, "SETARI RELEU 4");
    lcdPrint(1, 1, "HUM ZI: ", data.humZi4);
    lcdPrint(1, 2, "HUM NOAPTE: ", data.humNoapte4);
    lcdPrint(1, 3, "HISTERESIS: ", data.histe4);
    if (lineFocus > 0)
    {
      lcd.setCursor(0, lineFocus);
      lcd.write(0);
    }
    menu = STOP;
    break;
  case STOP:
    
    break;
  }

  if (salvare)
  {
    if (timerSalvare > 2000) // salvare o singura data de la ultima modificare
    {
      EEPROM.put(0, data);
      salvare = false;
    }
  }
  else
  {
    timerSalvare = 0;
  }
}
