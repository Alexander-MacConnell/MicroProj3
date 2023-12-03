/*
  This program is for Microprocessors 2 Lab #3.
  This system utilizes a DC motor and motor controller,
  a sound module, an LCD display, a real time clock, and
  a microcontroller. A4 and C4 note frequencies are 
  detected by the sound module and will increase or
  decrease the motor speed. The real time clock will
  send accurate time readings to the system. The LCD
  will display the date, time, motor direction and motor
  speed. The display will be updated every second via
  a timer interupt.

  12/3/2023
  by Andrew Woods, Alex MacConnell, and Omar Said
*/

#include <Arduino.h>        // Libraries required for program
#include <Wire.h>           // Required for I2C comms
#include <RTClib.h>         // Real time clock library
#include <SPI.h>        
#include <arduinoFFT.h>     // Fast Fourier Transform library
#include <LiquidCrystal.h>  // Library required for LCD Display
#include <string.h>         // String library
#include <avr/interrupt.h>  // Required for ISR

#define ENABLE 5                  // Define Enable DIRA and DIRB pins for motor controller on Arduino
#define DIRA 3
#define DIRB 4
#define Analog_Mic_Pin A0         // Pin for mic analog input

#define SAMPLES 64                //Must be a power of 2
#define SAMPLING_FREQUENCY 1000   //Max freq is half this

// Define accepted frequency ranges for A4 and C4
#define C4_lower 257  
#define C4_upper 268
#define A4_lower 431
#define A4_upper 449

RTC_DS1307 rtc;                           // Initialize real time clock

arduinoFFT FFT = arduinoFFT();            // Initialize Fast Fourier Transform variable

LiquidCrystal lcd(7, 8, 9, 10 , 11, 12);  // Initializes display output ports for LCD

// Used for time and date string concatenation
String slash = "/";
String colon = ":";
String space = " ";

// Track Time Globally
int year;
int month;
int day;
int hour;
int minute;
int second;
String date;  // String concatenated to display the date
String time;  // String concatenated to display time

// Global Variables to track motor speed and direction
String direction = "NA";
String speed_string = "NA";
int motor_speed = 0;        // int value from 0-255 to set motor speed. Initialize to 0 since motor is not moving


// Global strings to send to display
// Declared as volatile since they will be modified in the ISR
String display_date = "Empty";
String display_time = "Empty";
String display_direction = "NA";
String display_speed = "NA";

// Function declarations
int decrease_speed(int speed);
int increase_speed(int speed);

void setup(){
  Serial.begin(9600);                                               // Set Serial comm baud rate

  // Timer1 Interrupt Setup
  cli();  // Disable interrupts

  // Set Timer1 to Clear Timer on Compare Match (CTC) Mode
  TCCR1A = 0;               // Timer/Counter Control Registers A and B are 8-bit registers that control the operation of the Timer1 user for interrupts
  TCCR1B = 0;
  TCCR1B |= (1 << WGM12);

  // Sets Timer1 to divide clock by 1024
  TCCR1B |= (1 << CS12) | (1 << CS10);

  // Sets timer to compare match every second
  OCR1A = 15624;    // Output compare register A sets the value that the Timer1 counter is compared against
  
  // Enable timer comparison interrupt
  TIMSK1 |= (1 << OCIE1A);    // Timer/Counter1 Interrupt Mask Register used to enable or disable interrupts for Timer1

  // Enable interrupts
  sei();

  int sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY)); // Stores sampling period in micro seconds

  // Checks that real time clock is initiated
  if (! rtc.begin()){             
    Serial.println("Couldn't find RTC");
    while (1);
  }

  // Sets pin direction for motor outputs and mic input
  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);
  pinMode(Analog_Mic_Pin, INPUT);

  // Checks if the Real time clock is running and adjusts the clock to match the actual date and time
  if (! rtc.isrunning()){
    Serial.println("RTC is NOT running!");
    // Following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Sets LCD display to have 16 columns and 2 rows
  lcd.begin(16,2); 
}

// Interrupt Service Routine interrupts loop every second to update the display
ISR(TIMER1_COMPA_vect){
  // Set display values equal to current loop readings
  display_date = date;
  display_time = time;
  display_direction = direction;
  display_speed = speed_string;
}

void loop() {
  int i;
  unsigned int sampling_period_us;
  unsigned long microseconds;
  double vReal[SAMPLES];
  double vImag[SAMPLES];
  double peak;

  // Get current date and time
  DateTime now = rtc.now();
  year = now.year();
  month = now.month();
  day = now.day();
  hour = now.hour();
  minute = now.minute();
  second = now.second();

  // Concatenate strings to display the date and time in US format
  date = month + slash + day + slash + year + space;
  time = hour + colon + minute + colon + second + space;

  microseconds = micros();
  for(int i=0; i<SAMPLES; i++){
    vReal[i] = analogRead(A0);
    vImag[i] = 0;

    while(micros() - microseconds < sampling_period_us){
      //empty loop
    }
    microseconds += sampling_period_us;
  }

  // Perform FFT and find max frequency from mic reading
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  
  // Check if frequency is within a range and changes motor speed accordingly
  if(peak >= A4_lower && peak <= A4_upper){
    Serial.print(peak);
    Serial.println("- Decreasing Fan Speed by 1 step...");
    motor_speed = decrease_speed(motor_speed);  // Calls function to decrease the speed setting
  }
  else if(peak >= C4_lower && peak <= C4_upper){
    Serial.print(peak);
    Serial.println("- Increasing Fan Speed by 1 step...");
    motor_speed = increase_speed(motor_speed);  // Calls function to increase the speed setting
  }
  else{
    Serial.print(peak);
    Serial.println("- Not in range...");
  }
  
  // Sets motor direction
  digitalWrite(DIRA, HIGH); 
  digitalWrite(DIRB, LOW);
  direction = "FD=CC";      // Fan direction = CC

  analogWrite(ENABLE, 0);   // *** CHANGE 0 to motor_speed when microphone function works properly ***

  // Display Values
  lcd.setCursor(0,0);   // Set LCD cursor to first spot on display 
  lcd.print(display_date);
  lcd.print(display_direction);
  lcd.setCursor(0,1);
  lcd.print(display_time);
  lcd.print(display_speed);

  delay(1000);
}

// Function utilizes a switch statement to determine the new motor speed
int increase_speed(int speed){
  switch (speed){
    case 0:
      speed_string = "FS=1/2";
      return 128;
      break;
    
    case 128:
      speed_string = "FS=3/4";
      return 192;
      break;
    
    case 192:
      speed_string = "FS=FULL";
      return 255;
      break;
    
    case 255:       // Cannot go faster than full speed
      return 255;
      break;
  }
}

// Function decreases motor speed based on current speed
int decrease_speed(int speed){
  switch (speed){
    case 255:
      speed_string = "FS=3/4";
      return 192;
      break;
    
    case 192:
      speed_string = "FS=1/2";
      return 128;
      break;
    
    case 128:
      speed_string = "FS=0";
      return 0;
      break;
    
    case 0:       // Cannot go slower than fully stopped
      return 0;
      break;
  }
}