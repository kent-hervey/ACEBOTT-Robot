# ACEBOTT-Robot Project Notes
### Last Updated: March 28, 2026
### "Parking Spot" — safe to walk away and resume later

---

## 🚦 Current State: WORKING — FULLY DRIVEABLE WITH OBSTACLE AVOIDANCE

The robot is fully functional as of March 28, 2026:
- IR remote controls all movement (UP/DOWN/LEFT/RIGHT/OK)
- Number buttons 1-9 set speed presets, 0 stops
- Ultrasonic sensor auto-stops when obstacle < 15cm
- Proximity warning beeps as you approach 15-30cm range
- Auto-resumes at speed 150 when obstacle clears
- Headlights on at startup, flash on emergency stop

---

## 🌿 Git Branch Map

| Branch | Status | What's In It |
|---|---|---|
| `experiment-control` | ✅ **Active development** | Full working code — use this |
| `master` | ✅ Stable | Should be merged from experiment-control |
| `feature-ir-remote` | 🗑️ Delete me | Obsolete — worse than experiment-control |
| `thursday-accel-logic` | 🗑️ Delete me | Obsolete — same as experiment-control base |

### Cleanup commands (run once):
```bash
git branch -d feature-ir-remote
git branch -d thursday-accel-logic
git push origin --delete feature-ir-remote
git push origin --delete thursday-accel-logic
```

---

## 📁 File Structure

```
main/
  acebott_main.c        — App entry point, IR control loop, ultrasonic logic
  acebott_hw.c          — ALL hardware drivers: motors, IR, ultrasonic, beeper, LEDs
  acebott_hw.h          — Pin definitions, enums, function prototypes
  secrets.h             — WiFi credentials (gitignored, not currently used)
  secrets.h.template    — Safe template
docs/
  NOTES.md              — This file
  HARDWARE_MAP.md       — GPIO pin reference
  HOW_IT_WORKS.md       — Architecture overview
  assets/               — Images
  Assemble the smart car.pdf
IR_REMOTE_CODES.txt     — All 17 confirmed IR codes (repo root)
```

---

## 🔑 Key Toggles in acebott_main.c

```c
#define USE_ULTRASONIC  1   // 0 = ignore sensor, 1 = auto-stop at 15cm
#define USE_MOTORS      1   // 0 = log only (bench test), 1 = real movement
```

Set `USE_MOTORS 0` when testing IR codes on your desk without risking the car driving off.
Set `USE_ULTRASONIC 0` if sensor is unplugged (avoids -1.0 timeout spam).

---

## ✅ All 17 IR Remote Codes (Confirmed March 28, 2026)

NEC protocol, receiver on GPIO 4. All tested and verified against hardware.

