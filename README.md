# Shake, Rattle, and Roll — Parkinson’s Tremor & Dyskinesia Detector (Embedded Challenge Fall 2025)

Term project for **Embedded Challenge Fall 2025**: detect and quantify **tremor** vs **dyskinesia** using a single IMU (accelerometer + gyroscope) and display results on a TFT touchscreen in real time.

## Overview
Parkinson’s tremor is characterized by rhythmic oscillations in the **3–5 Hz** band, while dyskinesia tends to appear in the **5–7 Hz** band. This project samples motion data at **52 Hz**, processes **3-second windows**, and uses an **FFT-based** approach to estimate symptom presence and intensity.

## Key Features
- **IMU acquisition at 52 Hz** (3-axis accel/gyro)
- **3-second windowed FFT** for frequency-domain classification
- **Symptom classification**
  - Tremor band (3–5 Hz)
  - Dyskinesia band (5–7 Hz)
- **Intensity estimation** (e.g., peak magnitude or bandpower within each band)
- **Real-time TFT UI** for status + visualization
- Touchscreen interaction (no external peripherals)

## Hardware (fill in your exact parts)
- Development board: **[Your board model here]**
- IMU (accelerometer + gyroscope): **[Your sensor model here]**
- TFT + touch: **[Your TFT/touch model here]**
- Power: USB power bank / LiPo (per project constraints)

## Repo Structure
- `src/` — application entrypoint + runtime logic
- `include/` — project headers
- `lib/` — third-party / local libraries (if applicable)
- `platformio.ini` — PlatformIO environment configuration
- `test/` — (optional) unit/integration tests

## Signal Processing Approach (High-Level)
1. **Sample IMU signals at 52 Hz** for **3 seconds**.
2. Construct a feature signal (examples):
   - magnitude of acceleration: `sqrt(ax^2 + ay^2 + az^2)`
   - and/or magnitude of angular velocity: `sqrt(gx^2 + gy^2 + gz^2)`
3. Apply preprocessing as needed:
   - mean removal (DC offset)
   - optional windowing (Hann/Hamming)
4. Compute FFT over the 3-second segment (optionally zero-pad to power-of-2).
5. Extract frequency-domain measures:
   - **Peak** within 3–5 Hz and 5–7 Hz
   - and/or **bandpower** within those bands
6. Decision logic:
   - Tremor detected if tremor-band metric exceeds threshold
   - Dyskinesia detected if dyskinesia-band metric exceeds threshold
7. Display:
   - Current state (REST / TREMOR / DYSKINESIA)
   - Intensity meter(s)
   - Optional live waveform or spectrum plot

## UI / Interaction
Touchscreen-only interaction. Suggested flow (adapt to your implementation):
- Home screen: start/stop live analysis
- Live screen: symptom label + intensity + plot
- Optional calibration screen: establish baseline noise / thresholds

## Build & Run (PlatformIO)
### Prerequisites
- VS Code
- PlatformIO extension

### Steps
1. Clone the repository:
   ```bash
   git clone https://github.com/A-Albaoud/Embedded-Challenge.git
   cd Embedded-Challenge
   git checkout Main
