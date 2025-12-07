#include <Arduino.h>
#include "fft_processor.h"

// ------ GLOBAL VARIABLES ------------------------------------
const float FS = 52.0f;    // sampling frequency
const size_t N  = 256;     // chosen FFT size (upper bound power of 2)
const unsigned long SAMPLE_PERIOD_MS = 1000.0 / FS;  // ≈ 19.23 ms

// ------ Variables + Functions used for testing ---------------
float sampleBuffer[N];        // holds time-domain samples (up to 256)
const size_t N_SAMPLES = (size_t)(FS * 3.0f);  // 3 seconds * 52 Hz = 156

#include <math.h>

void generateSine(float* buf, size_t nSamples, float fs, float freqHz, float amplitude) {
  for (size_t i = 0; i < nSamples; i++) {
    float t = (float)i / fs;                     // time in seconds
    buf[i] = amplitude * sinf(2.0f * PI * freqHz * t);
  }
}



void setup() {
  
  // set up fft processor
  initFFT(N, FS);

}

void loop() {
  
  
  

}

