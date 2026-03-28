# ACEBOTT-Robot Project Notes
### Last Updated: March 28, 2026
### "Parking Spot" — safe to walk away and resume later

---

## 🚦 Current State: PARTIALLY WORKING — MOTOR LOGIC IS ONE COMMIT BEHIND

Your current HEAD (`1a9d1ce` on `feature-ir-remote`) is a STRIPPED-DOWN version
with no motor control logic. The GOOD version with full IR→motor logic is in the
commit immediately behind it: `1071ec2`.

**The single most important thing to do when you resume:**
```bash
git reset --hard 1071ec2
# or just cherry-pick what you want from it
```

---

## 🌿 Git Branch Map

| Branch | Commit | What's In It |
|---|---|---|
| `feature-ir-remote` HEAD | `1a9d1ce` | ⚠️ Stripped — IR decoding works, NO motor logic |
| `feature-ir-remote` prev | `1071ec2` | ✅ **THE GOOD ONE** — torque jump + full UP/DOWN/OK logic |
| `thursday-accel-logic` | `cb7bf5d` | Shares same base as experiment-control, docs update only |
| `experiment-control` | `cb7bf5d` | Has unstaged changes to acebott_main.c (stashed) |
| `master` | `f7d5240` | Old WiFi web server — not relevant |

### ⚠️ Git Housekeeping Needed Before Walkaway
```bash
# Push current branch to GitHub (it's 1 ahead of origin)
git push

# Clean up the stash on experiment-control
git switch experiment-control
git stash drop    # or: git stash pop then decide to keep or discard
git switch feature-ir-remote
```

---

## 📁 File Structure

```
main/
  acebott_main.c        — App entry point, startup sequence, main IR control loop
  acebott_hw.c          — ALL hardware drivers: motors, IR, ultrasonic, beeper, LEDs
  acebott_hw.h          — Pin definitions, enums (ir_button_t, motor_dir_t), prototypes
  secrets.h             — WiFi credentials (gitignored)
  secrets.h.template    — Safe template to commit
docs/
  HARDWARE_MAP.md       — GPIO reference (⚠️ OUT OF DATE — see pin table below)
  HOW_IT_WORKS.md       — Architecture overview
  assets/
    Assemble the smart car.pdf
IR_REMOTE_CODES.txt     — All 17 confirmed IR codes (repo root)
NOTES.md                — This file
```

---

## ✅ All 17 IR Remote Codes (All Confirmed Working)

Using **NEC protocol**, IR receiver on **GPIO 4**.
These are also in `IR_REMOTE_CODES.txt` and the `ir_button_t` enum in `acebott_hw.h`.

