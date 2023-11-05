#include <Arduino.h>
#include <max6675.h>
#include <EEPROM.h>
#include <U8glib.h>
#define PIN_RLY_CTRL 9
#define PIN_MAX6675_DO 12
#define PIN_MAX6675_CS 10
#define PIN_MAX6675_CLK 13
#define PIN_SW_TEMP_UP 5
#define PIN_SW_TEMP_DN 3
#define PIN_SW_SETUP 6
#define PIN_SW_PRESET1 7
#define PIN_SW_PRESET2 2
#define PIN_LED_HITEMP A2
#define PIN_LED_ERROR A3
#define MAX_SET_TEMP 1000
#define MIN_SET_TEMP 0
#define DEFAULT_TEMP 50
#define MAX_HYSTERESIS 20
#define DEFAULT_HYSTERESIS 5
#define DEFAULT_OFFSET 0
#define MAX_OFFSET 20
#define DEFAULT_MODE 1 // 0=Thermometer, 1=Thermostat, 2=Thermofuse
#define ADDR_TEMP0 0
#define ADDR_TEMP1 1
#define ADDR_MODE 2
#define ADDR_OFFSET 3
#define ADDR_HYSTERESIS 4

MAX6675 max6675(PIN_MAX6675_CLK, PIN_MAX6675_CS, PIN_MAX6675_DO);
U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE|U8G_I2C_OPT_DEV_0);
int16_t actualTemp, setTemp, storedTemp, maxTemp, minTemp; 
uint32_t lastSetTempChangeTime, lastActualTempRead, lastMenuItemChange;
uint8_t hysteresis, storedHysteresis, selectedMenuItem, mode, storedMode;
uint8_t preset1counter, preset2counter, message1counter, message2counter, tempsetCounter;
int8_t offset, storedOffset;
bool saveTempChange, overtempReached, sensorError;

void drawTemperatures();
void drawSetupMenu();
void handleButtons();
void handleTemperature();
void saveTempToEEPROM(uint8_t presetId);
void saveSettingsToEEPROM(uint8_t presetId);
void clearEEPROM();
void readEEPROM(uint8_t presetId);
void updateDisplay();
void viewEEPROMdata();


void setup() 
{
  pinMode(PIN_RLY_CTRL, OUTPUT);
  pinMode(PIN_LED_HITEMP, OUTPUT);
  pinMode(PIN_LED_ERROR, OUTPUT);
  pinMode(PIN_SW_TEMP_UP, INPUT_PULLUP);
  pinMode(PIN_SW_TEMP_DN, INPUT_PULLUP);
  pinMode(PIN_SW_SETUP, INPUT_PULLUP);
  pinMode(PIN_SW_PRESET1, INPUT_PULLUP);
  pinMode(PIN_SW_PRESET2, INPUT_PULLUP);
  Serial.begin(115200);
  readEEPROM(0);
  u8g.setColorIndex(1);
  minTemp=1000;
  delay(500);
  // viewEEPROMdata();
  // clearEEPROM();
}

void loop() 
{
  if(saveTempChange && (millis()-lastSetTempChangeTime>5000))
  {
    saveTempToEEPROM(0);
  }
  if(millis()-lastActualTempRead>500)
  {
    handleTemperature();
  }
  handleButtons();
  updateDisplay();
}


void drawTemperatures() 
{
  u8g.setFont(u8g_font_fur35n);
  u8g.setDefaultForegroundColor();
  u8g.drawHLine(0,47,127);
  u8g.drawVLine(80,0,48);
  u8g.setPrintPos(1,40);
  if(!sensorError) u8g.print(actualTemp);
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(58,38);
  u8g.print(char(0x0b0));
  u8g.print("C");
  u8g.setPrintPos(88,10);
  u8g.print("Max:");
  u8g.setPrintPos(88,22);
  u8g.print(maxTemp);
  u8g.setPrintPos(88,34);
  u8g.print("Min:");
  u8g.setPrintPos(88,46);
  u8g.print(minTemp);
  u8g.setPrintPos(0,61);
  if(sensorError)
  {
    u8g.print("Sensor error");
  }
  else if(preset1counter>100)
  {
    u8g.print("Saved preset 1");
  }
  else if(preset2counter>100)
  {
    u8g.print("Saved preset 2");
  }
  else if((mode==2)&&overtempReached)
  {
    u8g.print("Overtemp.");
  }
  else if(mode>0)
  {
    u8g.print("Set:");
    if(setTemp<100) u8g.print(" ");
    u8g.print(setTemp);
  }
}

