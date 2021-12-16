//LIBRARIES
#include <Arduino.h>
#include <TM1637Display.h>
#include "RTClib.h"

//CONSTANTS
//pins
const uint8_t b1Pin = 5;
const uint8_t b2Pin = 2;
const uint8_t b3Pin = 3;
const uint8_t b4Pin = 4;
const uint8_t switchPin = 8;
const uint8_t lGreenPin = 6;
const uint8_t buzzerPin = 11;
#define CLK 12
#define DIO 10
//brightness
const int displayBrightness = 2;
const int lGreenBrightness = 15;

//CREATE VARIABLES
//Mode manipulation
int mode = 0; //0-show time, 1-set alarm, 2-execute alarm
int m1Setting = 0; //0-set hours, 1-set minutes
int m1Cycle = 0; 
int m0Cycle = 0;
int lGreenBlink = 0;
//Pin states
bool b1State = 0;
bool b2State = 0;
bool b3State = 0;
bool b4State = 0;
bool switchState = 0;
//Buzzer
int buzzerPitch = 1000;
int buzzerDelay = 500;
int buzzerDelay2 = 100;
int buzzerDuration = 500;
int buzzerDuration2 = 100;
int currentBuzzer = 0;
//Rtc
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//Alarm
bool alarmState = 0;
int alarmHour = 0;
int alarmMinute = 0;

//CREATE OBJECTS
TM1637Display display = TM1637Display(CLK, DIO);
RTC_DS3231 rtc;

//DEFINE FUNCTIONS
void checkButtons(){ //checks states of all buttons and switches and stores them in variables (the states are flipped, because I am using internal pullup resistors)
  b1State = !digitalRead(b1Pin);
  b2State = !digitalRead(b2Pin);
  b3State = !digitalRead(b3Pin);
  b4State = !digitalRead(b4Pin);
  switchState = !digitalRead(switchPin);
}
void displayTime(){ //displays current time on the display
  DateTime now = rtc.now();

  
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.println();
  
  
  display.showNumberDecEx(now.hour(), 0b11100000, true, 2, 0);
  display.showNumberDecEx(now.minute(), 0b11100000, true, 2, 2);
}
void displayAlarm(){ //displays the time of the alarm
  /*
  Serial.print(alarmHour);
  Serial.print(':');
  Serial.print(alarmMinute);
  Serial.println();
  */

  display.showNumberDecEx(alarmHour, 0b11100000, true, 2, 0);
  display.showNumberDecEx(alarmMinute, 0b11100000, true, 2, 2);
}
void checkAlarmSwitch(){ //checks if alarm is enabled or not, than it sets the alarmState variable and manages the green led accordingly
  if(switchState && alarmHour + alarmMinute != 0){
    analogWrite(lGreenPin, lGreenBrightness);
    alarmState = 1; 
  } else{
    analogWrite(lGreenPin, 0);
    alarmState = 0; 
  }
}
void switchM1Setting(){ //adds 1 to m1Setting, if it is bigger than 1 it changes mode to 0(shows time normally)
  m1Setting ++;
  //m1Cycle = 0;
  if(m1Setting == 2){
    mode = 0;
    tone(buzzerPin, 1500, 400);
  }else{
    tone(buzzerPin, 1500, 100);
  }
}
void alarmAdjustment(){ //checks if the buttons + or - are pressed, than it adjusts the alarm hour or minute(that is declared by m1Setting)
  if(b2State){
    if(m1Setting == 0){
      alarmHour ++;
      if(alarmHour > 23){
        alarmHour = 0;
      }
    } else{
      alarmMinute ++;
      if(alarmMinute > 59){
        alarmMinute = 0;
      }
    }
  }else if(b3State){
    if(m1Setting == 0){
      alarmHour --;
      if(alarmHour < 0){
        alarmHour = 23;
      }
    }else{
      alarmMinute --;
      if(alarmMinute < 0){
        alarmMinute = 59;
      }
    }
  }
}
void blinkLGreen(){ //simply inverts the state of the green led
  if(lGreenBlink){
    digitalWrite(lGreenPin, 0);
    lGreenBlink = 0;
  }else{
    digitalWrite(lGreenPin, 1);
    lGreenBlink = 1;
  }
}
void startAlarm(){ //starts the alarm by changing the mode to 2
  mode = 2;
}

