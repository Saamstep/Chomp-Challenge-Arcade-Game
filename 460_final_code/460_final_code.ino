#include <Servo.h>
#include "LedControl.h"

// ------------------------------------
// Penny Config
// ------------------------------------
#define PENNY_PIN 15

// ------------------------------------
// LED Display Config
// ------------------------------------
#define LC_DIN     12
#define LC_CLK     11
#define LC_CS      10
#define LC_DEVICES 1

LedControl lc = LedControl(LC_DIN, LC_CLK, LC_CS, LC_DEVICES);

#define DISPLAY_LEFT  4
#define DISPLAY_RIGHT 0

// ------------------------------------
// IR Sensors (Ball Score Detectors)
// ------------------------------------
#define HIPPO1_SENSOR 2
#define HIPPO2_SENSOR 3
#define HIPPO3_SENSOR 21

#define NUM_HIPPO_SENSORS 3
const int allHippoSensors[NUM_HIPPO_SENSORS] = {
  HIPPO1_SENSOR, HIPPO2_SENSOR, HIPPO3_SENSOR
};

// ------------------------------------
// Servos (Hippo Mouths)
// ------------------------------------
#define HIPPO1_SERVO 7
#define HIPPO2_SERVO 6
#define HIPPO3_SERVO 5

#define NUM_HIPPO_SERVO 3
const int allHippoServos[NUM_HIPPO_SERVO] = {
  HIPPO1_SERVO, HIPPO2_SERVO, HIPPO3_SERVO
};

Servo hippoServo[NUM_HIPPO_SERVO];

#define HIPPO_ANGLE_CLOSED 180
#define HIPPO_ANGLE_OPEN   150

// ------------------------------------
// Game Config
// ------------------------------------
#define GAME_DURATION_S 30
#define END_GAME_WAIT_S 5

enum state {
  IDLE,
  START,
  PLAYING,
 END
};

// ------------------------------------
// Globals
// ------------------------------------
volatile unsigned int score = 0;
unsigned int timeLeft = 0;
unsigned int pennyInserted = 0;
state CS = IDLE;
state NS = IDLE;
unsigned long prevTime = 0;

// ------------------------------------
// ISR Debounce Tracking
// ------------------------------------
volatile unsigned long lastTriggerTime[NUM_HIPPO_SENSORS] = {0};
const unsigned long SENSOR_DEBOUNCE_MS = 3000;

// ====================================================================
// Functions
// ====================================================================

// Penny Input
void PennyInit() {
  pinMode(PENNY_PIN, INPUT_PULLUP);
}

// Display --------------------------------------------------------------

void DisplayReset() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);
}

void _DisplayWrite(int startDigit, int value) {
  lc.setDigit(0, startDigit,     value % 10, false);
  lc.setDigit(0, startDigit + 1, (value / 10) % 10, false);
  lc.setDigit(0, startDigit + 2, (value / 100) % 10, false);
  lc.setDigit(0, startDigit + 3, (value / 1000) % 10, false);
}

void DisplayWrite(int disp, int value) {
  switch (disp) {
    case 0: _DisplayWrite(DISPLAY_LEFT, value); break;
    case 1: _DisplayWrite(DISPLAY_RIGHT, value); break;
    case 2:
      _DisplayWrite(DISPLAY_LEFT, value);
      _DisplayWrite(DISPLAY_RIGHT, value);
      break;
  }
}

void DisplayCountdown(int count) {
  DisplayWrite(1, 0);
  for (int i = count; i > 0; i--) {
    DisplayWrite(0, i);
    delay(1000);
  }
}

void DisplayUpdateCounters(int timeLeft, int score) {
  DisplayWrite(0, timeLeft);
  DisplayWrite(1, score);
}

// ----------------------------------------------------------------------
// IDLE DISPLAY ANIMATION (Rolling numbers)
// ----------------------------------------------------------------------
void IdleAnimation_Rolling() {
  static unsigned long lastRoll = 0;
  static int rollValue = 0;

  if (millis() - lastRoll > 150) {
    lastRoll = millis();
    rollValue = (rollValue + 1) % 10000;

    DisplayWrite(0, rollValue);
    DisplayWrite(1, 9999 - rollValue);
  }
}

// ----------------------------------------------------------------------
// IDLE HIPPO ANIMATION (one hippo opens at a time)
// ----------------------------------------------------------------------
void IdleAnimation_HippoCycle() {
  static unsigned long lastMove = 0;
  static int currentHippo = 0;

  if (millis() - lastMove > 700) {
    lastMove = millis();

    // Close all hippos
    for (int i = 0; i < NUM_HIPPO_SERVO; i++)
      hippoServo[i].write(HIPPO_ANGLE_CLOSED);

    // Open one hippo
    hippoServo[currentHippo].write(HIPPO_ANGLE_OPEN);

    currentHippo = (currentHippo + 1) % NUM_HIPPO_SERVO;
  }
}

