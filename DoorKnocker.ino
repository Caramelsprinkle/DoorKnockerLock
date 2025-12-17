#include <Servo.h>
#include <EEPROM.h>

// Project Specifications:
//  1. The piezo should be able to hear knocks at certain ranges and have a cooldown to prevent sending multiple signals using millis()
//  2. Make it so that an LED should light up whenever the Piezo recognizes a knock
//  3. The button, when pressed, should record the next 5 seconds (max) to rewrite to EEPROM a new sequence of knocks to remember
//    NOTE1: current button is only used to reset the servo motor's orientation. Need to change that
//    NOTE2: The button can be pressed again before 5 seconds ends the sequence reading earlier
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
//    EXAMPLE: int knockLeniency = 30. Knock value in savedSequence[0] = 550. Acceptable knock range = 450 - 650.
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

#define EEPROM_ADDRESS_UNIT_KNOCK_SEQUENCE 50
#define EEPROM_ADDRESS_UNIT_TIME_SEQUENCE 51
#define EEPROM_ADDRESS_UNIT_KNOCK_COUNT 52

Servo myServo;

// Servo variables
const unsigned long timeUntilLock = 10000;  // stays open for 10 seconds before locking again  
const unsigned int lockedState = 90;        // degree angle the servo spins to 
const unsigned int unlockedState = 0;
const unsigned long timeForServoLock = 800;

unsigned long lastTimeLocked = millis();
unsigned long lastTimeUnlocked = millis();
bool isLocked = false;


// Piezo variables
const int minKnockTreshold = 10;    // Knocks are only recognized if Piezo sends a signal above 10 and below 180
const int maxKnockTreshold = 180;
bool isRecording = false;
bool isListening = false;

unsigned long lastTimeKnocked = millis();
const unsigned long knockDelay = 80;

unsigned long firstKnockTimed = millis();
const unsigned long firstKnockTimeout = 5000;

unsigned int knockCount = 0;
// Button variables
const unsigned long debounceDelay = 50;
unsigned long lastTimeButtonChanged = millis();

// LED Variables
const unsigned long knockLEDDelay = 200;
unsigned long knockLEDLastOn = millis();
bool knockLEDState = false;

const unsigned long listeningBlinkLEDDelay = 500;
unsigned long listeningBlinkLEDLastOn = millis();

const unsigned long successLEDDelay = 1000;
unsigned long successLEDLastOn = millis();

const unsigned long failureLEDDelay = 1000;
unsigned long failureLEDLastOn = millis();

// Miscellaneous
const unsigned long recordingDuration = 5000; 

int knockValuesSequence[20] = {};       // Records knock values (after initial knock) to be compared against stored knockvalues
unsigned long timeSequence[20] = {};    // Stores time of when knock occurs ^
unsigned int index = 0;

bool hasKnocked = false;
bool hasKnockedPhrase = false;

const unsigned long timeLeniency = 300; // 0.3 second leniency for knock comparison
const unsigned int knockLeniency = 30;  // 30 analog reading leniency for knock comparison

// Stand in for EEPROM. Will need to use Address + 1 to store each analog and millis value. Didn't have time.
unsigned int storedKnockValuesSequence[20] = {};  // Stores knock values to be compared later as the passcode
unsigned long storedTimeSequence[20] = {};        // Stores time of when knock occurs ^
unsigned int storedKnockCount = 0;

// TODO:  Implement save function of knock timing patterns to EEPROM
//        Add reset timer for knocks after the first knock

void captureNewKnockSequence() {
    // stub
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  myServo.attach(SERVO_PIN);
  pinMode(LISTENING_LED_PIN, OUTPUT);
  pinMode(KNOCK_LED_PIN, OUTPUT);
  pinMode(FAILURE_LED_PIN, OUTPUT);
  pinMode(SUCCESS_LED_PIN, OUTPUT);
  pinMode(REDO_SEQUENCE_BUTTON, INPUT);

  delay(timeForServoLock);

  myServo.write(lockedState);

  delay(timeForServoLock); // delay so that the servo's vibration isn't captured by the Piezo

  Serial.println("Locked State");
}