| Button | Code | In enum? |
|---|---|---|
| UP | `0x00FF629D` | ✅ |
| DOWN | `0x00FFA857` | ✅ |
| LEFT | `0x00FF22DD` | ✅ |
| RIGHT | `0x00FFC23D` | ✅ |
| OK | `0x00FF02FD` | ✅ |
| STAR (*) | `0x00FF42BD` | ✅ |
| HASH (#) | `0x00FF52AD` | ✅ |
| 0 | `0x00FF4AB5` | ❌ not in enum yet |
| 1 | `0x00FF6897` | ❌ not in enum yet |
| 2 | `0x00FF9867` | ❌ not in enum yet |
| 3 | `0x00FFB04F` | ❌ not in enum yet |
| 4 | `0x00FF30CF` | ❌ not in enum yet |
| 5 | `0x00FF18E7` | ❌ not in enum yet |
| 6 | `0x00FF7A85` | ❌ not in enum yet |
| 7 | `0x00FF10EF` | ❌ not in enum yet |
| 8 | `0x00FF38C7` | ❌ not in enum yet |
| 9 | `0x00FF5AA5` | ❌ not in enum yet |

---

## 🔌 GPIO Pin Assignments (from acebott_hw.h — trust this, not HARDWARE_MAP.md)

| Signal | GPIO |
|---|---|
| Motor EN | 16 |
| Motor DATA | 5 |
| Motor LATCH | 17 |
| Motor CLOCK | 18 |
| PWM1 | 19 |
| PWM2 | 23 |
| Ultrasonic TRIG | 13 |
| Ultrasonic ECHO | 14 |
| Line Sensor L | 35 |
| Line Sensor M | 36 |
| Line Sensor R | 39 |
| IR Receiver | 4 |
| Servo | 25 |
| Headlight L | 12 |
| Headlight R | 2 |
| Buzzer | 33 |

> ⚠️ HARDWARE_MAP.md lists different pin numbers — it is out of date. Always
> trust acebott_hw.h as the source of truth.

---

## 🔧 Motor Direction Values (shift register patterns for 74HC595 + L298N)

| Direction | Value |
|---|---|
| STOP | 0 |
| FORWARD | 163 |
| BACKWARD | 92 |
| CW (spin right) | 172 |
| CCW (spin left) | 83 |

---

## 🧠 The Working Motor Logic (from commit 1071ec2)

This is the "Holy Grail" code. Key constants:

```c
const int speed_step = 50;       // Speed change per button press
const int min_motor_speed = 100; // Jump-start speed to overcome gear friction
```

**The Torque Jump:** When speed is 0 and you press UP or DOWN, instead of
starting at speed 0 (which stalls), it jumps immediately to `min_motor_speed`
(100). This is the key insight that makes the car feel responsive.

**UP button behavior:**
- If stopped or going forward → accelerate forward (jump to 100, then +50 each press, max 255)
- If going backward → decelerate (subtract 50, stop at stall zone)

**DOWN button behavior:**
- If stopped or going backward → accelerate backward (same torque jump)
- If going forward → decelerate

**OK button:** Emergency stop — speed=0, direction=STOP, flash headlights 2x

**LEFT/RIGHT:** Logged but NOT YET IMPLEMENTED in 1071ec2 — just beeps.
This is the next thing to code.

**Ultrasonic:** Controlled by `#define ENABLE_ULTRASONIC 0` — set to 1 when
sensor is plugged in. When 0, distance checks are skipped.

**Line following:** Fully stubbed out in comments in the main loop (Section 5).
The sensors are wired and ADC reads work — just needs the logic uncommented
and tuned.

---

## 🛠️ All Available Hardware Functions (acebott_hw.h)

```c
void acebott_init(void);                              // Call first, always
void acebott_move(motor_dir_t dir, uint8_t speed);    // speed: 0–255
void acebott_beep(uint32_t freq, uint32_t duration_ms);
void acebott_set_headlights(bool on);
void acebott_flash_headlights(int count, uint32_t duration_ms);
void acebott_read_line_sensors(uint32_t *l, uint32_t *m, uint32_t *r);
float acebott_get_distance(void);                     // Returns cm, -1.0 on timeout
ir_button_t acebott_get_ir_command(void);             // Returns IR_CMD_NONE if nothing
```

---

## 📋 TODO — Resume Here (in priority order)

### Step 1: Get back to the good code
```bash
cd ~/CLionProjects/ACEBOTT-Robot
git switch feature-ir-remote
git reset --hard 1071ec2
# Verify motor logic is back:
grep -n "min_motor_speed" main/acebott_main.c
```

### Step 2: Implement LEFT and RIGHT (they just beep currently)
Gemini proposed "momentary nudge" turns — car pivots briefly then resumes:
```c
case IR_CMD_LEFT:
    acebott_move(MOTOR_CCW, 150);
    vTaskDelay(pdMS_TO_TICKS(150));
    acebott_move(current_dir, current_speed);  // resume previous
    break;

case IR_CMD_RIGHT:
    acebott_move(MOTOR_CW, 150);
    vTaskDelay(pdMS_TO_TICKS(150));
    acebott_move(current_dir, current_speed);
    break;
```

### Step 3: Verify DOWN button works on real hardware
Gemini noted DOWN was unresponsive in testing. May be a decoding issue
or the deceleration logic zeroing speed too aggressively. Test and tune.

### Step 4: Add numbers 0–9 to ir_button_t enum in acebott_hw.h
```c
IR_CMD_0 = 0x00FF4AB5,
IR_CMD_1 = 0x00FF6897,
IR_CMD_2 = 0x00FF9867,
IR_CMD_3 = 0x00FFB04F,
IR_CMD_4 = 0x00FF30CF,
IR_CMD_5 = 0x00FF18E7,
IR_CMD_6 = 0x00FF7A85,
IR_CMD_7 = 0x00FF10EF,
IR_CMD_8 = 0x00FF38C7,
IR_CMD_9 = 0x00FF5AA5,
```
Then decide what they do (speed presets? mode switching?)

### Step 5: Headlight state feedback (Gemini design notes)
- Forward → solid ON
- Backward → slow blink (reverse warning)
- Stopped → OFF or dim
- Line lost → rapid flash

### Step 6 (Future): Obstacle avoidance with servo pan
- Set `#define ENABLE_ULTRASONIC 1`
- If distance < threshold: stop, pan servo left (45°) then right (135°)
- Compare distances, pivot toward open side
- Servo on GPIO 25. PWM: 500µs = 0°, 2500µs = 180°, 50Hz signal

### Step 7 (Future): Line following
- Uncomment Section 5 in main loop
- Tune `black_line_threshold` (currently 2000 in comments)
- Test with actual black line on floor

### Step 8 (Future): OTA updates
- Custom partitions.csv with ota_0 / ota_1
- Flash over WiFi — no more USB cable needed

---

## 🚀 How to Resume

```bash
cd ~/CLionProjects/ACEBOTT-Robot
git switch feature-ir-remote
source ~/esp/esp-idf/export.sh          # only if idf.py not found
idf.py flash monitor | cat              # build, flash, watch serial
```

Monitor only (no flash):
```bash
idf.py -p /dev/cu.usbserial-14340 monitor | cat
```

Useful git commands — always pipe through `cat` to avoid the interactive pager:
```bash
git show 1071ec2:main/acebott_main.c | cat
git log --oneline --all | cat
git diff HEAD~1 HEAD -- main/acebott_main.c | cat
```

Serial port: `/dev/cu.usbserial-14340` (may change if you replug USB)

---

## ⚠️ Known Issues & Warnings

- Legacy RMT and ADC driver deprecation warnings on every build. They work fine
  but ESP-IDF v5.4 has new APIs (`driver/rmt_tx.h`, `esp_adc/adc_oneshot.h`).
  Not urgent.
- `acebott_main.c` in commit 1071ec2 has a duplicate Section 5/6 block at the
  very bottom (copy-paste artifact from a git show). Safe to delete the duplicate.
- CLion 2025.3 upgrade may have lost VCS root mapping. If Claude Agent in CLion
  shows errors: Settings → Version Control → Directory Mappings → re-add project
  path, set VCS to Git. Then run:
  `git config --global --add safe.directory ~/CLionProjects/ACEBOTT-Robot`
- The `stable-manual-passive-dist` branch mentioned in Gemini sessions does NOT
  appear in git branch output — it may never have been fully committed or pushed.

---

## 📦 Before Walking Away — Complete Checklist

```bash
cd ~/CLionProjects/ACEBOTT-Robot

# 1. Copy this NOTES.md into the repo and commit it
# (download from Claude, then:)
cp ~/Downloads/NOTES.md ~/CLionProjects/ACEBOTT-Robot/NOTES.md
git add NOTES.md
git commit -m "docs: add project parking spot notes"

# 2. Push feature-ir-remote (it's 1 commit ahead of origin)
git push

# 3. Clean up experiment-control stash
git switch experiment-control
git stash drop
git switch feature-ir-remote

# 4. Confirm everything is pushed
git log --oneline origin/feature-ir-remote..HEAD | cat
# Empty output = fully synced to GitHub
```
