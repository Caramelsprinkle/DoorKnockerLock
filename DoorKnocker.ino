#include <Servo.h>

#define PIEZO_PIN A0
#define BUTTON 9
#define YELLOW_LED_PIN 11
#define GREEN_LED_PIN 12
#define RED_LED_PIN 13
#define SERVO_PIN 10

Servo myServo;

int knockValue;
int buttonValue;

const int quietKnock = 10;
const int loudKnock = 100;
bool locked = false;
int numberOfKnocks = 0;

unsigned long lastTimeKnocked = millis();
unsigned long knockDelay = 20;

unsigned long lastTimeLocked = millis();
unsigned long lastTimeUnlocked = millis();
unsigned long lockDelay = 1000;

unsigned long debounceDelay = 50;
unsigned long lastTimeButtonChanged = millis();

// TODO:  Add Debounce for buttons
//        Attach interrupt for when Piezo recieves data
//        Implement save function of knock timing patterns to EEPROM
//        Add reset timer for knocks after the first knock

void setup() {
  // put your setup code here, to run once:
  myServo.attach(SERVO_PIN);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BUTTON, INPUT);
  Serial.begin(9600);

  digitalWrite(GREEN_LED_PIN, HIGH);
  myServo.write(0);
  Serial.println("Unlocked State");
}

bool checkForKnock(int knockValue) {
  if (knockValue > quietKnock && knockValue < loudKnock) {
    digitalWrite(YELLOW_LED_PIN, HIGH);
    delay(20);
    digitalWrite(YELLOW_LED_PIN, LOW);
    //Serial.print("Knock value: ");
    //Serial.println(knockValue);
    return true;
  } else {
    //Serial.print("Invalid knock value: ");
    //Serial.println(knockValue);
    return false;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long timeNow = millis();

  if (locked == false || timeNow - lastTimeLocked >= 10000) {
    buttonValue = digitalRead(BUTTON);
    
    if (timeNow - lastTimeButtonChanged >= debounceDelay) {
      if (buttonValue == HIGH && timeNow - lastTimeLocked >= lockDelay) {
        lastTimeButtonChanged = timeNow;
        lastTimeLocked = timeNow;

        locked = true;
        digitalWrite(GREEN_LED_PIN, LOW);
        digitalWrite(RED_LED_PIN, HIGH);
        myServo.write(90);
        Serial.println("Locked State");        
      }
    }
  }

  if (locked == true) {
    if (timeNow - lastTimeKnocked >= knockDelay) {
      lastTimeKnocked = timeNow;
      knockValue = analogRead(PIEZO_PIN);
    }

    if(numberOfKnocks < 3 && knockValue > 0) {
      if (checkForKnock(knockValue) == true) {
        numberOfKnocks++;
      }
      //Serial.print("Knocked ");
      //Serial.print(numberOfKnocks);
      //Serial.println(" times");
    }

    if (timeNow - lastTimeUnlocked >= lockDelay) {
      if(numberOfKnocks >= 3) {
        lastTimeUnlocked = timeNow;
        
        locked = false;
        myServo.write(0);
        digitalWrite(GREEN_LED_PIN, HIGH);
        digitalWrite(RED_LED_PIN, LOW);
        Serial.println("Unlocked State");
        numberOfKnocks = 0;
      }
    }
  }
}