// Run both idle animations
void RunIdleAnimations() {
  IdleAnimation_Rolling();
  IdleAnimation_HippoCycle();
}

// Servos --------------------------------------------------------------

void ServoSetup() {
  for (int i = 0; i < NUM_HIPPO_SERVO; ++i) {
    hippoServo[i].attach(allHippoServos[i]);
    hippoServo[i].write(HIPPO_ANGLE_CLOSED);
  }

  randomSeed(analogRead(0));
}

void ServoReset() {
  for (int i = 0; i < NUM_HIPPO_SERVO; ++i)
    hippoServo[i].write(HIPPO_ANGLE_CLOSED);
}

void ServoSetAngle(int angle) {
  for (int i = 0; i < NUM_HIPPO_SERVO; ++i)
    hippoServo[i].write(angle);
}

void ServoOpenHippoMouth() {
  int servoNum = random(0, NUM_HIPPO_SERVO);
  ServoSetAngle(HIPPO_ANGLE_CLOSED);
  hippoServo[servoNum].write(HIPPO_ANGLE_OPEN);
}

// Sensor Interrupt Handlers -------------------------------------------

void SensorISR_Normal() {
  int sensorIndex = 0;
  unsigned long now = millis();
  if (now - lastTriggerTime[sensorIndex] >= SENSOR_DEBOUNCE_MS) {
    score++;
    lastTriggerTime[sensorIndex] = now;
  }
}

void SensorISR_Normal2() {
  int sensorIndex = 1;
  unsigned long now = millis();
  if (now - lastTriggerTime[sensorIndex] >= SENSOR_DEBOUNCE_MS) {
    score++;
    lastTriggerTime[sensorIndex] = now;
  }
}

void SensorISR_Double() {
  int sensorIndex = 2;
  unsigned long now = millis();
  if (now - lastTriggerTime[sensorIndex] >= SENSOR_DEBOUNCE_MS) {
    score += 2;
    lastTriggerTime[sensorIndex] = now;
  }
}

// Enable / Disable Interrupts -----------------------------------------

void EnableSensorInterrupts() {
  attachInterrupt(digitalPinToInterrupt(allHippoSensors[0]), SensorISR_Normal, RISING);
  attachInterrupt(digitalPinToInterrupt(allHippoSensors[1]), SensorISR_Normal2, RISING);
  attachInterrupt(digitalPinToInterrupt(allHippoSensors[2]), SensorISR_Double, RISING);
}

void DisableSensorInterrupts() {
  for (int i = 0; i < NUM_HIPPO_SENSORS; i++)
    detachInterrupt(digitalPinToInterrupt(allHippoSensors[i]));
}

// ====================================================================
// Setup
// ====================================================================

void setup() {
  DisplayReset();
  ServoSetup();
  PennyInit();
  DisableSensorInterrupts();
}

// ====================================================================
// Main Loop
// ====================================================================

void loop() {
  unsigned long timeNow = millis();
  unsigned long timeElapsed = timeNow - prevTime;

  // -------------------------
  // STATE TRANSITION LOGIC
  //-------------------------
  switch (CS) {
    case IDLE:
      pennyInserted = digitalRead(PENNY_PIN);
      NS = (pennyInserted == HIGH) ? START : IDLE;
      break;

    case START:
      NS = START;
      break;

    case PLAYING:
      NS = (timeLeft > 0) ? PLAYING : END;
      break;

    case END:
      NS = (timeElapsed >= END_GAME_WAIT_S * 1000) ? IDLE : END;
      break;
  }

  // -------------------------
  // STATE ACTIONS
  //-------------------------
  switch (CS) {

    case IDLE:
      DisableSensorInterrupts();
      RunIdleAnimations();
      break;

    case START:
      DisableSensorInterrupts();
      DisplayCountdown(5);
      score = 0;
      timeLeft = GAME_DURATION_S;
      prevTime = millis();
      NS = PLAYING;
      break;

    case PLAYING:
      EnableSensorInterrupts();

      if (timeElapsed >= 1000) {
        ServoOpenHippoMouth();
        timeLeft--;
        prevTime = millis();
        DisplayUpdateCounters(timeLeft, score);
      }
      break;

    case END:
      DisableSensorInterrupts();

      if (timeElapsed == 0) {
        DisplayWrite(0, 0);
        DisplayWrite(1, score);
        prevTime = millis();
      }
      break;
  }

  CS = NS;
}