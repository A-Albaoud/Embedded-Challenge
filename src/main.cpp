#include <Arduino.h>
#include <math.h>
#include "fft_processing.h"

// ------------------------------------------------------------
// Configuration
// ------------------------------------------------------------
constexpr float SAMPLE_TIME_SEC = 3.0f;   // 3-second window
constexpr size_t FFT_SIZE = 64;            // FFT size (must match initFFT)
constexpr size_t MAX_SAMPLES = 160;        // >= 3s * 52Hz = 156

// ------------------------------------------------------------
// Buffers
// ------------------------------------------------------------
float sampleBuffer[MAX_SAMPLES];

// ------------------------------------------------------------
// Derived values (set in setup)
// ------------------------------------------------------------
size_t N_SAMPLES = 0;
unsigned long SAMPLE_PERIOD_MS = 0;
float FS = 0.0f;   // sampling frequency pulled from FFT module

// ------------------------------------------------------------
// Test signal generator
// ------------------------------------------------------------
void generateSine(float* buf, size_t nSamples, float fs,
                  float freqHz, float amplitude) {
    for (size_t i = 0; i < nSamples; i++) {
        float t = (float)i / fs;
        buf[i] = amplitude * sinf(2.0f * PI * freqHz * t);
    }
}

// ------------------------------------------------------------
// Tests
// ------------------------------------------------------------
void runTremorTest() {
    Serial.println("=== Tremor Test: 4 Hz sine for 3 seconds ===");

    generateSine(sampleBuffer, N_SAMPLES, FS, 4.0f, 1.0f);
    MovementAnalysis result = analyzeWindow(sampleBuffer, N_SAMPLES);

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

    generateSine(sampleBuffer, N_SAMPLES, FS, 6.0f, 1.0f);
    MovementAnalysis result = analyzeWindow(sampleBuffer, N_SAMPLES);

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

// ------------------------------------------------------------
// Arduino entry points
// ------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    while (!Serial);

    Serial.println("Starting FFT self-test...");

    // Initialize FFT module
    initFFT(FFT_SIZE, 52.0f);

    // Pull config from FFT module
    FS = fftSamplingFreq();
    size_t N = fftSize();

    // Derived timing
    SAMPLE_PERIOD_MS = (unsigned long)(1000.0f / FS);
    N_SAMPLES = (size_t)(FS * SAMPLE_TIME_SEC);

    Serial.print("FFT size = ");
    Serial.println(N);
    Serial.print("Sampling freq = ");
    Serial.println(FS);
    Serial.print("Samples per window = ");
    Serial.println(N_SAMPLES);

    runTremorTest();
    runDyskinesiaTest();
}

void loop() {
    // Tests already ran in setup
}



// #include <Arduino.h>
// #include "fft_processing.h"

// // -------------------------------
// // Test parameters
// // -------------------------------
// const size_t TEST_N = 128;     // must match initFFT()
// const float  TEST_FS = 52.0f;  // sampling rate

// // 3-D test buffers
// static float ax[TEST_N];
// static float ay[TEST_N];
// static float az[TEST_N];

// // -------------------------------------------------------
// // Generate synthetic 3-D signal for testing FFT pipeline
// // tremorFreq = 4 Hz  → inside 3–5 Hz (should detect tremor)
// // dyskFreq   = 6 Hz  → inside 5–7 Hz (should detect dyskinesia)
// // -------------------------------------------------------
// void generateTestData(float tremorFreq, float dyskFreq) {

//     for (size_t i = 0; i < TEST_N; i++) {

//         float t = (float)i / TEST_FS;

//         // 4 Hz tremor component
//         float tremor = 0.5f * sinf(2.0f * PI * tremorFreq * t);

//         // 6 Hz dyskinesia component (optional)
//         float dysk = 0.0f;
//         if (dyskFreq > 0.0f) {
//             dysk = 0.3f * sinf(2.0f * PI * dyskFreq * t);
//         }

//         // noise
//         float noise = 0.05f * ((float)random(-100, 100) / 100.0f);

//         // assign to axes
//         ax[i] = tremor + dysk + noise;
//         ay[i] = tremor * 0.8f + noise * 1.2f;
//         az[i] = tremor * 0.4f + dysk * 0.2f + noise;
//     }
// }

// // -------------------------------------------------------
// // Run FFT test using your analyzeWindow3D_Magnitude()
// // -------------------------------------------------------
// void runFFTTest(const char* label) {
//     Serial.println(label);

//     MovementAnalysis result = analyzeWindow3D_Magnitude(ax, ay, az, TEST_N);

//     Serial.println("----- FFT RESULT -----");
//     Serial.print("Tremor Power:     "); Serial.println(result.tremorPower, 6);
//     Serial.print("Dyskinesia Power: "); Serial.println(result.dyskinesiaPower, 6);
//     Serial.print("hasTremor?        "); Serial.println(result.hasTremor ? "YES" : "NO");
//     Serial.print("hasDyskinesia?    "); Serial.println(result.hasDyskinesia ? "YES" : "NO");
//     Serial.println("----------------------\n");
// }

// // -------------------------------------------------------
// // Arduino Setup
// // -------------------------------------------------------
// void setup() {
//     Serial.begin(115200);
//     delay(1000);

//     Serial.println("Initializing FFT...");
//     initFFT(TEST_N, TEST_FS);

//     // ---------------------------
//     // TEST 1: Pure tremor (4 Hz)
//     // ---------------------------
//     generateTestData(4.0f, 0.0f);
//     runFFTTest("TEST 1: 4 Hz tremor only (expect tremor = YES, dyskinesia = NO)");

//     // ---------------------------
//     // TEST 2: Pure dyskinesia (6 Hz)
//     // ---------------------------
//     generateTestData(0.0f, 6.0f);
//     runFFTTest("TEST 2: 6 Hz dyskinesia only (expect tremor = NO, dyskinesia = YES)");

//     // ---------------------------
//     // TEST 3: Both 4 Hz and 6 Hz
//     // ---------------------------
//     generateTestData(4.0f, 6.0f);
//     runFFTTest("TEST 3: 4 Hz + 6 Hz (expect BOTH = YES)");
// }

// void loop() {
//     // test only runs once in setup()
// }
