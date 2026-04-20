# 🎵 Real-Time Pitch-to-Light System for Kalimba

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/Language-C++14-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%205-cc2533.svg)](https://www.raspberrypi.org/)

A high-performance, real-time embedded system designed to capture live Kalimba audio, detect musical pitch, and instantly map notes to synchronized LED visual output. Developed for the **ENG 5220 Real Time Embedded Programming** course at the University of Glasgow.

---

## 🌟 The Pitch: Real-World Application
Tuning and learning the Kalimba is often challenging for beginners due to the instrument's unique resonance and harmonic characteristics. This project solves this real-world problem by providing:
1. **High-Precision Pitch Detection:** Accurately isolates the fundamental frequency of Kalimba tines (from C4 to B6), effectively filtering out attack noise and harmonic interference.
2. **Interactive Visual Learning:** Translates the 21 unique pitches into a real-time responsive LED light show. The animation mode (direction) and speed dynamically adapt to the octave and frequency played, creating an intuitive learning and performance experience.

---

## ⚙️ Real-Time Architecture & SOLID Design
To ensure strict real-time performance without audio dropouts or visual stuttering, the software architecture leverages C++ multi-threading and event-driven concepts:

* **Separation of Concerns (SOLID):** The system completely decouples the heavy DSP (Digital Signal Processing) audio capture thread from the GPIO hardware rendering thread. 
* **Lock-Free Concurrency:** Inter-thread communication is handled via `std::atomic` variables (`anim_mode`, `anim_speed`), eliminating blocking mutexes and guaranteeing deterministic execution time for the audio loop.
* **Event-Driven Backend:** Audio capture utilizes `libasound2` (ALSA) `plughw` buffers, reacting to hardware interrupts rather than inefficient CPU polling.
* **Fault Tolerance:** Includes robust I2S bus error recovery (`-EPIPE` handling) ensuring the system safely re-initializes memory and hardware without crashing during prolonged use.

---

## ⏱️ Latency Analysis
Achieving real-time responsiveness is critical for musical applications. 
* By optimizing the ALSA chunk size to `4096` at a sample rate of `48000Hz`, our FFT window allows for accurate pitch detection (~11Hz resolution) while maintaining a theoretical audio capture latency of just **~85 milliseconds**.
* The independent LED rendering thread updates GPIO states instantly based on atomic flags, ensuring visual feedback feels instantaneous to the human eye when a note is struck.

---

## 🛠 Hardware Setup & Reproducibility
To reproduce this project, you will need the following hardware components connected to a **Raspberry Pi 5**:

| Component | Quantity | Connection / GPIO Pin |
| :--- | :---: | :--- |
| **I2S MEMS Microphone** | 1 | `VCC`->3.3V, `GND`->GND, `BCLK`->Pin 12, `LRCL`->Pin 35, `DOUT`->Pin 38 |
| **LEDs** | 5 | Positive legs to GPIO `17, 27, 22, 23, 24` (via 220Ω resistors) -> GND |

---

## 🚀 Installation & Compilation

### 1. Prerequisites
Ensure your Raspberry Pi runs a modern Linux OS and install the required DSP and Audio libraries:
```bash
sudo apt-get update
sudo apt-get install build-essential libasound2-dev libfftw3-dev git -y

Youtube: www.youtube.com/@Bcq-1122  
The actual upload date of the video can be found in the Youtube.png.
