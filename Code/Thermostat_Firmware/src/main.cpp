#include <Arduino.h>
#include <max6675.h>
#include <EEPROM.h>
#include <U8glib.h>
#define PIN_RLY_CTRL 9
#define PIN_MAX6675_DO 12
#define PIN_MAX6675_CS 10
#define PIN_MAX6675_CLK 13
#define PIN_SW_TEMP_UP 3
#define PIN_SW_TEMP_DN 2
#define PIN_SW_SETUP 8
#define PIN_SW_ENTER 7
#define MAX_TEMP 200
#define MIN_TEMP 20

MAX6675 max6675(PIN_MAX6675_CLK, PIN_MAX6675_CS, PIN_MAX6675_DO);
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
uint16_t setTemp, storedTemp, actualTemp;
uint32_t lastSetTempChangeTime, lastActualTempRead;
bool saveTempChange;
bool setupMode=false;

void updateTemperatures();
void drawSetupMenu();
void checkButtons();
void updateRelay();

void setup() 
{
  pinMode(PIN_RLY_CTRL, OUTPUT);
  pinMode(PIN_SW_TEMP_UP, INPUT_PULLUP);
  pinMode(PIN_SW_TEMP_DN, INPUT_PULLUP);
  pinMode(PIN_SW_SETUP, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println("MAX6675 test");
  delay(500); // Wait for MAX chip to stabilize
  storedTemp=(EEPROM.read(0)<<8) | EEPROM.read(1);
  Serial.print("Stored temp: ");
  Serial.println(storedTemp);
  if(storedTemp>MIN_TEMP && storedTemp<=MAX_TEMP) setTemp=storedTemp;
  u8g.setColorIndex(1);
}

void loop() 
{
  if(saveTempChange && (millis()-lastSetTempChangeTime>5000))
  {
    Serial.println("Save to EEPROM");
    EEPROM.write(0, highByte(setTemp));
    EEPROM.write(1, lowByte(setTemp));
    saveTempChange=false;
  }
  if(millis()-lastActualTempRead>500)
  {
    actualTemp=max6675.readCelsius();  
    lastActualTempRead=millis();
    updateRelay();
  }
  checkButtons();

  //Update display
  u8g.firstPage(); 
  if(setupMode)
  {
    do {
    drawSetupMenu();
    } while( u8g.nextPage() );
  }
  else 
  {
    do {
    updateTemperatures();
    } while( u8g.nextPage() );
  }
  
  delay(20);
}

void updateTemperatures() 
{
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(0,22);
  u8g.print("Set temp: ");
  u8g.setPrintPos(80,22);
  u8g.print(String(setTemp));
  u8g.setPrintPos(0,44);
  u8g.print("Actual: ");
  u8g.setPrintPos(80,44);
  u8g.print(String(actualTemp));
}

void drawSetupMenu() 
{
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(0,22);
  u8g.print("Settings:");

}

void checkButtons()
{
  if(!digitalRead(PIN_SW_TEMP_UP))
  {
    setTemp=(setTemp>=MAX_TEMP)? MAX_TEMP : setTemp+1;
    saveTempChange=true;
    lastSetTempChangeTime=millis();
  }
  else if(!digitalRead(PIN_SW_TEMP_DN))
  {
    setTemp=(setTemp<=MIN_TEMP)? MIN_TEMP : setTemp-1;
    saveTempChange=true;
    lastSetTempChangeTime=millis();
  }
  else if(!digitalRead(PIN_SW_SETUP))
  {
    setupMode=!setupMode;
  }
}

void updateRelay()
{
  if(setTemp>actualTemp)
  {
    digitalWrite(PIN_RLY_CTRL, HIGH);
  }
  else
  {
    digitalWrite(PIN_RLY_CTRL, LOW);
  }
}