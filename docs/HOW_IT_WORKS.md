# How It Works — ACEBOTT QD001 Firmware
### Last Updated: March 28, 2026

---

## Overview

This firmware controls the ACEBOTT QD001 robot car using an **ESP32 microcontroller**
programmed with **ESP-IDF v5.4** in C. The car is controlled via an IR remote control
and avoids obstacles automatically using an ultrasonic distance sensor.

---

## Architecture

The codebase is split into two layers:

```
acebott_main.c          — "The Brain"
                          Startup sequence, main control loop,
                          IR command handling, ultrasonic logic

acebott_hw.c / .h       — "The Body"
                          Hardware abstraction: motors, IR receiver,
                          ultrasonic sensor, buzzer, headlights
```

This separation means `acebott_main.c` never talks to GPIO directly —
it only calls functions like `acebott_move()`, `acebott_get_distance()`,
and `acebott_get_ir_command()`.

---

## Startup Sequence

When the car powers on:

1. `acebott_init()` configures all GPIO pins, PWM channels, ADC, and IR RMT peripheral
2. Triple ascending beep (2000→2500→3000 Hz) signals system ready
3. Headlights double-flash to confirm LED output working
4. Brief forward motor pulse confirms motor wiring
5. Main loop begins

---

## Main Loop

The main loop runs continuously at ~50ms intervals and does two things:

### 1. Ultrasonic Check (if USE_ULTRASONIC = 1)

Every loop iteration:
- Pings the HC-SR04 sensor and reads distance in cm
- **30–15cm:** Proximity warning — beeps at increasing rate as car approaches obstacle
- **< 15cm:** AUTO-STOP — motors halt, low buzz, sets `stopped_by_obstacle` flag
- **> 25cm after stop:** AUTO-RESUME — car restarts at speed 150, two ascending beeps

The ultrasonic check only applies when going **FORWARD** — reverse movement
is never blocked (you're already moving away from the obstacle).

### 2. IR Remote Check

Reads the RMT ring buffer for any received IR codes. If a valid 32-bit NEC
code is decoded it's matched against known button codes and acted upon:

| Button | Action |
|:---|:---|
| UP | Accelerate forward. First press jumps to MIN speed (150) to overcome gear friction, then +50 per press up to 255 |
| DOWN | Accelerate reverse (same torque-jump logic) |
| UP while reversing | Decelerate — reduces reverse speed by 50 |
| DOWN while going forward | Decelerate — reduces forward speed by 50 |
| LEFT | Momentary CCW pivot (150ms) then resume previous speed/direction |
| RIGHT | Momentary CW pivot (150ms) then resume |
| OK | Emergency stop — speed=0, headlights flash twice, long low beep |
| 1–9 | Speed presets (mapped to specific speeds, minimum MIN_MOTOR_SPEED) |
| 0 | Stop |
| STAR (*) | Double beep (currently unassigned — future use) |
| HASH (#) | Headlight flash (currently unassigned — future use) |

---

## Motor Control

Motors are driven through a **74HC595 8-bit shift register** connected to an **L298N
H-bridge**. To set a direction, the firmware bit-bangs an 8-bit pattern into the
shift register via DATA/CLOCK/LATCH pins, then sets PWM speed via LEDC.

The 4 Mecanum wheels (2 per side, wired in parallel) allow:
- Forward/backward: all wheels same direction
- CW/CCW spin: left and right sides opposite directions
- True strafing is not yet implemented (would require individual wheel control)

**Key insight discovered during development:** Starting at speed 0 stalls the
gearbox. The firmware always jumps to `MIN_MOTOR_SPEED` (150) on first press
before incrementing, ensuring the motors always have enough torque to start.

---

## IR Decoding

The ESP32's **RMT (Remote Control) peripheral** captures the raw IR pulse train.
The firmware decodes it as NEC protocol: a 9ms header burst, 4.5ms gap, then
32 bits encoded as pulse-width (short = 0, long = 1).

All 17 buttons on the included remote were tested and confirmed March 28, 2026.
Codes are stored in:
- `IR_REMOTE_CODES.txt` — human-readable reference
- `ir_button_t` enum in `acebott_hw.h` — 7 named buttons (0–9 handled in main)

---

## Ultrasonic Distance

The HC-SR04 sensor uses a simple timing approach:
1. Send 10µs trigger pulse on TRIG pin
2. Measure how long ECHO pin stays HIGH
3. `distance_cm = echo_duration_us / 58.0`

Timeouts return -1.0f (no echo = sensor error or object too far).
The firmware polls the sensor every loop cycle (~50ms).

---

## Configuration Toggles

Two `#define` toggles at the top of `acebott_main.c` let you adjust behavior
without changing logic:

```c
#define USE_ULTRASONIC  1   // 0 = disable sensor (set this if sensor unplugged)
#define USE_MOTORS      1   // 0 = log commands only, no movement (safe bench testing)
```

---

## Key Speed Constants

```c
#define SPEED_STEP         50   // Speed change per UP/DOWN press
#define MIN_MOTOR_SPEED   150   // Minimum speed that overcomes gear friction
#define MAX_MOTOR_SPEED   255   // Maximum PWM duty cycle
#define STEER_NUDGE_MS    150   // How long LEFT/RIGHT pivot lasts
#define STEER_NUDGE_SPEED 160   // Speed used during pivot
```

---

## File Map

| File | Purpose |
|:---|:---|
| `main/acebott_main.c` | App entry point, startup, main loop |
| `main/acebott_hw.c` | Hardware driver implementations |
| `main/acebott_hw.h` | Pin definitions, enums, function prototypes |
| `main/secrets.h` | WiFi credentials (gitignored, not currently used) |
| `IR_REMOTE_CODES.txt` | All 17 confirmed IR button codes |
| `docs/NOTES.md` | Project parking spot — resume from here |
| `docs/HARDWARE_MAP.md` | Complete GPIO reference |
| `docs/HOW_IT_WORKS.md` | This file |

---

## What's Not Yet Implemented

- **Servo panning:** Servo is wired to GPIO 25 but firmware not written yet
- **Scanning obstacle avoidance:** Stop, pan servo left/right, choose open path
- **Line following:** Sensors wired and ADC working, logic stubbed in comments
- **STAR/HASH button actions:** Currently just beep/flash — unassigned
- **OTA updates:** Future feature — flash over WiFi without USB cable
- **Modern ESP-IDF drivers:** Legacy RMT and ADC APIs work but are deprecated
