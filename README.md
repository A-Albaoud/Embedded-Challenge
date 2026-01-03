# Shake, Rattle, and Roll — Embedded Challenge (Fall 2025)
A real-time embedded prototype that analyzes motion data from an IMU and classifies **Parkinsonian tremor (3–5 Hz)** vs **dyskinesia (5–7 Hz)** using an FFT pipeline and a TFT touchscreen UI.

> Course project prototype — not a medical device.

## Hardware
- **MCU:** Adafruit Feather 32u4 Basic Proto  
- **Sensor:** Adafruit Triple-Axis Accelerometer (ADXL345)  
- **Display/UI:** Adafruit TFT FeatherWing (ILI9341 + TSC2007 resistive touch)

## What it does
- Samples accelerometer data at **52 Hz** over a **3-second window** (156 raw samples).
- Downsamples each window to **N = 64** samples for FFT.
- Runs an FFT-based analysis to estimate:
  - whether motion is **rhythmic in 3–7 Hz**, and
  - relative **band power** in tremor vs dyskinesia bands.
- Displays:
  - live waveform preview,
  - current classification state (REST / NORM / TREM / DYSK),
  - dominant rhythmic frequency (Hz),
  - a severity-style bar visualization.

## Repository structure
- `src/main.cpp` — sampling scheduler, downsampling, rest detection, calls FFT analysis, updates UI
- `src/fft_processing.cpp` — FFT windowing + frequency-domain bandpower extraction and rhythmic gating
- `src/sensor.cpp` — ADXL345 init + read
- `src/ui.cpp` — TFT + touch UI (clock screen + live analysis screen)

## Build & Flash (PlatformIO)
### Prerequisites
- VS Code + PlatformIO extension

### Steps
1. Clone and open in VS Code:
   ```bash
   git clone https://github.com/A-Albaoud/Embedded-Challenge.git
   cd Embedded-Challenge
   git checkout Main
