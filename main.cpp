#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <arduinoFFT.h>

#define ENABLE 5
#define DIRA 3
#define DIRB 4

#define SAMPLES 64             //Must be a power of 2
#define SAMPLING_FREQUENCY 1000 //Max freq is half this

RTC_DS1307 rtc;
int i;
unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[SAMPLES];
double vImag[SAMPLES];

arduinoFFT FFT = arduinoFFT();
 
void setup() {

  Serial.begin(9600);

  sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);
  pinMode(PIN_A0, INPUT);

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

}

void loop() {

  DateTime now = rtc.now();

  microseconds = micros();
  for(int i=0; i<SAMPLES; i++){

    vReal[i] = analogRead(A0);
    vImag[i] = 0;

    while(micros() - microseconds < sampling_period_us){
      //empty loop
    }

    microseconds += sampling_period_us;

  }

  /*Perform FFT*/
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  /*Print out frequency*/
  Serial.println(peak);
  
  delay(1000);

  /*

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
  
  */
  
}