void drawSetupMenu() 
{
  u8g.setDefaultForegroundColor();
  u8g.setFont(u8g_font_unifont);
  u8g.setPrintPos(0,12);
  u8g.print("Mode:");
  if(selectedMenuItem==1)
  {
    u8g.drawBox(40, 0, 88, 14);
    u8g.setDefaultBackgroundColor();
  }
  u8g.setPrintPos(40,12);
  switch (mode)
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
  u8g.setDefaultForegroundColor();
  u8g.setPrintPos(0,29);
  u8g.print("Offset:");
  if(selectedMenuItem==2)
  {
    u8g.drawBox(86, 16, 24, 14);
    u8g.setDefaultBackgroundColor();
  }
  u8g.setPrintPos(86,29);
  if((offset>=0)&&(offset<10)) u8g.print("  ");
  else if((offset>=10)||(offset>-10)) u8g.print(" ");
  
  u8g.print(offset);
  u8g.setDefaultForegroundColor();
  u8g.print(char(0x0b0));
  u8g.print("C");
  if(mode==1)
  {
    u8g.setDefaultForegroundColor();
    u8g.setPrintPos(0,45);
    u8g.print("Hysteresis:");
    if(selectedMenuItem==3)
    {
      u8g.drawBox(90, 33, 20, 14);
      u8g.setDefaultBackgroundColor();
    }
    u8g.setPrintPos(94,45);
    if(hysteresis<10) u8g.print(" ");
    u8g.print(hysteresis);
    u8g.setDefaultForegroundColor();
    u8g.print(char(0x0b0));
    u8g.print("C");
  }
  u8g.setPrintPos(0,61);
  if(preset1counter>100)
  {
    u8g.print("Saved preset 1");
  }
  else if(preset2counter>100)
  {
    u8g.print("Saved preset 2");
  }
  
}

void handleButtons()
{
  if(!digitalRead(PIN_SW_TEMP_UP))
  {
    if((selectedMenuItem==0)&&(millis()-lastSetTempChangeTime>100))
    {
      tempsetCounter=(tempsetCounter<10)?tempsetCounter+1 : 10;
      if(tempsetCounter<10) setTemp=(setTemp>=MAX_SET_TEMP)? MAX_SET_TEMP : setTemp+1;
      else setTemp=(setTemp>=(MAX_SET_TEMP-10))? MAX_SET_TEMP : setTemp+10;
      saveTempChange=true;
      lastSetTempChangeTime=millis();
    }
    else if((selectedMenuItem==1) && ((millis()-lastMenuItemChange)>500))
    {
      if(mode<2) mode++;
      lastMenuItemChange=millis();
    }
    else if((selectedMenuItem==2) && ((millis()-lastMenuItemChange)>500))
    {
      if(offset<MAX_OFFSET) offset++;
      lastMenuItemChange=millis();
    }
    else if((selectedMenuItem==3) && ((millis()-lastMenuItemChange)>500))
    {
      if(hysteresis<MAX_HYSTERESIS) hysteresis++;
      lastMenuItemChange=millis();
    }
  }
  else if(!digitalRead(PIN_SW_TEMP_DN))
  {
    if((selectedMenuItem==0)&&(millis()-lastSetTempChangeTime>100))
    {
      tempsetCounter=(tempsetCounter<10)?tempsetCounter+1 : 10;
      if(tempsetCounter<10) setTemp=(setTemp<=MIN_SET_TEMP)? MIN_SET_TEMP : setTemp-1;
      else setTemp=(setTemp<=(MIN_SET_TEMP+10))? MIN_SET_TEMP : setTemp-10;
      saveTempChange=true;
      lastSetTempChangeTime=millis();
    }
    else if((selectedMenuItem==1) && ((millis()-lastMenuItemChange)>500))
    {
      if(mode>=1) mode--;
      lastMenuItemChange=millis();
    }
    else if((selectedMenuItem==2) && ((millis()-lastMenuItemChange)>500))
    {
      if(offset>(-MAX_OFFSET)) offset--;
      lastMenuItemChange=millis();
    }
    else if((selectedMenuItem==3) && ((millis()-lastMenuItemChange)>500))
    {
      if(hysteresis>0) hysteresis--;
      lastMenuItemChange=millis();
    }
  }
  else if((!digitalRead(PIN_SW_SETUP))&&((millis()-lastMenuItemChange)>500))
  {
    if(((selectedMenuItem++)>=3)||((mode!=1) && (selectedMenuItem==3))) 
    {
      selectedMenuItem=0;
      saveSettingsToEEPROM(0);
    }
    // Serial.print("Mode: ");
    // Serial.println(mode);
    // Serial.print("Menu item: ");
    // Serial.println(selectedMenuItem);
    lastMenuItemChange=millis();
  }
  else if(!digitalRead(PIN_SW_PRESET1)&&(preset1counter<100))
  {
    if ((preset1counter++)>8)
    {
      saveSettingsToEEPROM(1);
      saveTempToEEPROM(1);
      preset1counter=110;
    }
  }
  else if((digitalRead(PIN_SW_PRESET1))&&(preset1counter>0))
  {
    if(preset1counter>100) preset1counter--;
    else if(preset1counter>10) preset1counter=0;
    else
    {
      readEEPROM(1);
      saveSettingsToEEPROM(0);
      saveTempToEEPROM(0);
      preset1counter=0;
    }
  } 
  else if(!digitalRead(PIN_SW_PRESET2)&&(preset2counter<100))
  {
    if ((preset2counter++)>8)
    {
      saveSettingsToEEPROM(2);
      saveTempToEEPROM(2);
      preset2counter=110;
    }
  }
  else if((digitalRead(PIN_SW_PRESET2))&&(preset2counter>0))
  {
    if(preset2counter>100) preset2counter--;
    else if(preset2counter>10) preset2counter=0;
    else
    {
      readEEPROM(2);
      saveSettingsToEEPROM(0);
      saveTempToEEPROM(0);
      preset2counter=0;
    }
  } 
  else if((digitalRead(PIN_SW_TEMP_DN))&&(digitalRead(PIN_SW_TEMP_UP)))
  {
    tempsetCounter=0;
  }
  
}

