# Chomp Challenge 
## Hippo Feeding Arcade Game – Arduino Project

## Overview

This project implements a **Hippo Feeding arcade game**. Players feed “food pellets” into hippo mouths by triggering infrared (IR) sensors. The goal is to score points before the timer runs out. The project includes:

- 3 hippos with **servo-controlled mouths**
- **IR sensors** to detect when a hippo is “fed”
- **MAX7219 7-segment LED displays** for timer and score
- **Idle animations** for display and hippos when the game is not active
- Penny input to start the game
- Debounced sensor interrupts to prevent multiple counts per feed
- Game states: **IDLE → START → PLAYING → END**

---

## Features

### Gameplay

- Insert a penny to start the game
- Hippos open their mouths randomly while playing
- Feed hippos to score points
  - Normal hippos: +1 point
  - Baby hippo: +2 points
- Timer counts down from 30 seconds
- Final score displayed for 5 seconds after the game ends

### Idle Mode Animations

- **LED rolling digits:** Both displays show a rolling number animation
- **Hippo cycling:** Hippos open one at a time in a loop to simulate life-like motion

---

## Hardware Requirements

- Arduino board (Uno, Mega, or compatible)
- 3x SG90 or similar micro servos
- 3x IR sensors
- MAX7219 7-segment LED display module
- Penny switch or button input
- 5V regulated power supply

---

## Wiring Overview

| Component            | Arduino Pin      |
|---------------------|----------------|
| HIPPO1 Servo         | 7              |
| HIPPO2 Servo         | 6              |
| HIPPO3 Servo         | 5              |
| HIPPO1 Sensor        | 2 (Interrupt)  |
| HIPPO2 Sensor        | 3 (Interrupt)  |
| HIPPO3 Sensor        | 21 (Interrupt) |
| MAX7219 DIN          | 12             |
| MAX7219 CLK          | 11             |
| MAX7219 CS           | 10             |
| Penny Input          | 15             |

> All grounds (GND) should be connected together, including the Arduino, display, sensors, and power supply.

---

## Software Overview

- **Servo Library**: Controls hippo mouth positions  
- **LedControl Library**: Drives the MAX7219 displays  
- **Debounced ISRs**: Prevent multiple triggers from a single sensor activation  
- **Game States**: IDLE, START, PLAYING, END  
- **Idle Animations**: Rolling digits and hippo mouth cycling  

---

## Usage

1. Connect all components as per wiring diagram.
2. Upload the Arduino code to your board.
3. Insert a penny (or press the input button) to start the game.
4. Feed the hippos by triggering the IR sensors.
5. Watch the timer count down and the score accumulate.
6. After the game ends, the final score is displayed for 5 seconds.
7. Return to idle animations until the next game starts.

---

## Notes

- Use a **clean, regulated 5V supply** for stable operation.  
- Sensor interrupts are **enabled only during PLAYING** to avoid false triggers.  

---