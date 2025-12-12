#include <Arduino.h>
#include "fft_processor.h"   // Your FFT header

// -----------------------------------------------------
// CONFIGURATION FOR SYNTHETIC TEST SIGNAL GENERATION
// -----------------------------------------------------
const size_t N = 256;         // FFT size
const float FS = 52.0f;       // Sampling rate (52 Hz)

// Arrays for synthetic 3D IMU data
float ax[N];
float ay[N];
float az[N];

// -----------------------------------------------------
// Create synthetic 3D motion with known tremor frequency
// -----------------------------------------------------
void generateTestData(float tremorFreq = 4.0f, float dyskFreq = 0.0f) {
    for (size_t i = 0; i < N; i++) {

        float t = (float)i / FS;

        // Simulated tremor (4 Hz default)
        float tremor = 0.5f * sinf(2.0f * PI * tremorFreq * t);

        // Optional dyskinesia (e.g., 6 Hz)
        float dysk = 0.0f;
        if (dyskFreq > 0.0f) {
            dysk = 0.3f * sinf(2.0f * PI * dyskFreq * t);
        }

        // Small noise
        float noise = 0.05f * ((float)random(-100, 100) / 100.0f);

        // Assign each axis differently
        ax[i] = tremor + dysk + noise;          // full signal
        ay[i] = (tremor * 0.8f) + noise * 1.2f; // scaled tremor
        az[i] = (tremor * 0.4f) + (dysk * 0.3f) + noise;
    }
}

// -----------------------------------------------------
// MAIN TEST OF YOUR analyzeWindow3D_Magnitude()
// -----------------------------------------------------
void runFFTTest() {
    Serial.println("\n--- Running 3D FFT Test ---");

    // Generate synthetic movement with 4 Hz tremor, 0 Hz dyskinesia
    generateTestData(4.0f, 0.0f);

    // Run your FFT-based movement detection
    MovementAnalysis result = analyzeWindow3D_Magnitude(ax, ay, az, N);

    // Print results
    Serial.println("FFT Analysis Results:");
    Serial.print("  Tremor Power:     "); Serial.println(result.tremorPower, 6);
    Serial.print("  Dyskinesia Power: "); Serial.println(result.dyskinesiaPower, 6);
    Serial.print("  hasTremor?        "); Serial.println(result.hasTremor ? "YES" : "NO");
    Serial.print("  hasDyskinesia?    "); Serial.println(result.hasDyskinesia ? "YES" : "NO");

    Serial.println("\n--- Test Complete ---");
}

// -----------------------------------------------------
// Arduino SETUP + LOOP
// -----------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Initializing FFT...");
    initFFT(N, FS);

    delay(500);
    runFFTTest();
}

void loop() {
    // Nothing here.
    // You can re-run tests (e.g., every few seconds) if you want:
    // delay(3000);
    // runFFTTest();
}
