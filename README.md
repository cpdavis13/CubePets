# CubePets
A real-time multiplayer embedded game built on ESP32 microcontrollers using C++.

![CubePets demo](https://github.com/user-attachments/assets/bf6f668c-2633-4386-b83d-f33c17279af5)

## Overview

CubePets is a keychain-sized multiplayer embedded game system built on ESP32 microcontrollers. Multiple devices communicate over wired UART and GPIO connections to synchronize game state and player actions in real time. The system is designed to support dynamic connection and disconnection in arbitrary configurations, supporting immediate play and flexible network topologies.

This project explores timing-sensitive embedded design, finite state machines, and reliable synchronization under the hardware, latency, and resource constraints typical of microcontroller-based systems.

> **Status:** In Progress — core gameplay and communication implemented; continuing to improve reliability and expand features.

---

## Features

- Real-time multiplayer gameplay using ESP32 microcontrollers  
- Dynamic host / client mesh architecture for game state coordination  
- Robust communication with heartbeat monitoring, automatic retries, and dynamic host election for reconnection
- Modular source structure separating gameplay and communication layers  
- Designed for low-latency, resource-constrained embedded systems  

---

## Building & Running

### Requirements
- ESP32 development board  
- PlatformIO or Arduino IDE  
- USB cable for flashing  

### Steps
1. Clone the repository  
2. Open the project in PlatformIO  
3. Upload the firmware to one or more ESP32 devices
4. Configure pins to match hardware 
5. Power up and connect to form a network and watch
   
---

## Next Planned Work

- Full prototypes with batteries, magnetic GPIO pins and buttons
- Device limit stress testing
- Advanced character AI
- Expanded character catalogue
- Custom PCB

---

## Why This Matters

This project demonstrates practical embedded engineering skills, including:
- Wired communication under real-time constraints  
- Embedded state machine and timing-driven design  
- Modular firmware architecture  
- Hardware–software integration on resource-limited platforms  

CubePets is intended both as a learning project and as a portfolio demonstration of real-world embedded systems development.

---

## License

MIT License
