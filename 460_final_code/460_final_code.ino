// ------------------------------------
// Chomp Challenge Game Code
// ------------------------------------
#include <Servo.h>      ///< Servo Library https://docs.arduino.cc/libraries/servo/
#include "LedControl.h" ///< MAX7219 Control https://wayoda.github.io/LedControl/

// ------------------------------------
// Penny Config
// ------------------------------------
#define PENNY_PIN 15

// ------------------------------------
// LED Display Config
// ------------------------------------
#define LC_DIN     12 ///< Data In Pin
#define LC_CLK     11 ///< Clock Pin
#define LC_CS      10 ///< CS Pin
#define LC_DEVICES 1  ///< Number of MAX7219 ICs

LedControl lc = LedControl(LC_DIN, LC_CLK, LC_CS, LC_DEVICES);

#define DISPLAY_LEFT  4 ///< Index for writing to the left 7 segement display
#define DISPLAY_RIGHT 0 ///< Index for writing to the right 7 segement display

// ------------------------------------
// IR Sensors (Ball Score Detectors)
// ------------------------------------
#define HIPPO1_SENSOR 2  ///< GPIO Pin for IR Sensor 1
#define HIPPO2_SENSOR 3  ///< GPIO Pin for IR Sensor 2 (baby)
#define HIPPO3_SENSOR 21 ///< GPIO Pin for IR Sensor 3

#define NUM_HIPPO_SENSORS 3 ///< Total Number of IR sensors for scoring

/// Look UP Table (LUT) of all available hippo sensors as defined above
const int allHippoSensors[NUM_HIPPO_SENSORS] = {
  HIPPO1_SENSOR, HIPPO2_SENSOR, HIPPO3_SENSOR
};

// ------------------------------------
// Servos (Hippo Mouths)
// ------------------------------------
#define HIPPO1_SERVO 7 ///< GPIO Pin for Hippo 1
#define HIPPO2_SERVO 6 ///< GPIO Pin for Hippo 2 (baby)
#define HIPPO3_SERVO 5 ///< GPIO Pin for Hippo 3

#define NUM_HIPPO_SERVO 3 ///< Total Number of actuated Hippos

/// Look UP Table (LUT) of all available hippo servos as defined above
const int allHippoServos[NUM_HIPPO_SERVO] = {
  HIPPO1_SERVO, HIPPO2_SERVO, HIPPO3_SERVO
};

/// Servo array to easily handle all hippo servos at once (or individually using index-based accessing)
Servo hippoServo[NUM_HIPPO_SERVO];

#define HIPPO_ANGLE_CLOSED 180 /// Defined servo angle to have hippo mouth fully CLOSED
#define HIPPO_ANGLE_OPEN   150 /// Defined servo angle to have hippo mouth OPEN up to approximately 30degrees (enough for the ball to score)

// ------------------------------------
// Game Config
// ------------------------------------
#define GAME_DURATION_S 30 ///< The amount of time in seconds to alot for a game
#define END_GAME_WAIT_S 5 ///< Amount of time in seconds to wait 

/// States to track game (based on FSM drawing)
enum state {
  IDLE,
  START,
  PLAYING,
  END
};

// ------------------------------------
// Globals
// ------------------------------------
volatile unsigned int score = 0; ///< Tracking player score
unsigned int timeLeft = 0;       ///< Track time left in a game
unsigned int pennyInserted = 0;  ///< Value asserted if penny is inserted, else value is deasserted
state CS = IDLE;                 ///< Current State
state NS = IDLE;                 ///< Next State
unsigned long prevTime = 0;      ///< Variable to hold previous time in milliseconds for non-blocking cyclic code execution or timeouts

// ------------------------------------
// ISR Debounce Tracking
// ------------------------------------
volatile unsigned long lastTriggerTime[NUM_HIPPO_SENSORS] = {0};
const unsigned long SENSOR_DEBOUNCE_MS = 3000; // Number of milliseconds to wait before accepting a new score

// ====================================================================
// Functions
// ====================================================================

/// Penny Input Initalization function to set the pin mode to appropriate GPIO pin
void PennyInit() {
  pinMode(PENNY_PIN, INPUT_PULLUP);
}

// ----------------------------------------------------------------------
// Display Controller
// ----------------------------------------------------------------------

/// Reset and clear the display
void DisplayReset() {
  lc.shutdown(0, false);
  lc.setIntensity(0, 8);
  lc.clearDisplay(0);
}

