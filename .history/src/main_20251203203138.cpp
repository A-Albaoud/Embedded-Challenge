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

void runTremorTest() {
  Serial.println("=== Tremor Test: 4 Hz sine for 3 seconds ===");

  // 1. Generate 4 Hz sine wave
  generateSine(sampleBuffer, N_SAMPLES, FS, 4.0f, 1.0f);

  // 2. Run FFT analysis
  MovementAnalysis result = analyzeWindow(sampleBuffer, N_SAMPLES);

  // 3. Print results
  Serial.print("tremorPower = ");
  Serial.println(result.tremorPower, 6);
  Serial.print("dyskinesiaPower = ");
  Serial.println(result.dyskinesiaPower, 6);
  Serial.print("hasTremor = ");
  Serial.println(result.hasTremor ? "true" : "false");
  Serial.print("hasDyskinesia = ");
  Serial.println(result.hasDyskinesia ? "true" : "false");
  Serial.println();
}

void runDyskinesiaTest() {
  Serial.println("=== Dyskinesia Test: 6 Hz sine for 3 seconds ===");

  // 1. Generate 6 Hz sine wave
  generateSine(sampleBuffer, N_SAMPLES, FS, 6.0f, 1.0f);

  // 2. Run FFT analysis
  MovementAnalysis result = analyzeWindow(sampleBuffer, N_SAMPLES);

  // 3. Print results
  Serial.print("tremorPower = ");
  Serial.println(result.tremorPower, 6);
  Serial.print("dyskinesiaPower = ");
  Serial.println(result.dyskinesiaPower, 6);
  Serial.print("hasTremor = ");
  Serial.println(result.hasTremor ? "true" : "false");
  Serial.print("hasDyskinesia = ");
  Serial.println(result.hasDyskinesia ? "true" : "false");
  Serial.println();
}



void setup() {

  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting FFT self-test...");

  // set up FFT processor
  initFFT(N, FS);

  // Run synthetic tests
  runTremorTest();
  runDyskinesiaTest();

}

void loop() {
  // nothing for now; tests already ran in setup()
}

