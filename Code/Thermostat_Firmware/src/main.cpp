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
#define DEFAULT_TEMP 50
#define MAX_HYSTERESIS 50
#define DEFAULT_HYSTERESIS 5
#define DEFAULT_MODE 1 // 0=Thermometer, 1=Thermostat, 2=Thermofuse


MAX6675 max6675(PIN_MAX6675_CLK, PIN_MAX6675_CS, PIN_MAX6675_DO);
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
uint16_t setTemp, storedTemp, actualTemp;
uint32_t lastSetTempChangeTime, lastActualTempRead, lastMenuItemChange;
uint8_t setHysteresis, storedHysteresis, setMode, storedMode, selectedMenuItem;
bool saveTempChange;

void updateTemperatures();
void drawSetupMenu();
void checkButtons();
void updateRelay();
void saveTempToEEPROM();
void readEEPROM();
void updateDisplay();

void setup() 
{
  pinMode(PIN_RLY_CTRL, OUTPUT);
  pinMode(PIN_SW_TEMP_UP, INPUT_PULLUP);
  pinMode(PIN_SW_TEMP_DN, INPUT_PULLUP);
  pinMode(PIN_SW_SETUP, INPUT_PULLUP);
  Serial.begin(115200);
  readEEPROM();
  u8g.setColorIndex(1);
}

void loop() 
{
  if(saveTempChange && (millis()-lastSetTempChangeTime>5000))
  {
    saveTempToEEPROM();
  }
  if(millis()-lastActualTempRead>500)
  {
    actualTemp=max6675.readCelsius();  
    lastActualTempRead=millis();
    updateRelay();
  }
  checkButtons();
  updateDisplay();
}

void updateTemperatures() 
{
  u8g.setFont(u8g_font_unifont);
  u8g.setDefaultForegroundColor();
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
  u8g.setDefaultForegroundColor();
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(0,13);
  u8g.print("Mode:");
  if(selectedMenuItem==1)
  {
    u8g.drawBox(40, 0, 88, 15);
    u8g.setDefaultBackgroundColor();
  }
  u8g.setPrintPos(40,13);
  switch (setMode)
  {
  case 0:
    u8g.print("Thermometer");
    break;
  case 1:
    u8g.print("Thermostat");
    break;
  case 2:
    u8g.print("Thermofuse");
    break;
  default:
    break;
  }
  if(setMode==1)
  {
    u8g.setDefaultForegroundColor();
    u8g.setPrintPos(0,27);
    u8g.print("Hysteresis:");
    if(selectedMenuItem==2)
    {
      u8g.drawBox(90, 15, 20, 13);
      u8g.setDefaultBackgroundColor();
    }
    u8g.setPrintPos(92,26);
    if(setHysteresis<10) u8g.print("0");
    u8g.print(setHysteresis);
  }
  
}

void checkButtons()
{
  if(!digitalRead(PIN_SW_TEMP_UP))
  {
    if((selectedMenuItem==0)&&(millis()-lastSetTempChangeTime>100))
    {
      setTemp=(setTemp>=MAX_TEMP)? MAX_TEMP : setTemp+1;
      saveTempChange=true;
      lastSetTempChangeTime=millis();
    }
    else if((selectedMenuItem==1) && ((millis()-lastMenuItemChange)>1000))
    {
      setMode++;
      if(setMode>2) setMode=0;
      lastMenuItemChange=millis();
    }
  }
  else if(!digitalRead(PIN_SW_TEMP_DN))
  {
    if((selectedMenuItem==0)&&(millis()-lastSetTempChangeTime>100))
    {
      setTemp=(setTemp<=MIN_TEMP)? MIN_TEMP : setTemp-1;
      saveTempChange=true;
      lastSetTempChangeTime=millis();
    }
    
  }
  else if((!digitalRead(PIN_SW_SETUP))&&((millis()-lastMenuItemChange)>1000))
  {
    if((selectedMenuItem++)>=2) selectedMenuItem=0;
    if((setMode!=1) && (selectedMenuItem==2)) selectedMenuItem=0;
    Serial.print("Mode: ");
    Serial.println(setMode);
    Serial.print("Menu item: ");
    Serial.println(selectedMenuItem);
    lastMenuItemChange=millis();
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

void saveTempToEEPROM()
{
  Serial.println("Save to EEPROM");
  EEPROM.write(0, highByte(setTemp));
  EEPROM.write(1, lowByte(setTemp));
  saveTempChange=false;
}

void readEEPROM()
{
  storedTemp=(EEPROM.read(0)<<8) | EEPROM.read(1);
  setTemp=(storedTemp>MIN_TEMP && storedTemp<=MAX_TEMP)?storedTemp : DEFAULT_TEMP;
  Serial.print("Stored temp: ");
  Serial.println(storedTemp);
  storedHysteresis=EEPROM.read(2);
  Serial.print("Stored hysteresis: ");
  Serial.println(storedHysteresis);
  setHysteresis=(storedHysteresis<=MAX_HYSTERESIS)?storedHysteresis : DEFAULT_HYSTERESIS;
  storedMode=EEPROM.read(3);
  setMode=(storedMode<3)?storedMode : DEFAULT_MODE;
  Serial.print("Stored mode: ");
  Serial.println(storedMode);
  Serial.print("Settings: ");
  Serial.println(setMode);
}

void updateDisplay()
{
  u8g.firstPage(); 
  if(selectedMenuItem>0)
  {
    do {
    drawSetupMenu();
    } while(u8g.nextPage());
  }
  else 
  {
    do {
    updateTemperatures();
    } while(u8g.nextPage());
  }
}