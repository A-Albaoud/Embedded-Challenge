#include "fft_processor.h"
#include <arduinoFFT.h>

// Static Variables
static size_t N;         // Length of of data arrays
static float FS;         // Sampling frequency
static ArduinoFFT<double> FFT;

// Signal array pointers
static double* vReal;
static double* vImag;

// Threshold Indicators
static const float TREMOR_THRESH = 0.05f;       
static const float DYSK_THRESH = 0.05f;   

void initFFT(size_t nSamples, float fs) {
    N = nSamples;
    FS = fs;

    // Allocate memory for signal arrays
    vReal = new double[N];
    vImag = new double[N];
}

MovementAnalysis analyzeWindow3D_Magnitude(const float* ax,
                                           const float* ay,
                                           const float* az,
                                           size_t nSamples) {
    // Write magnitude directly into vReal; reuse vImag as usual
    for (size_t i = 0; i < N; i++) {
        if (i < nSamples) {
            float x = ax[i];
            float y = ay[i];
            float z = az[i];
            vReal[i] = x*x + y*y + z*z;   // squared magnitude
        } else {
            vReal[i] = 0.0f;
        }
        vImag[i] = 0.0f;
    }

    // then run your usual FFT pipeline…
}


MovementAnalysis analyzeWindow(float* samples, size_t nSamples) {

    // Copy samples to data arrays
    for (size_t i = 0; i < N; i++) {
        if (i < nSamples) {
            vReal[i] = samples[i]; // copy over available samples
        } else {
            vReal[i] = 0.0f;       // zero-pad if fewer samples
        }
        vImag[i] = 0.0f; 
    }

    // Perform FFT
    FFT.windowing(vReal, N, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // smooth abrupt edge cutoffs
    FFT.compute(vReal, vImag, N, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, N); // vReal now contains magnitudes

    // Frequency resolution
    float freqResolution = FS / (float)N;

    // Calculate power in tremor (3-5 Hz) and dyskinesia (5-7 Hz) bands
    float tremorPower = 0.0f;
    float dyskinesiaPower = 0.0f;
    float totalPower = 0.0f;

    for (size_t i = 1; i < N / 2; i++) { // ignore DC component at index 0
        float freq = i * freqResolution;
        float power = vReal[i] * vReal[i]; // find power (magnitude squared)

        totalPower += power;

        if (freq >= 3.0f && freq < 5.0f) {
            tremorPower += power;
        } else if (freq >= 5.0f && freq < 7.0f) {
            dyskinesiaPower += power;
        }
    }

    MovementAnalysis analysis;

    analysis.tremorPower     = tremorPower;
    analysis.dyskinesiaPower = dyskinesiaPower;
    analysis.hasTremor       = tremorPower     > (TREMOR_THRESH * totalPower);       
    analysis.hasDyskinesia   = dyskinesiaPower > (DYSK_THRESH   * totalPower);

    return analysis;
}
