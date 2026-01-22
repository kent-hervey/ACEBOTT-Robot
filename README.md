# ESP32 H-Bridge Web Controller

### Purpose
A foundational building block for controlling an ESP32 to open, close, or toggle devices (like motors or actuators) via an H-Bridge configuration. The system is accessible via a web page on the same network or through a direct WiFi connection (SoftAP) to the ESP32.

## Features
- **Dual Mode WiFi:** Supports connecting to an existing router or acting as a standalone Access Point.
- **H-Bridge Logic:** Controls two GPIO pins (18 & 19) for Forward/Reverse/Stop functionality.
- **Safety Timeout:** Automatic 5-second shutoff to prevent hardware strain.
- **Visual Feedback:** Built-in LED (GPIO 2) blinks at different speeds based on motor direction.
- **Mobile-Friendly UI:** Large buttons designed specifically for use on smartphones.

## Hardware Requirements
- ESP32 Development Board
- Dual-channel Relay Module or H-Bridge Motor Driver
- Power supply suitable for your specific motor/actuator

## Setup
1. Create a `main/secrets.h` file (refer to `secrets.h.template`).
2. Define your WiFi credentials and preferred mode (SoftAP vs STA).
3. Build and flash using ESP-IDF.