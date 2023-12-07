#include <Arduino.h>        // Libraries required for program
#include <Wire.h>           // Required for I2C comms
#include <RTClib.h>         // Real time clock library
#include <SPI.h>        
#include <arduinoFFT.h>     // Fast Fourier Transform library
#include <LiquidCrystal.h>  // Library required for LCD Display
#include <string.h>         // String library
#include <avr/interrupt.h>  // Required for ISR

#define ENABLE 5
#define DIRA 3
#define DIRB 4

#define SAMPLES 64             //Must be a power of 2
#define SAMPLING_FREQUENCY 1000 //Max freq is half this

// Define accepted frequency ranges for A4 and C4
#define C4_lower 257  
#define C4_upper 268
#define A4_lower 431
#define A4_upper 449

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
String display_time = "Empty";
String display_direction = "NA";
String display_speed = "FS=0";

RTC_DS1307 rtc;
int i;
int x = 0;
double peak = 0.0;
double pervious_peak = 0.0;
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

  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  pinMode(ENABLE, OUTPUT);
  pinMode(DIRA, OUTPUT);
  pinMode(DIRB, OUTPUT);
  pinMode(PIN_A0, INPUT);

  // Sets motor direction
  digitalWrite(DIRA, HIGH); 
  digitalWrite(DIRB, LOW);
  direction = " FD=CC";      // Fan direction = CC

  // Sets LCD display to have 16 columns and 2 rows
  lcd.begin(16,2);


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

}


ISR(TIMER1_COMPA_vect){
  
  // Set display values equal to current loop readings
  display_time = time;
  display_direction = direction;

  // Display Values
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(display_time);
  lcd.setCursor(0,1);
  lcd.print(display_speed);
  lcd.print(display_direction);

}


void loop() {

  pervious_peak = peak;

  microseconds = micros();
  for(int i=0; i<SAMPLES; i++){

    vReal[i] = analogRead(A0);
    vImag[i] = 0;

    while(micros() - microseconds < sampling_period_us){
      //empty loop
    }

    microseconds += sampling_period_us;
  }

  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);

  if(!((peak >= A4_lower && peak <= A4_upper) || (peak >= C4_lower && peak <= C4_upper))){
    peak = 0;
  }

  /*Print out frequency*/
  //Serial.println(peak);

  // Check if frequency is within a range and changes motor speed accordingly
  if(peak >= A4_lower && peak <= A4_upper){

    if((x > 5) && (peak < pervious_peak + 10) && (peak > pervious_peak - 10) && (motor_speed > 135)){
      motor_speed = motor_speed - 60;
      analogWrite(ENABLE, motor_speed);

      if(motor_speed == 195){
        display_speed = "FS=3/4";
      }
      else{
        display_speed = "FS=1/2";
      }

      x = 0;
    }

    if((x > 5) && (peak < pervious_peak + 10) && (peak > pervious_peak - 10) && (motor_speed == 135)){
      motor_speed = 0;
      analogWrite(ENABLE, motor_speed);
      display_speed = "FS=0";
      x = 0;
    }

    x++;

  }

  if(peak >= C4_lower && peak <= C4_upper && motor_speed < 255){

    if((x > 5) && (peak < pervious_peak + 10) && (peak > pervious_peak - 10) && (motor_speed >= 135)){
      motor_speed = motor_speed + 60;
      analogWrite(ENABLE, motor_speed);

      if(motor_speed == 255){
        display_speed = "FS=Full";
      }
      else{
        display_speed = "FS=3/4";
      }

      x = 0;
    }

    if((x > 5) && (peak < pervious_peak + 10) && (peak > pervious_peak - 10) && (motor_speed == 0)){
      motor_speed = 135;
      analogWrite(ENABLE, motor_speed);
      display_speed = "FS=1/2";
      x = 0;
    }

    x++;

  }

  // Get current date and time
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
  second = now.second();

  // Concatenate strings to display the date and time in US format
  date = month + slash + day + slash + year + space;
  time = hour + colon + minute + colon + second + space;

  delay(200);
  
}