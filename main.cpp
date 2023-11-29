#include <Arduino.h>
#include <arduinoFTT.h>

//---L298N motor driver example

#define ENABLE 5
#define DIRA 3
#define DIRB 4

int i;
 
void setup() {
  //---set pin direction
  pinMode(ENABLE,OUTPUT);
  pinMode(DIRA,OUTPUT);
  pinMode(DIRB,OUTPUT);
  Serial.begin(9600);
}

void loop() {

  Serial.println("PWM full then slow");

  digitalWrite(DIRA,HIGH);
  digitalWrite(DIRB,LOW);

  delay(1000);

  analogWrite(ENABLE, 100);
  delay(1000);

  analogWrite(ENABLE, 180);
  delay(1000);

  delay(1000);
  analogWrite(ENABLE, 255);
  delay(1000);

  digitalWrite(ENABLE,LOW); //all done
  delay(10000);
  
}