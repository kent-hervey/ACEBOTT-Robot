# 🛠️ ACEBOTT ESP32-Car Hardware Map

This document serves as the master "Source of Truth" for the GPIO pin assignments on the **ACEBOTT ESP32 Max V1.0** controller.

---

## 🏎️ Drive System (Mecanum Wheels)
The robot uses a shift-register based motor driver (74HC595) to control direction and two PWM channels for speed.

| Component | GPIO Pin | Function |
| :--- | :--- | :--- |
| **Enable (EN)** | `GPIO 16` | Driver Enable |
| **Data (SER)** | `GPIO 5` | Shift Register Data |
| **Latch (RCLK)** | `GPIO 17` | Shift Register Latch |
| **Clock (SRCLK)** | `GPIO 18` | Shift Register Clock |
| **PWM Left** | `GPIO 19` | Speed control (Left Side) |
| **PWM Right** | `GPIO 23` | Speed control (Right Side) |

---

## 👁️ Sensors & Logic (Navigation)

### 🦇 Ultrasonic Obstacle Avoidance
* **Trig (Trigger):** `GPIO 13`
* **Echo:** `GPIO 14`  *(Updated per hardware shield)*
* **Servo (Head):** `GPIO 25` (Rotates the sensor)

### 🛤️ Line Tracing (Under-Chassis)
* **Left Sensor:** `GPIO 35`
* **Middle Sensor:** `GPIO 36`
* **Right Sensor:** `GPIO 39`

### 🎮 Remote Control
* **IR Receiver:** `GPIO 4`

---

## 📡 Outputs & Feedback

### 📢 Sound & Light
* **Buzzer:** `GPIO 33`
* **Headlight (Left):** `GPIO 12`
* **Headlight (Right):** `GPIO 2`

---

## 📖 Assembly Instructions
For physical construction and wiring diagrams, refer to the official manual:
* 📄 **[Assemble the Smart Car (PDF)](<Assemble the smart car.pdf>)**

---

## ⚠️ Development Notes
* **Input Only:** GPIOs 35, 36, and 39 are input-only pins on the ESP32 and lack internal pull-ups.
* **Strapping Pins:** GPIO 12 and GPIO 2 are strapping pins. Ensure they are not pulled high/low during boot in a way that interferes with the ESP32 flash mode.