bool checkForKnock(int knockValue) {
  if (knockValue > minKnockTreshold && knockValue < maxKnockTreshold) {
    knockLEDState = true;
    digitalWrite(KNOCK_LED_PIN, HIGH);
    Serial.print("Knock value: ");
    Serial.println(knockValue);
    return true;
  }
  //Serial.print("Invalid knock value: ");
  //Serial.println(knockValue);
  return false;
}

// States:
//  Piezo:
//  - Idle (not represented with boolean)
//  - Listening
//  - Recording
//
//  Servo:
//  - Locked
//  - Unlocked

bool compareKnockSequence(int knockSequence[20], unsigned long timeSequence[20], unsigned int knockCount) {
  //unsigned int storedKnockSequence[20] = storedKnockSequence = EEPROM.get(EEPROM_ADDRESS_UNIT_KNOCK_SEQUENCE);
  //unsigned long storedTimeSequence[20] = EEPROM.get(EEPROM_ADDRESS_UNIT_TIME_SEQUENCE);
  //unsigned int storedKnockCount = EEPROM.read(EEPROM_ADDRESS_UNIT_KNOCK_COUNT);

  if (storedKnockCount == 0) {  // Checks if a passcode has been set
    Serial.println("Sequence has not been set yet");
    return false;
  }

  if (knockCount != storedKnockCount) { // Checks if passcode and most recent attempt has matching number of knocks
    Serial.print("storedKnockCount: ");
    Serial.println(storedKnockCount);
    return false;
  }
  
  // Checks that each element in the two arrays (piezo analog read strength and recorded time of knocks) match with leniency
  for (int i = 0; i < knockCount; i++) {
    if (knockSequence[i] <= storedKnockValuesSequence[i] + knockLeniency && knockSequence[i] >= storedKnockValuesSequence[i] - knockLeniency) {
      Serial.println(storedKnockValuesSequence[i]);
      return false;
    }
    if (timeSequence[i] <= storedTimeSequence[i] + timeLeniency && timeSequence[i] >= storedTimeSequence[i] - timeLeniency) {
      Serial.println(storedTimeSequence[i]);
      return false;
    }
  }
  
  Serial.println("Sequence matched");
  return true;
}

