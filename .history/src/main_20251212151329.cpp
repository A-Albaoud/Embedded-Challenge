#include <Arduino.h>
#include "fft_processor.h"

// -------------------------------
// Test parameters
// -------------------------------
const size_t TEST_N = 128;     // must match initFFT()
const float  TEST_FS = 52.0f;  // sampling rate

// 3-D test buffers
static float ax[TEST_N];
static float ay[TEST_N];
static float az[TEST_N];

// -------------------------------------------------------
// Generate synthetic 3-D signal for testing FFT pipeline
// tremorFreq = 4 Hz  → inside 3–5 Hz (should detect tremor)
// dyskFreq   = 6 Hz  → inside 5–7 Hz (should detect dyskinesia)
// -------------------------------------------------------
void generateTestData(float tremorFreq, float dyskFreq) {

    for (size_t i = 0; i < TEST_N; i++) {

        float t = (float)i / TEST_FS;

        // 4 Hz tremor component
        float tremor = 0.5f * sinf(2.0f * PI * tremorFreq * t);

        // 6 Hz dyskinesia component (optional)
        float dysk = 0.0f;
        if (dyskFreq > 0.0f) {
            dysk = 0.3f * sinf(2.0f * PI * dyskFreq * t);
        }

        // noise
        float noise = 0.05f * ((float)random(-100, 100) / 100.0f);

        // assign to axes
        ax[i] = tremor + dysk + noise;
        ay[i] = tremor * 0.8f + noise * 1.2f;
        az[i] = tremor * 0.4f + dysk * 0.2f + noise;
    }
}

// -------------------------------------------------------
// Run FFT test using your analyzeWindow3D_Magnitude()
// -------------------------------------------------------
void runFFTTest(const char* label) {
    Serial.println(label);

    MovementAnalysis result = analyzeWindow3D_Magnitude(ax, ay, az, TEST_N);

    Serial.println("----- FFT RESULT -----");
    Serial.print("Tremor Power:     "); Serial.println(result.tremorPower, 6);
    Serial.print("Dyskinesia Power: "); Serial.println(result.dyskinesiaPower, 6);
    Serial.print("hasTremor?        "); Serial.println(result.hasTremor ? "YES" : "NO");
    Serial.print("hasDyskinesia?    "); Serial.println(result.hasDyskinesia ? "YES" : "NO");
    Serial.println("----------------------\n");
}

// -------------------------------------------------------
// Arduino Setup
// -------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Initializing FFT...");
    initFFT(TEST_N, TEST_FS);

    // ---------------------------
    // TEST 1: Pure tremor (4 Hz)
    // ---------------------------
    generateTestData(4.0f, 0.0f);
    runFFTTest("TEST 1: 4 Hz tremor only (expect tremor = YES, dyskinesia = NO)");

    // ---------------------------
    // TEST 2: Pure dyskinesia (6 Hz)
    // ---------------------------
    generateTestData(0.0f, 6.0f);
    runFFTTest("TEST 2: 6 Hz dyskinesia only (expect tremor = NO, dyskinesia = YES)");

    // ---------------------------
    // TEST 3: Both 4 Hz and 6 Hz
    // ---------------------------
    generateTestData(4.0f, 6.0f);
    runFFTTest("TEST 3: 4 Hz + 6 Hz (expect BOTH = YES)");
}

void loop() {
    // test only runs once in setup()
}
