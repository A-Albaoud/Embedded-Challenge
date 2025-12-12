#include <Arduino.h>
#include <Wire.h>
#include "sensor.h"
#include "fft_processing.h"

// ---------------- Configuration ----------------
constexpr float SAMPLE_RATE = 52.0f;
constexpr float WINDOW_SEC  = 3.0f;
constexpr size_t NUM_SAMPLES = (size_t)(SAMPLE_RATE * WINDOW_SEC);

// ---------------- Buffers ----------------
float ax_buf[NUM_SAMPLES];
float ay_buf[NUM_SAMPLES];
float az_buf[NUM_SAMPLES];
volatile size_t writeIndex = 0;

// ---------------- Timing ----------------
unsigned long lastSampleMicros = 0;
unsigned long lastFFTMillis   = 0;
constexpr unsigned long SAMPLE_INTERVAL_US = 1000000 / SAMPLE_RATE;


float mag_buf[NUM_SAMPLES];  // ONLY ONE BUFFER


// ---------------- Helpers ----------------
void pushSample(float x, float y, float z) {
    ax_buf[writeIndex] = x;
    ay_buf[writeIndex] = y;
    az_buf[writeIndex] = z;
    writeIndex = (writeIndex + 1) % NUM_SAMPLES;
}

void scanI2C() {
    Serial.println("Scanning I2C bus...");
    Wire.begin();
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.print("Found I2C device at 0x");
            Serial.println(addr, HEX);
        }
    }
}


// ---------------- Arduino ----------------
void setup() {
    Serial.begin(115200);
    while (!Serial);
    Serial.println("Serial ready");

    sensor_init();
    initFFT(NUM_SAMPLES, SAMPLE_RATE);

    // Clear buffers
    for (size_t i = 0; i < NUM_SAMPLES; i++) {
        ax_buf[i] = ay_buf[i] = az_buf[i] = 0.0f;
    }

    lastSampleMicros = micros();
}


void loop() {
    unsigned long nowMicros = micros();

    // ----------- Sample sensor -----------
    if (nowMicros - lastSampleMicros >= SAMPLE_INTERVAL_US) {
        lastSampleMicros = nowMicros;

        float x, y, z;
        sensor_read(x, y, z);

        float mag = x*x + y*y + z*z;   // squared magnitude
        mag_buf[writeIndex] = mag;
        writeIndex = (writeIndex + 1) % NUM_SAMPLES;

        pushSample(x, y, z);
    }

    // ----------- Run FFT every 300 ms -----------
    if (millis() - lastFFTMillis >= 300) {
        lastFFTMillis = millis();

        MovementAnalysis result = analyzeWindow3D_Magnitude(ax_buf, ay_buf, az_buf, NUM_SAMPLES);

        Serial.print("Tremor: ");
        Serial.print(result.hasTremor ? "YES" : "NO");
        Serial.print(" | Dyskinesia: ");
        Serial.println(result.hasDyskinesia ? "YES" : "NO");
    }
}
