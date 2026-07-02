# Automatic Smart Billing Cart — ESP32-CAM + Edge Impulse

Real-time product recognition system using on-device AI inference on ESP32-CAM.
No cloud required — all inference runs directly on the microcontroller.

## What it does
- Captures live camera feed on ESP32-CAM
- Runs a custom-trained object detection model (Edge Impulse) on-device
- Recognizes specific products (soap, washing powder, household items)
- Serves detection results via a web server over WiFi for billing display

## Hardware
- ESP32-CAM (AI Thinker module)

## Tech Stack
- Embedded C / Arduino framework
- Edge Impulse (model training + deployment)
- WebServer library for ESP32
- CORS-enabled JSON API for frontend communication

## ML Model
- Trained on a custom image dataset of household products
- Deployed as a standalone WebAssembly + JS inference engine
- Runs fully on-device — no internet connection needed during inference

## How to run
1. Open `CameraWebServer_copy_...ino` in Arduino IDE
2. Add your WiFi credentials (SSID and password)
3. Flash to ESP32-CAM
4. Open the IP address shown in Serial Monitor in a browser

## Team
Built as a team project
Parshw Sangame,
Sumit Sungar,
Siddharth N,
Sugureshwara H.
— B.E. Electronics Engineering, BIT Bangalore
