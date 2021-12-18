//LIBRARIES
#include <Arduino.h>
#include <TM1637Display.h>
#include "RTClib.h"
#include <EEPROM.h>

//CONSTANTS
//pins
const uint8_t b1Pin = 5;
const uint8_t b2Pin = 2;
const uint8_t b3Pin = 3;
const uint8_t b4Pin = 4;
const uint8_t switchPin = 8;
const uint8_t ledPin = 6;
const uint8_t buzzerPin = 11;
#define CLK 12
#define DIO 10
//brightness
const uint8_t displayBrightness = 2;
const int ledBrightness = 15;
const unsigned long nightModeButtonPressLength = 3800; 
//buzzer
const int buzzerPitch = 1000;
const int buzzerDelay = 100;
const int buzzerDuration = 100;
//rtc
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//VARIABLES
//Mode manipulation
int mode = 0; //0-show time, 1-set alarm, 2-execute alarm
int m1Setting = 0; //0-set hours, 1-set minutes
int m1Cycle = 0; 
int m0Cycle = 0;
bool ledState = 0;
bool nightMode = 0;
//Pin states
bool b1State = 0;
bool b2State = 0;
bool b3State = 0;
bool b4State = 0;
bool b4PreviousState = 0;
unsigned long b4PressTime = 0;
bool switchState = 0;
//Alarm
bool alarmEnabled = 0;
int alarmHour = 0;
int alarmMinute = 0;

//OBJECTS
TM1637Display display = TM1637Display(CLK, DIO);
RTC_DS3231 rtc;

//DEFINE FUNCTIONS
void readButtons(){ //checks states of all buttons and switches and stores them in variables (the states are flipped, because I am using internal pullup resistors)
  b1State = !digitalRead(b1Pin);
  b2State = !digitalRead(b2Pin);
  b3State = !digitalRead(b3Pin);
  b4State = !digitalRead(b4Pin);
  switchState = !digitalRead(switchPin);
}

void displayTime(){ //displays current time on the display
  DateTime now = rtc.now();

  display.showNumberDecEx(now.hour(), 0b11100000, true, 2, 0);
  display.showNumberDecEx(now.minute(), 0b11100000, true, 2, 2);
}

void displayAlarm(){ //displays the time of the alarm
  display.showNumberDecEx(alarmHour, 0b11100000, true, 2, 0);
  display.showNumberDecEx(alarmMinute, 0b11100000, true, 2, 2);
}

void checkAlarmSwitch(){ //checks if alarm is enabled or not, than it sets the alarmEnabled variable and manages the led accordingly
  if(switchState){
    alarmEnabled = 1; 
  } else{
    alarmEnabled = 0; 
  }
  analogWrite(ledPin, (!nightMode)*alarmEnabled*ledBrightness);
}

void switchM1Setting(){ //adds 1 to m1Setting, if it is bigger than 1 it changes mode to 0(shows time normally)
  m1Setting ++;
  //m1Cycle = 0;
  if(m1Setting == 2){
    EEPROM.write(24, alarmHour);
    EEPROM.write(25, alarmMinute);
    mode = 0;
    tone(buzzerPin, buzzerPitch, 400);
  }else{
    tone(buzzerPin, buzzerPitch, 100);
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
}}}}

void blinkLed(){ //simply inverts the state of the led
  ledState = !ledState;
  analogWrite(ledPin, (!nightMode)*ledState*ledBrightness);
}

void makeNoise(){ 
  for (int i = 0; i < 4; i++){
    tone(buzzerPin, buzzerPitch, buzzerDuration);
    analogWrite(ledPin, ledBrightness);
    delay(buzzerDuration);
    analogWrite(ledPin, 0);
    delay(buzzerDelay);
  }
  delay(400);
}


void setup() {
  pinMode(b1Pin, INPUT_PULLUP);
  pinMode(b2Pin, INPUT_PULLUP);
  pinMode(b3Pin, INPUT_PULLUP);
  pinMode(b4Pin, INPUT_PULLUP);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  display.clear();
  display.setBrightness(displayBrightness);
  
  Serial.begin(9600); //if you need use serial

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  //set the time of the rtc module by one of these lines
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //rtc.adjust(DateTime(2020, 2, 14, 20, 57));
  
  alarmHour = EEPROM.read(24);
  alarmMinute = EEPROM.read(25);
  }


void loop() {
  readButtons();
  m0Cycle = 0;
  
  while(mode == 0){ //this mode just displays the time, and checks the buttons to react accordingly
    readButtons();
    checkAlarmSwitch(); //checks if alarm is enabled or not, than it sets the alarmEnabled variable and manages the led accordingly
    DateTime now = rtc.now(); //stores the current time in "now"
    if(b4State != b4PreviousState){
      if(b4State){
        b4PressTime = millis();
      }else if(millis() - b4PressTime >= nightModeButtonPressLength){
        nightMode = !nightMode;
        display.setBrightness((!nightMode)*displayBrightness);
      }
    }
    if(b4State){ //if the fourth(alarm) button is pressed, it displays the time of the alarm, otherwise it just normally displays the current time
      displayAlarm();
    }else{
      displayTime();
    }
    if(b1State){ //if the user "set alarm" button, the mode switches to 1
      mode = 1;
      m1Setting = 0; //starts by setting hours, than changed by switchM1Setting()
      tone(buzzerPin, buzzerPitch, 100); //beep
      m1Cycle = 0; //similar to m0Cycle, it makes the timing just right, dont care about it
    }
    if(now.hour() == alarmHour && now.minute() == alarmMinute && alarmEnabled && m0Cycle >= 1200){ //if the time of the alarm matches current time and alarm is turned on it starts the alarm sequence, also some m0Cycle mystery whatever 
      mode = 2;
      nightMode = 0;
    }
    b4PreviousState = b4State;
    delay(50); //cycle delay
    if(m0Cycle <= 1200){
      m0Cycle ++;
    }
    }
    
  while(mode == 1){ //setting of the time of the alarm is done through this mode
    readButtons();
    alarmAdjustment(); //checks if the buttons + or - are pressed, than it adjusts the alarm hour or minute(that is declared by m1Setting)
    displayAlarm(); //displays the time of the alarm
    blinkLed(); //simply inverts the state of the led
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
    readButtons(); 
    makeNoise(); //makes the passive buzzer do annoying things to wake you up (it beeps), also it has two """melodies""" available, the second switch determines which one should be playing
    displayTime(); //displays current time
    if(b4State){ //if button four (alarm) state is 1 it sets the mode to 0 (displaying time)
      mode = 0;
    }
    delay(200);
  }
}