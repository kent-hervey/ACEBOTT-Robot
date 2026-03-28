# 🛠️ ACEBOTT ESP32-Car Hardware Map
### Last Updated: March 28, 2026
### Source of Truth: `main/acebott_hw.h`

---

## 🏎️ Drive System (Mecanum Wheels)

The robot uses a **74HC595 shift register** driving an **L298N H-Bridge**.
Front and rear motors on each side are wired in parallel for 4WD control
through a 2-channel driver. Speed is controlled via LEDC PWM.

| Signal | GPIO | Notes |
|:---|:---|:---|
| Motor EN | 16 | Enable — held LOW to activate |
| Motor DATA | 5 | Shift register serial data |
| Motor LATCH | 17 | Shift register latch (output enable) |
| Motor CLOCK | 18 | Shift register clock |
| PWM Speed 1 | 19 | LEDC Channel 0, 1kHz, 8-bit |
| PWM Speed 2 | 23 | LEDC Channel 1, 1kHz, 8-bit |

### Motor Direction Byte Values (shift register patterns)

| Direction | Value | Description |
|:---|:---|:---|
| STOP | 0 | All motors off |
| FORWARD | 163 | All wheels forward |
| BACKWARD | 92 | All wheels reverse |
| CW | 172 | Spin clockwise (right turn) |
| CCW | 83 | Spin counter-clockwise (left turn) |

### Speed Range
- **0** = stopped
- **150** = minimum effective speed (confirmed stall threshold March 28, 2026)
- **255** = maximum speed

---

## 🦇 Ultrasonic Obstacle Avoidance (HC-SR04)

Mounted on servo bracket at front of car. Servo panning not yet implemented.

| Signal | GPIO | Notes |
|:---|:---|:---|
| TRIG | 13 | 10µs pulse to trigger |
| ECHO | 14 | Pulse width = distance |

**Wiring:** Uses Car-Shield **v1.1** (Wiring 2 diagram from assembly manual).
Distance formula: `cm = pulse_duration_us / 58.0`
Timeout: returns -1.0f if no echo within 25ms.

### Behavior Thresholds
| Distance | Behavior |
|:---|:---|
| > 30cm | Normal driving, no warning |
| 15–30cm | Proximity warning beeps (faster as closer) |
| < 15cm | AUTO-STOP, low buzz |
| > 25cm (after stop) | Auto-resume at speed 150, two ascending beeps |

---

## 🛤️ Line Tracing Sensors (Under-Chassis)

Three IR reflectance sensors. **Input-only pins — no internal pull-ups available.**

| Sensor | GPIO | ADC Channel |
|:---|:---|:---|
| Left | 35 | ADC1_CHANNEL_7 |
| Middle | 36 | ADC1_CHANNEL_0 |
| Right | 39 | ADC1_CHANNEL_3 |

ADC configured: 12-bit width, 12dB attenuation.
Line following logic is stubbed in `acebott_main.c` (currently disabled).

---

## 📡 IR Remote Control

| Signal | GPIO | Protocol |
|:---|:---|:---|
| IR Receiver | 4 | NEC, 38kHz |

All 17 remote buttons confirmed working. See `IR_REMOTE_CODES.txt` for
complete code table. Codes also defined in `ir_button_t` enum in `acebott_hw.h`.

RMT peripheral used for decoding. Uses legacy driver (works, deprecation
warning on build — migration to `driver/rmt_rx.h` is a future task).

---

## 🔵 Headlights (Blue LED Modules)

| Signal | GPIO | Notes |
|:---|:---|:---|
| Left Headlight | 12 | Active HIGH |
| Right Headlight | 2 | Active HIGH |

Both controlled together via `acebott_set_headlights(bool)` and
`acebott_flash_headlights(count, duration_ms)`.

---

## 🔊 Buzzer

| Signal | GPIO | Notes |
|:---|:---|:---|
| Passive Buzzer | 33 | Bit-banged PWM via `acebott_beep(freq, ms)` |

Frequency range tested: 600Hz (low warning) to 3000Hz (high alert).

---

## ⚙️ Servo Motor (SG90)

| Signal | GPIO | Notes |
|:---|:---|:---|
| Servo Signal | 25 | PWM: 500µs=0°, 2500µs=180°, 50Hz |

Servo is physically mounted and wired but **not yet implemented in firmware**.
Planned use: pan ultrasonic sensor for obstacle avoidance scanning.

---

## ⚠️ Development Notes

- GPIOs 35, 36, 39 are **input-only** — no internal pull-ups possible
- GPIO 2 is a headlight, **not** a diagnostic LED (common mistake)
- Car-Shield version matters for wiring: check `ACEBOTT-ESP32 Car-Shield-x.x`
  printed on your board. This project uses **v1.1** (Wiring 2 diagram)
- Legacy RMT and ADC drivers produce deprecation warnings on every build —
  harmless for now, migration is a future task
