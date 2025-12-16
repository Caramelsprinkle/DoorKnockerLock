#include <Servo.h>

// Project Specifications:
//  1. The piezo should be able to hear knocks at certain ranges and have a cooldown to prevent sending multiple signals using millis()
//  2. Make it so that an LED should light up whenever the Piezo recognizes a knock
//  3. The button, when pressed, should record the next 5 seconds (max) to rewrite to EEPROM a new sequence of knocks to remember
//    NOTE1: current button is only used to reset the servo motor's orientation. Need to change that
//    NOTE2: The button can be pressed again before 5 seconds ends the sequence reading earlier
//    NOTE3: The button should be attached to an interrupt service routine (I think). 
//    NOTE4: A single LED light should light up to show when it's recording.
//  4. When not in recording state - when in idle state - and the piezo receives a knock for the first time,
//      set the controller to a listening state
//    NOTE1: During listening state (lasts for 5 seconds after start), store data in an array of an array of: [[x1, y1], [x2, y2], ...]
//            - the piezo's electrical signal strength 
//            - time after the first knock in millis
//    NOTE2: Two LEDS: One lights up if the sequence is incorrect and the other lights up if sequence is correct
//  5. Result of listening state is compared against savedSequence to see if accepted
//    NOTE1: First, match the number of elements in the outer array of both the saved sequence and the listened sequence of knocks
//            Second, loop through a range of the size of the array and check both knock and time range are acceptable
//            Return true if acceptable within range, return false on the first detection of a non-acceptable knock 
//  6. Set two leniency numbers to create a range for what is considered as an acceptable knock range
//    EXAMPLE: int knockLeniency = 100. Knock value in savedSequence[0] = 550. Acceptable knock range = 450 - 650.
//    EXAMPLE (in milliseconds): int timeLeninecy = 300. time value in savedSequence[0] = 2300 (2.3 seconds). Acceptable range: 2000 - 2600.
//  7. Debug
//    log the sequence saved and sequence recorded into Serial

#define PIEZO_SENSOR_PIN A0     // Outputs electrical signal from vibrations
#define REDO_SEQUENCE_BUTTON 2  // Button sets listening state
#define LISTENING_LED_PIN 10    // Blue LED   : Lights up after button press and is listening to rewrite sequence. Blinks while in listening after first knock
#define KNOCK_LED_PIN 11        // Yellow LED : Lights up when Piezo recognizes knocks
#define SUCCESS_LED_PIN 12      // Green LED  : Lights up when sequence is correct   
#define FAILURE_LED_PIN 13      // Red LED    : Lights up when sequence is incorrect
#define SERVO_PIN 4             // Considered as "Locked" at 90 deg and "Unlocked" at 0

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
  Serial.begin(9600);

  myServo.attach(SERVO_PIN);
  pinMode(LISTENING_LED_PIN, OUTPUT);
  pinMode(KNOCK_LED_PIN, OUTPUT);
  pinMode(FAILURE_LED_PIN, OUTPUT);
  pinMode(SUCCESS_LED_PIN, OUTPUT);
  pinMode(REDO_SEQUENCE_BUTTON, INPUT);

  myServo.write(0);
  Serial.println("Unlocked State");
}

bool checkForKnock(int knockValue) {
  if (knockValue > quietKnock && knockValue < loudKnock) {
    digitalWrite(KNOCK_LED_PIN, HIGH);
    delay(20);
    digitalWrite(KNOCK_LED_PIN, LOW);
    Serial.print("Knock value: ");
    Serial.println(knockValue);
    return true;
  } else {
    //Serial.print("Invalid knock value: ");
    //Serial.println(knockValue);
    return false;
  }
}

void loop() {
  unsigned long timeNow = millis();

  knockValue = analogRead(PIEZO_SENSOR_PIN);
  
  if (knockValue > 10) {
    Serial.println(knockValue);
  }

  // if (locked == false || timeNow - lastTimeLocked >= 10000) {
  //   buttonValue = digitalRead(REDO_SEQUENCE_BUTTON);
    
  //   if (timeNow - lastTimeButtonChanged >= debounceDelay) {
  //     if (buttonValue == HIGH && timeNow - lastTimeLocked >= lockDelay) {
  //       lastTimeButtonChanged = timeNow;
  //       lastTimeLocked = timeNow;

  //       locked = true;
  //       digitalWrite(SUCCESS_LED_PIN, LOW);
  //       digitalWrite(FAILURE_LED_PIN, HIGH);
  //       myServo.write(90);
  //       Serial.println("Locked State");        
  //     }
  //   }
  // }

  // if (locked == true) {
  //   if (timeNow - lastTimeKnocked >= knockDelay) {
  //     lastTimeKnocked = timeNow;
  //     knockValue = analogRead(PIEZO_SENSOR_PIN);
  //   }

  //   if(numberOfKnocks < 3 && knockValue > 0) {
  //     if (checkForKnock(knockValue) == true) {
  //       numberOfKnocks++;
  //     }
  //     //Serial.print("Knocked ");
  //     //Serial.print(numberOfKnocks);
  //     //Serial.println(" times");
  //   }

  //   if (timeNow - lastTimeUnlocked >= lockDelay) {
  //     if(numberOfKnocks >= 3) {
  //       lastTimeUnlocked = timeNow;
        
  //       locked = false;
  //       myServo.write(0);
  //       digitalWrite(SUCCESS_LED_PIN, HIGH);
  //       digitalWrite(FAILURE_LED_PIN, LOW);
  //       Serial.println("Unlocked State");
  //       numberOfKnocks = 0;
  //     }
  //   }
  // }
}