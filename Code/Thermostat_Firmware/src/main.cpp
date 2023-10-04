#include <Arduino.h>
#include <max6675.h>
#define LED_TEMP A2
#define LED_STATUS A3

int thermoDO = 12;
int thermoCS = 10;
int thermoCLK = 13;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);


void setup() 
{
  pinMode(LED_TEMP, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_TEMP, HIGH);
  digitalWrite(LED_STATUS, HIGH);
  Serial.begin(9600);
  Serial.println("MAX6675 test");
  // wait for MAX chip to stabilize
  delay(500);
}

void loop() 
{
  Serial.print("\xB0 C = "); 
  Serial.println(thermocouple.readCelsius());
  delay(1000); 
}

