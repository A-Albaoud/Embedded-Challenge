#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct {
    float tremorPower;       // 3–5 Hz
    float dyskinesiaPower;   // 5–7 Hz

    float peakFreqHz;        // peak in 3–7 Hz if rhythmic, else 0
    bool  rhythmic37;        // gated inside FFT
} MovementAnalysis;

void initFFT(size_t nSamples, float fs);

MovementAnalysis analyzeWindow3D_Magnitude_Q10(const int16_t* axQ10,
                                              const int16_t* ayQ10,
                                              const int16_t* azQ10,
                                              size_t nSamples,
                                              int16_t scaleQ10);
