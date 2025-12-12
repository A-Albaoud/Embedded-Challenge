#include <Arduino.h>
#include "sensor.h"

// Sampling constants
#define SAMPLE_RATE     52
#define SAMPLE_INTERVAL (1000000 / SAMPLE_RATE)   // ~19230 microseconds
#define NUM_SAMPLES     (3 * SAMPLE_RATE)         // 156 samples

// Sliding window buffers (3 seconds of data)
float ax_buffer[NUM_SAMPLES];
float ay_buffer[NUM_SAMPLES];
float az_buffer[NUM_SAMPLES];

// Track timing
unsigned long lastSampleTime = 0;
unsigned long lastFFT = 0;

// Store latest sensor reading
float x, y, z;

// Shift window left and insert new sample
void pushSample(float x, float y, float z) {
    for (int i = 0; i < NUM_SAMPLES - 1; i++) {
        ax_buffer[i] = ax_buffer[i+1];
        ay_buffer[i] = ay_buffer[i+1];
        az_buffer[i] = az_buffer[i+1];
    }

    ax_buffer[NUM_SAMPLES - 1] = x;
    ay_buffer[NUM_SAMPLES - 1] = y;
    az_buffer[NUM_SAMPLES - 1] = z;
}

// Debug print (temporary)
void printWindow() {
    Serial.println("---- 3s Window ----");
    for (int i = 0; i < NUM_SAMPLES; i++) {
        Serial.print(ax_buffer[i]); Serial.print("\t");
        Serial.print(ay_buffer[i]); Serial.print("\t");
        Serial.println(az_buffer[i]);
    }
}

void setup() {
    Serial.begin(115200);
    sensor_init();

    // Initialize buffers to zeros
    for (int i = 0; i < NUM_SAMPLES; i++) {
        ax_buffer[i] = ay_buffer[i] = az_buffer[i] = 0.0f;
    }

    lastSampleTime = micros();
}

void loop() {
    unsigned long now = micros();

    // Sample at 52 Hz
    if (now - lastSampleTime >= SAMPLE_INTERVAL) {
        lastSampleTime = now;

        sensor_read(x, y, z);
        pushSample(x, y, z);
    }

    // Update FFT/classification every 300 ms (real-time update)
    if (millis() - lastFFT >= 300) {
        lastFFT = millis();

        // TODO: Call FFT processing (runFFT(ax_buffer))
        // TODO: tremor detection (detectTremor())
        // TODO: update TFT screen (updateScreen())

        // Debug: print sliding window
        // printWindow();
    }
}