| Button | Code | Action |
|---|---|---|
| UP | `0x00FF629D` | Accelerate forward (torque jump to 150, then +50) |
| DOWN | `0x00FFA857` | Accelerate reverse (same logic) |
| LEFT | `0x00FF22DD` | CCW nudge 150ms then resume |
| RIGHT | `0x00FFC23D` | CW nudge 150ms then resume |
| OK | `0x00FF02FD` | Emergency stop + headlight flash |
| STAR (*) | `0x00FF42BD` | Double beep test |
| HASH (#) | `0x00FF52AD` | Headlight flash test |
| 0 | `0x00FF4AB5` | Stop |
| 1 | `0x00FF6897` | Speed preset 150 (MIN) |
| 2 | `0x00FF9867` | Speed preset 150 (MIN) |
| 3 | `0x00FFB04F` | Speed preset 150 (MIN) |
| 4 | `0x00FF30CF` | Speed preset 150 (MIN) |
| 5 | `0x00FF18E7` | Speed preset 155 |
| 6 | `0x00FF7A85` | Speed preset 180 |
| 7 | `0x00FF10EF` | Speed preset 200 |
| 8 | `0x00FF38C7` | Speed preset 228 |
| 9 | `0x00FF5AA5` | Speed preset 255 (MAX) |

Note: Presets 1-4 all resolve to MIN_MOTOR_SPEED (150) since their mapped
values are below stall threshold.

---

## 🔌 GPIO Pin Assignments (acebott_hw.h is source of truth)

| Signal | GPIO | Notes |
|---|---|---|
| Motor EN | 16 | |
| Motor DATA | 5 | Shift register data |
| Motor LATCH | 17 | Shift register latch |
| Motor CLOCK | 18 | Shift register clock |
| PWM1 | 19 | Speed control |
| PWM2 | 23 | Speed control |
| Ultrasonic TRIG | 13 | |
| Ultrasonic ECHO | 14 | |
| Line Sensor L | 35 | Input only, no pullup |
| Line Sensor M | 36 | Input only, no pullup |
| Line Sensor R | 39 | Input only, no pullup |
| IR Receiver | 4 | |
| Servo | 25 | Not yet implemented |
| Headlight L | 12 | |
| Headlight R | 2 | |
| Buzzer | 33 | |

> ⚠️ HARDWARE_MAP.md previously had wrong pin numbers — now corrected.

---

## 🔧 Motor Direction Values (74HC595 shift register patterns)

| Direction | Value |
|---|---|
| STOP | 0 |
| FORWARD | 163 |
| BACKWARD | 92 |
| CW (spin right) | 172 |
| CCW (spin left) | 83 |

---

## 🛠️ Speed Constants (tuned March 28, 2026)

```c
#define SPEED_STEP         50   // Added per UP/DOWN press
#define MIN_MOTOR_SPEED   150   // Confirmed stall threshold for this car
#define MAX_MOTOR_SPEED   255
#define STEER_NUDGE_MS    150   // Duration of LEFT/RIGHT pivot
#define STEER_NUDGE_SPEED 160   // Speed during pivot
```

---

## 🦇 Ultrasonic Behavior

- Sensor: HC-SR04, TRIG=13, ECHO=14
- Shield version: Car-Shield v1.1 (Wiring 2 diagram)
- **Proximity warning:** 15-30cm → beeping speeds up as you get closer
- **Auto-stop:** < 15cm → stops, low buzz, sets `stopped_by_obstacle = true`
- **Auto-resume:** obstacle clears > 25cm → resumes at speed 150, two ascending beeps
- **Reverse:** ultrasonic does NOT stop reverse movement (intentional)
- **Manual override:** OK button stops car and clears `stopped_by_obstacle` flag

---

## 📋 TODO — Resume Here

### Immediate
- [ ] Take the car off the support stand and do a real floor drive test
- [ ] Tune `STEER_NUDGE_MS` if turns feel too short or too long on floor
- [ ] Verify auto-resume feels safe at speed 150 (may want to lower to 120)

### Short term
- [ ] STAR (*) currently just beeps — assign it something useful
  - Suggestion: toggle line-following mode on/off
- [ ] HASH (#) currently flashes headlights — assign something useful
  - Suggestion: toggle ultrasonic on/off (useful for testing)
- [ ] Add headlight state feedback:
  - Forward → solid ON (already done at startup)
  - Reverse → slow blink
  - Stopped → OFF or dim

### Medium term
- [ ] Servo pan obstacle avoidance
  - Stop, scan left (45°) then right (135°), pivot toward open side
  - Servo on GPIO 25, PWM 500µs=0°, 2500µs=180°, 50Hz
- [ ] Line following mode
  - Sensors wired and ADC reads working
  - Logic stubbed in comments in main loop (Section 5)
  - Tune `black_line_threshold` (currently 2000 in comments)

### Future
- [ ] OTA updates over WiFi (no more USB cable)
- [ ] Migrate from legacy RMT/ADC drivers to ESP-IDF v5.4 new APIs

---

## 🚀 How to Resume

```bash
cd ~/CLionProjects/ACEBOTT-Robot
git switch experiment-control
source ~/esp/esp-idf/export.sh      # only if idf.py not found
idf.py -p /dev/cu.usbserial-14340 flash monitor | cat
```

**Always pipe through `| cat`** to avoid the interactive pager stopping output.

Serial port: `/dev/cu.usbserial-14340` (may change if you replug USB).

If port is busy: check for old monitor sessions in other terminal tabs.
Kill with `Ctrl+]` in the tab that has it, or find with `lsof | grep usbserial`.

---

## 📦 Before Walking Away — Checklist

```bash
cd ~/CLionProjects/ACEBOTT-Robot

# 1. Commit any changes
git add -A
git commit -m "wip: describe what you were doing"

# 2. Merge experiment-control into master
git switch master
git merge experiment-control
git push

# 3. Push experiment-control
git switch experiment-control
git push

# 4. Verify GitHub is up to date
git log --oneline origin/master..master | cat   # should be empty
git log --oneline origin/experiment-control..experiment-control | cat  # should be empty
```
