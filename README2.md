# ESP32 H-Bridge Web Server (Building Block)

## Purpose
This project serves as a foundational **learning and documenting building block** for an ESP-IDF web-controlled motor system. It was designed to solve specific networking challenges between the **ESP32** and **Eero Mesh Wi-Fi** while implementing a C-based HTTP server to control a motor H-bridge.

## Technical Highlights
* **Framework:** Native ESP-IDF (C) using `esp_netif` and `esp_http_server`.
* **Networking:** Implemented **802.11b disabling** and **country code alignment (US)** to ensure stability on high-speed Eero/Frontier Fiber networks.
* **Fallback Mode:** Supports **SoftAP fallback** (Operation Mode 2) if the primary Station connection fails.
* **Web UI:** Interactive HTML/CSS interface served directly from the ESP32.

## Project Structure
* `main/esp32_web_server.c`: Core logic for Wi-Fi and HTTP handlers.
* `main/secrets.h`: (Excluded from Git) Contains sensitive Wi-Fi credentials.
* `main/secrets.h.template`: Template for configuring local credentials.

## Setup Instructions
1. Copy `main/secrets.h.template` to `main/secrets.h`.
2. Update the SSID and Password with your credentials.
3. Use **CLion** or the **ESP-IDF CLI** to build and flash.
4. **Recommendation:** Set a **DHCP Reservation** in your router (e.g., Eero App) for the ESP32's MAC address to ensure a consistent IP (e.g., `192.168.4.33`).