/// Write a value to the display given the offset and the value to write
/// @param startDigit The digit to start at when writing to the display (DISPLAY_LEFT or DISPLAY_WRITE)
/// @param value The value to write to the display
void _DisplayWrite(int startDigit, int value) {
  lc.setDigit(0, startDigit,     value % 10, false);
  lc.setDigit(0, startDigit + 1, (value / 10) % 10, false);
  lc.setDigit(0, startDigit + 2, (value / 100) % 10, false);
  lc.setDigit(0, startDigit + 3, (value / 1000) % 10, false);
}

/// A friendly method to write to the display (easier for user)
/// @param disp 0 for left display, 1 for right display, 2 for both left and right display
/// @param value The value to write to the display
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

/// Countdown from `count` seconds to 0 (blocking function)
/// @param count Starting countdown value
void DisplayCountdown(int count) {
  DisplayWrite(1, 0);
  for (int i = count; i > 0; i--) {
    DisplayWrite(0, i);
    delay(1000);
  }
}

/// Writes to the displays to update score and counter during game `PLAYING` state
/// @param timeLeft New timer value
/// @param score Score value
void DisplayUpdateCounters(int timeLeft, int score) {
  DisplayWrite(0, timeLeft);
  DisplayWrite(1, score);
}

// ----------------------------------------------------------------------
// IDLE DISPLAY ANIMATION (Rolling numbers)
// ----------------------------------------------------------------------
void IdleAnimation_Rolling() {
  static unsigned long lastRoll = 0; ///< Store last roll timestamp
  static int rollValue = 0; ///< Value to store rolling number

  // Check for 150ms passing and update rolling counter value on scoreboards
  if (millis() - lastRoll > 150) {
    lastRoll = millis();
    rollValue = (rollValue + 1) % 10000;

    DisplayWrite(0, rollValue); ///< Write the CountUp Value on left display
    DisplayWrite(1, 9999 - rollValue); ///< Write the CountDown value on right display
  }
}

/// Run `IDLE` state animation
void RunIdleAnimations() {
  IdleAnimation_Rolling();
}

// ----------------------------------------------------------------------
// Servos
// ----------------------------------------------------------------------

/// Runs at startup
/// Initialize all hippo servos and reset to CLOSED state
void ServoSetup() {
  for (int i = 0; i < NUM_HIPPO_SERVO; ++i) {
    hippoServo[i].attach(allHippoServos[i]);
    hippoServo[i].write(HIPPO_ANGLE_CLOSED);
  }
  /// Set new random seed based on 'noise' from analog input 0 to randomize chosen hippos on startup
  randomSeed(analogRead(0));
}

/// Reset all hippo servos to CLOSED state
void ServoReset() {
  for (int i = 0; i < NUM_HIPPO_SERVO; ++i)
    hippoServo[i].write(HIPPO_ANGLE_CLOSED);
}

/// Set all hippo servo angles to a specific value
/// @param angle The angle to set the servo to
void ServoSetAngle(int angle) {
  for (int i = 0; i < NUM_HIPPO_SERVO; ++i)
    hippoServo[i].write(angle);
}

/// Open a random hippos mouth
void ServoOpenHippoMouth() {
  int servoNum = random(0, NUM_HIPPO_SERVO); ///< Select random hippo index 0 to `NUM_HIPPO_SERVO`
  ServoSetAngle(HIPPO_ANGLE_CLOSED); ///< Reset all hippos to closed
  hippoServo[servoNum].write(HIPPO_ANGLE_OPEN); ///< Command the randomly selected hippo to open
}

/// Close all hippo mouths
void ServoCloseALLHippoMouth() {
  ServoSetAngle(HIPPO_ANGLE_CLOSED);
}

// ----------------------------------------------------------------------
// IDLE DISPLAY ANIMATION (Rolling numbers)
// ----------------------------------------------------------------------

/// ISR Function for normal points (1 point) hippo
void SensorISR_Normal() {
  int sensorIndex = 0; // Index of sensor this ISR is attached to
  unsigned long now = millis(); // Store current timestamp

  // Check if time elapsed from last detection is less than configured `SENSOR_DEBOUNCE_MS`
  if (now - lastTriggerTime[sensorIndex] >= SENSOR_DEBOUNCE_MS) { 
    score++; // Increment score
    lastTriggerTime[sensorIndex] = now; // Update last detection
  }
}

/// ISR Function for normal points (1 point) hippo
void SensorISR_Normal2() {
  int sensorIndex = 1; // Index of sensor this ISR is attached to
  unsigned long now = millis(); // Store current timestamp

  // Check if time elapsed from last detection is less than configured `SENSOR_DEBOUNCE_MS`
  if (now - lastTriggerTime[sensorIndex] >= SENSOR_DEBOUNCE_MS) {
    score++;
    lastTriggerTime[sensorIndex] = now;
  }
}

