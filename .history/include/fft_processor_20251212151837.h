// fft_processor.h
#include <Arduino.h>

// Structure to hold analysis results
struct MovementAnalysis {
    bool hasTremor;
    bool hasDyskinesia;
    float tremorPower;       // energy in 3–5 Hz band
    float dyskinesiaPower;   // energy in 5–7 Hz band
};

void initFFT(size_t nSamples, float fs);

// samples: time-domain signal (length nSamples)
// returns: tremor/dyskinesia detection + intensity
MovementAnalysis analyzeWindow3D_Magnitude(const float* ax,
                                           const float* ay,
                                           const float* az,
                                           size_t nSamples);