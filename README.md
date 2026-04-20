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
````

### 2\. Clone & Build

```bash
git clone [https://github.com/Bertrandifinity/Real-Time-Pitch-to-Light-System-for-Kalimba.git](https://github.com/Bertrandifinity/Real-Time-Pitch-to-Light-System-for-Kalimba.git)
cd Real-Time-Pitch-to-Light-System-for-Kalimba

# Compile using g++ with maximum optimization (-O3) and required linked libraries
g++ -O3 -o piano_lights src/piano_lights.cpp -lasound -lfftw3 -lm -lpthread
```

### 3\. Execution

The application requires hardware access privileges for native `pinctrl` GPIO manipulation.

```bash
sudo ./piano_lights
```

-----

---

## 👥 Team Contributions & Project Management
Our project was driven by a clear division of labor, formal project management (using GitHub Issues and branching strategies), and collaborative teamwork.

* **Yuanxu Li (3130055L):** *Team Leader & Software Architecture*
  * Oversaw the implementation of SOLID principles, designed the core C++ class structures, and managed the GitHub repository (merging branches, creating formal releases).
* **Liyang Hu (3075651H):** *Real-Time DSP & Algorithm Design*
  * Implemented the FFTW3 Fast Fourier Transform logic, developed the quantitative pitch detection algorithm, and successfully isolated Kalimba fundamental frequencies from harmonic noise.
* **Mingqi Duan (3075644D):** *Hardware Integration & Event Handling*
  * Handled the physical I2S microphone wiring and configured the Linux `pinctrl` subsystem for responsive, event-driven GPIO LED triggering.
* **Changqing Bao (3140708B):** *Multithreading & Memory Management*
  * Designed the lock-free multithreading architecture between the audio and visualizer threads. Ensured failsafe memory management and zero memory leaks.
* **Zheng Jin (3136460J):** *QA Testing, Calibration & PR Strategy*
  * Conducted unit testing and acoustic calibration to map exact Kalimba frequencies. Directed the public relations strategy and produced the social media video content.

---
## 📱 Promotion & Social Media

We believe in building in public\! Check out our development process, live demos, and tutorials on our social channels:

  * 🎥 **YouTube Channel & Live Demos:** [Subscribe and watch our project in action on YouTube (@Bcq-1122)](https://www.google.com/url?sa=E&source=gmail&q=https://www.youtube.com/@Bcq-1122)
  * 📰 **Community:** Keep an eye out for our upcoming project breakdown\!

---
## 📱 Android Companion App (Bluetooth Integration)
To enhance user interaction, we developed a companion Android application using **Android Studio (Kotlin)**. The app connects to the Raspberry Pi via Bluetooth to display the currently detected Kalimba note.
* **Core Source Files:** The project utilizes `MainActivity.kt` (Bluetooth logic & UI updates), `activity_main.xml` (Layout UI), and `AndroidManifest.xml` (Hardware permissions handling).
* **Pre-compiled Artifact:** For ease of testing and reproducibility, a ready-to-install `app-debug.apk` is provided directly in the repository.
* **Engineering Constraint Analysis (Latency):** While our onboard C++ hardware LED visualization achieves strict real-time performance, the Bluetooth broadcasting protocol introduces an inherent propagation delay of several hundred milliseconds. Consequently, we designed the Android app as a secondary diagnostic/logging display, while the GPIO-driven LEDs serve as the primary real-time performance feedback mechanism.

<!-- end list -->