/// ISR Function for double points (2 points) hippo
void SensorISR_Double() {
  int sensorIndex = 2; // Index of sensor this ISR is attached to
  unsigned long now = millis(); // Store current timestamp
  if (now - lastTriggerTime[sensorIndex] >= SENSOR_DEBOUNCE_MS) {
    score += 2; // Double score (instead of increment)
    lastTriggerTime[sensorIndex] = now; // Update last detection
  }
}

// ----------------------------------------------------------------------
// Enabling and Disabling Interrupts
// ----------------------------------------------------------------------

/// Attach interrupt for score detection via IR sensors configured GPIO pin
void EnableSensorInterrupts() {
  attachInterrupt(digitalPinToInterrupt(allHippoSensors[0]), SensorISR_Normal, RISING);  // Adult Hippo 1
  attachInterrupt(digitalPinToInterrupt(allHippoSensors[1]), SensorISR_Normal2, RISING); // Adult Hippo 2
  attachInterrupt(digitalPinToInterrupt(allHippoSensors[2]), SensorISR_Double, RISING);  // Adult Hippo 3
}

/// Detach all interrupts from IR sensors
void DisableSensorInterrupts() {
  for (int i = 0; i < NUM_HIPPO_SENSORS; i++)
    detachInterrupt(digitalPinToInterrupt(allHippoSensors[i]));
}

// ====================================================================
// Setup
// ====================================================================

void setup() {
  DisplayReset();             ///< 7 segement displays init
  ServoSetup();               ///< Hippo servos init
  PennyInit();                ///< Penny detection sensor init
  DisableSensorInterrupts();  ///< Disable all interrupts (enabled when they are only needed)
}

// ====================================================================
// Main Loop
// ====================================================================

void loop() {
  unsigned long timeNow = millis(); ///< Current cycle timestamp
  unsigned long timeElapsed = timeNow - prevTime; ///< Time elapsed since `prevTime` was stored

  // -------------------------
  // STATE TRANSITIONS
  //-------------------------
  switch (CS) {
    case IDLE:
      NS = (pennyInserted == HIGH) ? START : IDLE; ///< If penny is inserted move to `START`
      break;

    case START:
      NS = START; ///< State transition handled by state actions (this was more convenient) 
      break;

    case PLAYING:
      NS = (timeLeft > 0) ? PLAYING : END; ///< Stay in `PLAYING` until player runs out of time
      break;

    case END:
      NS = (timeElapsed >= END_GAME_WAIT_S * 1000) ? IDLE : END; ///< Go back to `IDLE` only if 5 seconds have elapsed
      break;
  }

  // -------------------------
  // STATE ACTIONS
  //-------------------------
  switch (CS) {

    case IDLE:
      pennyInserted = digitalRead(PENNY_PIN); ///< Poll penny sensor for detections
      DisableSensorInterrupts();              ///< Keep sensor interrupts disabled
      RunIdleAnimations();                    ///< Run idle animations while in state
      break;

    case START:
      DisableSensorInterrupts();             ///< Keep sensor interrupts disabled
      DisplayCountdown(5);                   ///< Start 5 second countdown
      score = 0;                             ///< Reset score
      timeLeft = GAME_DURATION_S;            ///< Update countdown timer to start at configured `GAME_DURATION_S`
      prevTime = millis();                   ///< Store previous time (legacy code)
      NS = PLAYING;                          ///< Moved to next state tracking
      break;

    case PLAYING:
      EnableSensorInterrupts();              ///< Enable sensor interrupts for score tracking

      /// Occurs every 1 second in the playing cycle (non blocking)
      if (timeElapsed >= 1000) {
        ServoOpenHippoMouth(); ///< Select random hippo mouth to open
        timeLeft--;            ///< Decrement timer (by 1 second)
        prevTime = millis();   ///< Update prevTime tracking to handle 1s cyclic exection using `millis()`
        DisplayUpdateCounters(timeLeft, score); ///< Update scoreboard
      }
      break;

    case END:
      DisableSensorInterrupts();             ///< Keep sensor interrupts disabled 
      ServoCloseALLHippoMouth();             ///< Close all hippo mouths (added so players don't try to score after the game ends)

      /// On the last second update the display to clear the timer and keep score displayed for 5 seconds before resetting to `IDLE`
      if (timeElapsed == 0) {
        DisplayWrite(0, 0);
        DisplayWrite(1, score);
        prevTime = millis(); ///< Update prevTime tracking to handle 5 second time out using `millis()`
      }
      break;
  }

  CS = NS; ///< FSM Condition for state transitions
}