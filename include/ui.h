#pragma once

#include <Arduino.h>

// ---- Symptom / analysis types ----

enum SymptomState {
    STATE_REST,
    STATE_NORMAL,
    STATE_DYSKINESIA,
    STATE_TREMOR
};

struct AnalysisResult {
    float dominantFreq;   // Hz of main peak
    float tremorPower;    // power in 3–5 Hz band
    float dyskPower;      // power in 5–7 Hz band
    float overallRms;     // overall motion level
};

// Simple clock time (24h)
struct ClockTime {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

// Call once from setup(), after TFT is created
void uiBegin();

// Call on every loop() iteration
void uiUpdate();

// analysis module can call this
// whenever a new 3-sec FFT window is ready.
void uiSetLatestAnalysis(const AnalysisResult &result);

// or drawing the live waveform.
// sensor code can call this occasionally
// with the most recent "overall motion" sample.
void uiPushSample(float sample);

static void redrawClockStatusOnly();