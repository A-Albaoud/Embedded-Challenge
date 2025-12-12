// fft_processor.h
#pragma once
#include <cstddef>

struct MovementAnalysis {
    float tremorPower;
    float dyskinesiaPower;
    bool  hasTremor;
    bool  hasDyskinesia;
};

// Must be called once in setup() before analyzeWindow()
void initFFT(std::size_t nSamples, float fs);

// Analyze one window of samples (1D float signal)
MovementAnalysis analyzeWindow(float* samples, std::size_t nSamples);
