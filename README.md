# ACEBOTT-Robot 🏎️

This repository contains the ESP-IDF firmware for the **ACEBOTT ESP32 Robot Car**.

## 📌 Project Overview
The goal of this project is to provide a robust C-based firmware that allows for real-time control of the ACEBOTT car via a built-in web server. This allows you to drive the robot and monitor its sensors directly from a mobile phone or computer browser.

## 📖 Project Documentation
To make this project accessible to everyone, we have created a dedicated guide for the non-technical side of the ACEBOTT:

* **[How It Works (Technical & Functional Overview)](docs/HOW_IT_WORKS.md)**:
  A visual guide explaining the robot's hardware components, sensor priorities (Obstacle Avoidance vs. Line Following), and how the IR remote handles smooth acceleration.

## 🛠️ Features
- **WiFi Web Server:** Hosted directly on the ESP32.
- **Motor Control:** Logic for forward, backward, and steering.
- **ESP-IDF v5.4:** Built using the latest stable framework.
- **Clean Architecture:** Organized for easy expansion with new sensors.

## 🚀 Getting Started
To get this code onto your robot car, follow these steps:

### Prerequisites
- **CLion IDE** with the **ESP-IDF Plugin** installed.
- **ESP-IDF v5.4** configured in your environment.

### Installation & Build
1. **Clone the repo:**
   `git clone https://github.com/kent-hervey/ACEBOTT-Robot.git`
2. **Open in CLion:** Open the folder and wait for the CMake sync to finish.
3. **Build:** Click the **Hammer icon** (Build) to generate the binary.
4. **Flash:** Connect your ESP32 via USB and click the **Play icon** (Flash).

## 📂 File Structure
- `main/acebott_main.c`: The primary application logic.
- `docs/`: User-friendly guides and documentation.
   - `assets/`: Images, diagrams, and photos of the ACEBOTT car.
- `CMakeLists.txt`: Project configuration and naming.