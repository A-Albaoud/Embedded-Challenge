#include <Arduino.h>
#include "fft_processor.h"   // Your FFT header

// -----------------------------------------------------
// CONFIGURATION FOR SYNTHETIC TEST SIGNAL GENERATION
// -----------------------------------------------------
const size_t N = 256;         // FFT size
const float FS = 52.0f;       // Sampling rate (52 Hz)

// Arrays for synthetic 3D IMU data
float ax[N];
float ay[N];
float az[N];

// -----------------------------------------------------
// Create synthetic 3D motion with known tremor frequency
// -----------------------------------------------------
void generateTestData(float tremorFreq = 4.0f, float dyskFreq = 0.0f) {
    for (size_t i = 0; i < N; i++) {

        float t = (float)i / FS;

        // Simulated tremor (4 Hz default)
        float tremor = 0.5f * sinf(2.0f * PI * tremorFreq * t);

        // Optional dyskinesia (e.g., 6 Hz)
        float dysk = 0.0f;
        if (dyskFreq > 0.0f) {
            dysk = 0.3f * sinf(2.0f * PI * dyskFreq * t);
        }

        // Small noise
        float noise = 0.05f * ((float)random(-100, 100) / 100.0f);

        // Assign each axis differently
        ax[i] = tremor + dysk + noise;          // full signal
        ay[i] = (tremor * 0.8f) + noise * 1.2f; // scaled tremor
        az[i] = (tremor * 0.4f) + (dysk * 0.3f) + noise;