void loop() {
  unsigned long currentTime = millis();
  
  // Press the button to record a new sequence that lasts 5 seconds
  if (currentTime - lastTimeButtonChanged >= debounceDelay) {
    if (digitalRead(REDO_SEQUENCE_BUTTON) == HIGH) {
      lastTimeButtonChanged = millis();
      isRecording = true;
      isListening = false;
      index = 0;
      digitalWrite(LISTENING_LED_PIN, HIGH);
      digitalWrite(LISTENING_LED_PIN, HIGH);
      Serial.println("Recording new sequence");
    }
  }

  // when recording, check for live knocks on the Piezo and add them to the storedValues arrays  
  if (isRecording) {
    unsigned int knockValue = 0;
    if(currentTime - lastTimeKnocked >= knockDelay) {
      lastTimeKnocked = millis();
      knockValue = analogRead(PIEZO_SENSOR_PIN);
      hasKnocked = true;
    }

    if (hasKnocked && checkForKnock(knockValue)) {
      // Two arrays contain electrical signal of Piezo and currentTime
      storedKnockValuesSequence[index] = knockValue;
      storedTimeSequence[index] = millis();
      knockCount++;
      index++;
    }
    hasKnocked = false;

    // Option 1: end the sequence early by pressing the button a second time while the blue LED is still on
    if (currentTime - lastTimeButtonChanged >= debounceDelay) {
      if (digitalRead(REDO_SEQUENCE_BUTTON) == HIGH) {
        lastTimeButtonChanged = millis();
        isRecording = false;
        digitalWrite(LISTENING_LED_PIN, LOW);
        // Serial.print("Knock sequence: ");
        // Serial.println(knockValuesSequence);
        // Serial.print("Time sequence: ");
        // Serial.println(timeSequence);
        
        // TODO: Fix EEPROM
        // EEPROM.write(EEPROM_ADDRESS_UNIT_KNOCK_SEQUENCE, knockValuesSequence);
        // EEPROM.write(EEPROM_ADDRESS_UNIT_TIME_SEQUENCE, timeSequence);
        // EEPROM.write(EEPROM_ADDRESS_UNIT_KNOCK_COUNT, knockCount);
        storedKnockCount = knockCount;
        knockCount = 0;
        index = 0;
      }
    }

    // Option 2: The sequence is saved automatically after 5 seconds have passed
    if (currentTime - lastTimeButtonChanged >= recordingDuration) {
      isRecording = false;
      digitalWrite(LISTENING_LED_PIN, LOW);
      storedKnockCount = knockCount;
      knockCount = 0;
      index = 0;
    }
  } else {  // if not recording, then it is idle
    if (isLocked == true) {
      if (!isListening) { // While idle, it listens for a first knock to start listening (not recorded as part of the passcode)
        unsigned int knockValue = 0;
        if(currentTime - lastTimeKnocked >= knockDelay) {
          lastTimeKnocked = millis();
          knockValue = analogRead(PIEZO_SENSOR_PIN);
          hasKnocked = true;
        }

        if (hasKnocked && checkForKnock(knockValue)) { // knock has been detected and the controller is now listening for the sequence
          isListening = true;
          index = 0;
          lastTimeKnocked = millis();
          firstKnockTimed = millis();
        } 
        hasKnocked = false;
      } else { // The controller listens for 5 seconds
        unsigned int knockValue = 0;
        if (currentTime - lastTimeKnocked >= knockDelay) { 
          lastTimeKnocked = millis();
          knockValue = analogRead(PIEZO_SENSOR_PIN);
          hasKnocked = true;
        }

        if (hasKnocked && checkForKnock(knockValue)) { // if a knock is recognized while the controller is listening for the sequence, add values to array
          knockValuesSequence[index] = knockValue;
          timeSequence[index] = millis();
          knockCount++;
          index++;
        }
        hasKnocked = false;

        if (currentTime - firstKnockTimed >= firstKnockTimeout) { // When timed-out, the knock phrase is ready to be compared
          hasKnockedPhrase = true;
          isListening = false;
          index = 0;
        }
      }


      // Unlocked after passing comparison
      if (hasKnockedPhrase) {
        if (compareKnockSequence(knockValuesSequence, timeSequence, knockCount)) {  // Valid knock passcode
          isLocked = false;
          myServo.write(unlockedState);
          lastTimeUnlocked = millis();
          
          successLEDLastOn = millis();
          digitalWrite(SUCCESS_LED_PIN, HIGH);
          digitalWrite(LISTENING_LED_PIN, LOW);
        } else {                                                                    // Invalid knock passcode
          failureLEDLastOn = millis();
          digitalWrite(FAILURE_LED_PIN, HIGH);
          Serial.println("Failed to unlock");
        }
        knockCount = 0;
        hasKnockedPhrase = false;
        delay(timeForServoLock);
      }
    } else { // controller has unlocked after providing correct passcode
      if (currentTime - lastTimeUnlocked >= timeUntilLock) {
        isLocked = true;
        myServo.write(lockedState);
        delay(timeForServoLock); // delay to prevent piezo from receiving servo's vibrations
      }
    }
  }
    
  // Yellow LED lights up for a short period when a knock is detected
  if (knockLEDState) {
    if (currentTime - knockLEDLastOn >= knockLEDDelay) {
      knockLEDState = false;
      digitalWrite(KNOCK_LED_PIN, LOW);
    }
  }

  // Blue LED blinks when listening 
  if (isListening) {
    if (currentTime - listeningBlinkLEDLastOn >= listeningBlinkLEDDelay) {
      listeningBlinkLEDLastOn = millis();
      digitalWrite(LISTENING_LED_PIN, !digitalRead(LISTENING_LED_PIN));
    }
  }

  // Success and Failure LED resets automatically after 1 second has passed
  if (currentTime - successLEDLastOn >= successLEDDelay) { 
    digitalWrite(SUCCESS_LED_PIN, LOW);
  }
   
  if (currentTime - failureLEDLastOn >= failureLEDDelay) {
    digitalWrite(FAILURE_LED_PIN, LOW);   
  }
}

// Considerations for improvements:
//  - Figure out how to implement it into EEPROM
//  - Use moving average to smooth out the knock instead of taking the first value and blocking the rest with millis (only thought about this today)