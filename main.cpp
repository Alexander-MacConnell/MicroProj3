#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <arduinoFFT.h>

#define ENABLE 5
#define DIRA 3
#define DIRB 4

RTC_DS1307 rtc;
int i;
 
void setup() {

  Serial.begin(9600);

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

}

void loop() {

  DateTime now = rtc.now();

  Serial.println("PWM full then slow");

  digitalWrite(DIRA, HIGH);
  digitalWrite(DIRB, LOW);

  delay(1000);

  analogWrite(ENABLE, 120);
  delay(1000);

  analogWrite(ENABLE, 180);
  delay(1000);

  delay(1000);
  analogWrite(ENABLE, 255);
  delay(1000);

  digitalWrite(ENABLE,LOW);
  delay(3000);

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  
}