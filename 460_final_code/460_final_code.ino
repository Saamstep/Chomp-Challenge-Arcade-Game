#include <Servo.h>
#include "LedControl.h"

// LED Display Config
#define LC_DIN     12
#define LC_CLK     11
#define LC_CS      10
#define LC_DEVICES 1

LedControl lc = LedControl(LC_DIN, LC_CLK, LC_CS, LC_DEVICES);

#define DISPLAY_LEFT 4
#define DISPLAY_RIGHT 0

// IR Sensor Config (Ball-Score Detector) [Interrupt Pins]
#define HIPPO1_SENSOR 2
#define HIPPO2_SENSOR 3
#define HIPPO3_SENSOR 21
#define HIPPO4_SENSOR 20
#define HIPPO5_SENSOR 19

#define NUM_HIPPO_SENSORS 5

const int allHippoSensors[NUM_HIPPO_SENSORS] = {HIPPO1_SENSOR, HIPPO2_SENSOR, HIPPO3_SENSOR, HIPPO4_SENSOR, HIPPO5_SENSOR};

// Servo Config (Hippo Mouth)
#define HIPPO1_SERVO 13
#define HIPPO2_SERVO 12
#define HIPPO3_SERVO 11
#define HIPPO4_SERVO 10
#define HIPPO5_SERVO 9

#define NUM_HIPPO_SERVO 5

const int allHippoServos[NUM_HIPPO_SERVO] = {HIPPO1_SERVO, HIPPO2_SERVO, HIPPO3_SERVO, HIPPO4_SERVO, HIPPO5_SERVO};

Servo hippoServo[NUM_HIPPO_SERVO];

// Game Config
#define GAME_DURATION_S 30
#define END_GAME_WAIT_S 5

enum state {
  IDLE,
  START,
  PLAYING,
  END,
};

// Game Globals
int score = 0;
int timeLeft = 0;
int pennyInserted = 0;
state CS = IDLE;
state NS = IDLE;
long prevTime = 0;

// Functions

// Display Control
void DisplayReset()
{
  /* The MAX72XX is in power-saving mode on startup,
  we have to do a wakeup call */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,8);
  /* and clear the display */
  lc.clearDisplay(0);
}

void _DisplayWrite(int startDigit, int value) {
  // startDigit = rightmost digit of the block
  lc.setDigit(0, startDigit,     value % 10, false);
  lc.setDigit(0, startDigit +1, (value / 10) % 10, false);
  lc.setDigit(0, startDigit +2, (value /100) % 10, false);
  lc.setDigit(0, startDigit +3, (value /1000)% 10, false);
}

// 0: Left, 1: Right, 2: Both
void DisplayWrite(int disp, int value)
{
  switch(disp)
  {
    case 0:
      _DisplayWrite(DISPLAY_LEFT, value);
    break;
    case 1:
      _DisplayWrite(DISPLAY_RIGHT, value);
      break;
    case 2:
      _DisplayWrite(DISPLAY_LEFT, value);
      _DisplayWrite(DISPLAY_RIGHT, value);
      break;
  }
}

void DisplayCountdown(int count)
{
  DisplayWrite(1, 0);
  DisplayWrite(0, 3);
  delay(1000);
  DisplayWrite(0, 2);
  delay(1000);
  DisplayWrite(0, 1);
  delay(1000);
}

void DisplayUpdateCounters(int timeLeft, int score);

// Servo Control
void ServoSetup()
{
  for(int i = 0; i < NUM_HIPPO_SERVO; ++i)
  {
    hippoServo[i].attach(allHippoServos[i]);
    hippoServo[i].write(0);
  }
}

void ServoReset()
{
  for(int i = 0; i < NUM_HIPPO_SERVO; ++i)
  {
    hippoServo[i].write(0);
  }
}

void ServoSetAngle(int angle)
{
  for(int i = 0; i < NUM_HIPPO_SERVO; ++i)
  {
    hippoServo[i].write(angle);
  }
}

// Sensor Reading
void SensorISR_Normal()
{
// points ++
}

void SensorISR_Double()
{
// points += 2
}

void SensorInit()
{
  for(int i = 0; i < NUM_HIPPO_SENSORS; ++i)
  {
    if(i < 5)
      attachInterrupt(digitalPinToInterrupt(allHippoSensors[i]), SensorISR_Normal, RISING);
    else
      attachInterrupt(digitalPinToInterrupt(allHippoSensors[i]), SensorISR_Double, RISING);
  }
}

// Setup Function (Runs on Startup)
void setup() {
  // LCD Display Init
  DisplayReset();
  
  // Servo Init

}

void loop() {
  // put your main code here, to run repeatedly:
  //DisplayCountdown(0);
  long time = millis();
  long timeElapsed = time - prevTime;

  switch(CS)
  {
    case IDLE:
      if(pennyInserted == HIGH)
      {
        NS = START;
      } else
      {
        NS = IDLE;
      }
      break;
    case START:
      if(timeLeft == GAME_DURATION_S)
      {
        NS = PLAYING;
      } else if(timeLeft == 0)
      {
        NS = START;
      }
      break;
    case PLAYING:
      if(timeLeft < GAME_DURATION_S && timeLeft > 0)
      {
        NS = PLAYING;
      } else
      {
        NS = END;
      }
      break;
    case END:
      if(timeElapsed >= END_GAME_WAIT_S*1000)
      {
        NS = IDLE;
      } else
      {
        NS = END;
      }
      break;
  }

    switch(CS)
  {
    case IDLE:

      break;
    case START:

      break;
    case PLAYING:

      break;
    case END:

      break;
  }

  CS = NS;
}
