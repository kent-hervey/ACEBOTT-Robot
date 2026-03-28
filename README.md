# ACEBOTT-Robot 🏎️

This repository contains the ESP-IDF firmware for the **ACEBOTT QD001 ESP32 Robot Car** — a 4WD Mecanum-wheel robot controlled via IR remote with ultrasonic obstacle avoidance.

---

## 📖 Project Resources

- **Official Product Page:** [ACEBOTT QD001 Smart Car Kit](https://shop.acebott.com/products/qd001-esp32-smart-car-kit-collection)
- **Assembly Guide:** [Assemble the Smart Car (PDF)](<docs/Assemble the smart car.pdf>)
- **Hardware Documentation:** [Hardware GPIO Map](docs/HARDWARE_MAP.md)
- **Technical Overview:** [How It Works](docs/HOW_IT_WORKS.md)
- **Project Status & Resume Guide:** [NOTES.md](docs/NOTES.md)

---

## 📌 Project Overview

This firmware provides real-time IR remote control of the ACEBOTT QD001 with automatic obstacle avoidance. The car is driven using a 17-button IR remote, responds to proximity warnings, and auto-stops when an obstacle is detected within 15cm.

---

## 🛠️ Features

- **IR Remote Control:** All 17 buttons mapped and confirmed (NEC protocol)
- **Torque-Jump Acceleration:** Jumps to minimum effective speed on first press to overcome gear friction
- **Ultrasonic Obstacle Avoidance:** Proximity warning beeps 15–30cm, auto-stop at 15cm, auto-resume when clear
- **Speed Presets:** Number buttons 1–9 set discrete speed levels, 0 stops
- **Momentary Steering:** LEFT/RIGHT buttons pivot briefly then resume previous direction
- **Startup Self-Test:** Triple ascending beep + headlight flash confirms hardware on boot
- **Clean Architecture:** Hardware abstraction layer separates drivers from logic
- **ESP-IDF v5.4:** Built using ESP-IDF in C (not Arduino)

---

## 🚀 Getting Started

### Prerequisites

- **CLion IDE** with the **ESP-IDF Plugin** installed
- **ESP-IDF v5.4** configured in your environment
- **macOS:** Run `source ~/esp/esp-idf/export.sh` before using `idf.py`

### Build & Flash

```bash
git clone https://github.com/kent-hervey/ACEBOTT-Robot.git
cd ACEBOTT-Robot
git switch experiment-control        # active development branch
idf.py -p /dev/cu.usbserial-14340 flash monitor | cat
```

### Configuration Toggles

Two `#define` flags at the top of `main/acebott_main.c`:

```c
#define USE_ULTRASONIC  1   // 0 = disable sensor, 1 = enable auto-stop
#define USE_MOTORS      1   // 0 = log only (bench test), 1 = real movement
```

---

## 🎮 IR Remote Button Map

| Button | Action |
|:---|:---|
| UP | Accelerate forward (torque jump to 150, then +50 per press, max 255) |
| DOWN | Accelerate reverse (same logic) |
| UP while reversing | Decelerate reverse |
| DOWN while forward | Decelerate forward |
| LEFT | CCW pivot 150ms then resume |
| RIGHT | CW pivot 150ms then resume |
| OK | Emergency stop + headlight flash |
| 1–9 | Speed presets |
| 0 | Stop |

---

## 📂 File Structure

```
main/
  acebott_main.c      — App entry point, IR control loop, ultrasonic logic
  acebott_hw.c        — Hardware drivers: motors, IR, ultrasonic, buzzer, LEDs
  acebott_hw.h        — Pin definitions, enums, function prototypes
docs/
  NOTES.md            — Project status and resume guide
  HARDWARE_MAP.md     — Complete GPIO pin reference
  HOW_IT_WORKS.md     — Architecture and logic overview
  Assemble the smart car.pdf
IR_REMOTE_CODES.txt   — All 17 confirmed IR button codes
```

---

## 🌿 Branch Structure

| Branch | Purpose |
|:---|:---|
| `master` | Stable — mirrors experiment-control |
| `experiment-control` | Active development |

---

## 🛠️ Hardware Specifications

| Component | Specification |
|:---|:---|
| Core Controller | ESP32-WROOM-32E |
| Drive System | 4WD Mecanum (Omni-directional) |
| Motor Driver | L298N H-Bridge via 74HC595 shift register |
| IR Remote | NEC protocol, 38kHz, 17 buttons |
| Distance Sensor | HC-SR04 Ultrasonic |
| Framework | ESP-IDF v5.4 (C) |
| Base Model | ACEBOTT QD001 |
