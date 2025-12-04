#include "fft_processor.h"
#include <arduinoFFT.h>

// Capable of capturing 3 second intervals at 52Hz
// i.e., 156 samples

static arduinoFFT FFT;   
static size_t N;         // Length of of data arrays
static float FS;         // Sampling frequency

// Signal array pointers
static float* vReal;     
static float* vImag;

// Threshold Indicators
static const float TREMOR_THRESH = 0.05f;       
static const float DYSK_THRESH = 0.05f;   

void initFFT(size_t nSamples, float fs) {
    N = nSamples;
    FS = fs;

    // Allocate memory for signal arrays
    vReal = new float[N];
    vImag = new float[N];

    FFT.Windowing(vReal, N, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // Apply windowing
    FFT.SetResolution(10); // Set FFT resolution
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
    FFT.Windowing(vReal, N, FFT_WIN_TYP_HAMMING, FFT_FORWARD); // smooth abrupt edge cutoffs
    FFT.Compute(vReal, vImag, N, FFT_FORWARD);
    FFT.ComplexToMagnitude(vReal, vImag, N); // vReal now contains magnitudes

    // Frequency resolution
    float freqResolution = FS / N;

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


    // Analysis Results to return
    MovementAnalysis analysis;

    analysis.tremorPower = tremorPower;
    analysis.dyskinesiaPower = dyskinesiaPower;
    analysis.hasTremor = (tremorPower > TREMOR_THRESH * totalPower);       
    analysis.hasDyskinesia = (dyskinesiaPower > DYSK_THRESH * totalPower);

    return analysis;
}