void makeNoise(){ //makes the passive buzzer do annoying things to wake you up (it beeps), also it has two """melodies""" available, the second switch determines which one should be playing
  if(false){
    tone(buzzerPin, buzzerPitch, buzzerDuration);
    delay(buzzerDuration + buzzerDelay);
  }else{
    tone(buzzerPin, buzzerPitch, buzzerDuration2);
    delay(buzzerDuration2 + buzzerDelay2);
    tone(buzzerPin, buzzerPitch, buzzerDuration2);
    delay(buzzerDuration2 + buzzerDelay2);
    tone(buzzerPin, buzzerPitch, buzzerDuration2);
    delay(buzzerDuration2 + buzzerDelay2);
    tone(buzzerPin, buzzerPitch, buzzerDuration2);
    delay(buzzerDuration2 + buzzerDelay);
  }
}


void setup() {
  //this warms up the pins or something
  pinMode(b1Pin, INPUT_PULLUP);
  pinMode(b2Pin, INPUT_PULLUP);
  pinMode(b3Pin, INPUT_PULLUP);
  pinMode(b4Pin, INPUT_PULLUP);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(lGreenPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  display.clear(); //just to make it clear
  display.setBrightness(displayBrightness);
  
  Serial.begin(9600); //if you need use serial

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  //you can set the time of the real time clock module by one of these lines
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2020, 2, 14, 20, 57));
  }


void loop() {
  checkButtons(); //checks states of all buttons and switches and stores them in variables
  m0Cycle = 0; //this variable counts the number of cycles that the next while loop has gone through, it is confusing (even for me, now that i am commenting this code) but very important
  
  while(mode == 0){ //this mode just displays the time, and checks the buttons to react accordingly
    checkButtons(); //stores button states in variables
    checkAlarmSwitch(); //checks if alarm is enabled or not, than it sets the alarmState variable and manages the green led accordingly
    DateTime now = rtc.now(); //stores the current time in "now"
    if(b4State){ //if the fourth(alarm) button is pressed, it displays the time of the alarm, otherwise it just normally displays the current time
      displayAlarm();
    }else{
      displayTime();
    }
    if(b1State){ //if the user wants to set the time of the alarm and presses the first (set) button, the mode switches to 1
      mode = 1;
      m1Setting = 0; //starts by setting hours, than changed by switchM1Setting()
      tone(buzzerPin, 1500, 100); //makes the buzzer to do a fast beep
      m1Cycle = 0; //similar to m0Cycle, it makes the timing just right, dont care about it
    }
    if(now.hour() == alarmHour && now.minute() == alarmMinute && alarmState && m0Cycle >= 1200){ //if the time of the alarm matches current time and alarm is turned on it starts the alarm sequence, also some m0Cycle mystery whatever 
      startAlarm();
    }
    delay(50); //added delay so the microcontroller doesnt fly away
    if(m0Cycle <= 1200){
      m0Cycle ++;
    }
    }
    
  while(mode == 1){ //setting of the time of the alarm is done through this mode
    checkButtons();
    alarmAdjustment(); //checks if the buttons + or - are pressed, than it adjusts the alarm hour or minute(that is declared by m1Setting)
    displayAlarm(); //displays the time of the alarm
    blinkLGreen(); //simply inverts the state of the green led so it blinks each two cycles
    delay(200);
    if(b1State && m1Cycle){ //if the user confirms than it moves onto setting minutes, after second confirmation it switches to mode 0 (showing time)
      switchM1Setting();
      delay(500);
    }
    if(m1Cycle == 900){ //if you leave the device in the middle of the alarm setting process, it goes back to showing time (mode 0), aditionally a fast and sad sounding beep is executed
      mode = 0;
      tone(buzzerPin, 700, 100);
    }
    m1Cycle ++;
  }
  
  while(mode == 2){ //this mode is the alarm sequence
    checkButtons(); 
    makeNoise(); //makes the passive buzzer do annoying things to wake you up (it beeps), also it has two """melodies""" available, the second switch determines which one should be playing
    displayTime(); //displays current time
    if(b4State){ //if button four (alarm) state is 1 it sets the mode to 0 (displaying time)
      mode = 0;
    }
    delay(200);
  }
}