void handleTemperature()
{
  actualTemp=max6675.readCelsius(); 
  lastActualTempRead=millis();
  if(actualTemp<0) 
  {
    digitalWrite(PIN_LED_ERROR, HIGH);
    sensorError=true;
    digitalWrite(PIN_RLY_CTRL, LOW);
  }
  else
  {
    actualTemp=((actualTemp+offset)>0)? actualTemp+offset : 0;
    maxTemp=(maxTemp<actualTemp)?actualTemp:maxTemp;
    minTemp=(minTemp>actualTemp)?actualTemp:minTemp;
    digitalWrite(PIN_LED_ERROR, LOW);
    sensorError=false;
  }
  switch (mode)
  {
  case 0:
    digitalWrite(PIN_RLY_CTRL, LOW);
    digitalWrite(PIN_LED_HITEMP, LOW);
    break;
  case 1:
    if(((actualTemp<=setTemp)&&!overtempReached)||(actualTemp<=(setTemp-hysteresis)))
    {
      digitalWrite(PIN_RLY_CTRL, HIGH);
      overtempReached=false;
    }
    else
    {
      digitalWrite(PIN_RLY_CTRL, LOW);
      overtempReached=true;
      digitalWrite(PIN_LED_HITEMP, HIGH);
    }
    digitalWrite(PIN_LED_HITEMP, (actualTemp>setTemp));
    break;
  case 2:
    if((!overtempReached)&&(actualTemp<setTemp))
    {
      digitalWrite(PIN_RLY_CTRL, HIGH);
    }
    else
    {
      overtempReached=true;
      digitalWrite(PIN_RLY_CTRL, LOW);
      digitalWrite(PIN_LED_HITEMP, HIGH);
    }
  default:
    break;
  }
}

void saveTempToEEPROM(uint8_t presetId)
{
  Serial.print("Save temp to EEPROM, presetId ");
  Serial.println(presetId);
  EEPROM.write(ADDR_TEMP0+5*presetId, highByte(setTemp));
  EEPROM.write(ADDR_TEMP1+5*presetId, lowByte(setTemp));
  saveTempChange=false;
}

void saveSettingsToEEPROM(uint8_t presetId)
{
  Serial.print("Save settings to EEPROM, presetId ");
  Serial.println(presetId);
  EEPROM.write(ADDR_HYSTERESIS+5*presetId, hysteresis);
  EEPROM.write(ADDR_MODE+5*presetId, mode);
  EEPROM.write(ADDR_OFFSET+5*presetId, offset);
}

void clearEEPROM()
{
  for(uint8_t i; i<16; i++)
  {
    EEPROM.write(i, 255);
  }
}

void viewEEPROMdata()
{
  for(uint8_t i; i<16; i++)
  {
    Serial.print("Addr ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(EEPROM.read(i));
  }
}

void readEEPROM(uint8_t presetId)
{
  Serial.print("Read EEPROM, preset");
  Serial.println(presetId);
  storedTemp=(EEPROM.read(ADDR_TEMP0+5*presetId)<<8) | EEPROM.read(ADDR_TEMP1+5*presetId);
  setTemp=(storedTemp>MIN_SET_TEMP && storedTemp<=MAX_SET_TEMP)?storedTemp : DEFAULT_TEMP;
  // Serial.print("Stored temp: ");
  // Serial.println(storedTemp);
  // Serial.print("Set temp: ");
  // Serial.println(setTemp);

  storedHysteresis=EEPROM.read(ADDR_HYSTERESIS+5*presetId);
  hysteresis=(storedHysteresis<=MAX_HYSTERESIS)?storedHysteresis : DEFAULT_HYSTERESIS;
  // Serial.print("Stored hysteresis: ");
  // Serial.println(storedHysteresis);
  // Serial.print("Hysteresis: ");
  // Serial.println(hysteresis);

  storedOffset=EEPROM.read(ADDR_OFFSET+5*presetId);
  offset=(storedOffset<=MAX_OFFSET)?storedOffset : DEFAULT_OFFSET;
  // Serial.print("Stored offset: ");
  // Serial.println(offset);
  // Serial.print("Offset: ");
  // Serial.println(offset);

  storedMode=EEPROM.read(ADDR_MODE+5*presetId);
  mode=(storedMode<3)?storedMode : DEFAULT_MODE;
  // Serial.print("Stored mode: ");
  // Serial.println(storedMode);
  // Serial.print("Mode: ");
  // Serial.println(mode);
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
    drawTemperatures();
    } while(u8g.nextPage());
  }
}