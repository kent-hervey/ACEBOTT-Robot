# 🛠️ ACEBOTT ESP32-Car Hardware Map

This document serves as the master "Source of Truth" for the GPIO pin assignments on the **ACEBOTT ESP32 Max V1.0** controller.

---

## 🏎️ Drive System (Mecanum Wheels)
The robot uses an **L298N H-Bridge** driver. To support 4WD with a 2-channel driver, the front and rear motors on each side are wired in parallel.

| Component | GPIO Pin | Function |
| :--- | :--- | :--- |
| **Left Side (Front & Rear)** | `GPIO 16`, `GPIO 17` | PWM Speed & Direction |
| **Right Side (Front & Rear)** | `GPIO 18`, `GPIO 19` | PWM Speed & Direction |

---

## 👁️ Sensors & Logic (Navigation)
These sensors allow for autonomous movement and environment scanning.

### 🦇 Ultrasonic Obstacle Avoidance
* **Trig (Trigger):** `GPIO 5`
* **Echo:** `GPIO 18` (Signal Return)
* **Servo Mount:** `GPIO 2` (Rotates the sensor 0°–180°)

### 🛤️ Line Tracing (Under-Chassis)
Used for the "Line Follower" mode to detect floor contrast.
* **Left Sensor:** `GPIO 34`
* **Middle Sensor:** `GPIO 35`
* **Right Sensor:** `GPIO 36`

---

## 📡 User Interface & Feedback
Components used for manual control and status indications.

### 🎮 Inputs
* **IR Receiver:** `GPIO 32` (NEC Protocol / 38kHz)

### 📢 Outputs
* **Passive Buzzer:** `GPIO 25` (Requires PWM for tonal feedback)
* **Left Blue LED:** `GPIO 13`
* **Right Blue LED:** `GPIO 12`

---

## 💡 IR Control & Sound Logic
When receiving commands via the IR Remote, the following feedback is implemented:

| IR Button | Action | Sound Feedback |
| :--- | :--- | :--- |
| **Up Arrow** | Increase Speed | Short High-Pitch Beep (ascending) |
| **Down Arrow** | Decrease Speed | Short Low-Pitch Beep (descending) |
| **OK / Stop** | Emergency Stop | Long, Flat Tone (warning) |

---

## ⚠️ Development Notes
* **Input Only:** GPIOs 34, 35, and 36 are input-only pins on the ESP32 and lack internal pull-ups.
* **Center Alignment:** Use `<p align="center">` for images in GitHub READMEs to ensure compatibility across browsers.