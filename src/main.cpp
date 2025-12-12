#include <Arduino.h> 
#include <Wire.h> 
#include <Adafruit_ADXL345_U.h> 
#include "sensor.h" // Sampling constants 

#define SAMPLE_RATE 52 
#define SAMPLE_INTERVAL (1000000 / SAMPLE_RATE) // ~19230 microseconds 
#define NUM_SAMPLES (3 * SAMPLE_RATE) // 156 samples for 3 seconds 
#define SENSOR_I2C_ADDR 0x42 //set slave 
// Sliding window buffers (3 seconds of data) 
float ax_buffer[NUM_SAMPLES]; 
float ay_buffer[NUM_SAMPLES]; 
float az_buffer[NUM_SAMPLES]; // Track timing 
unsigned long lastSampleTime = 0; 
// unsigned long lastFFT = 0; 
// Store latest sensor reading 
float x, y, z; 
volatile int writeIndex = 0; 
// // Debug print (temporary) 
// void printWindow() { 
// Serial.println("---- 3s Window ----"); 
// for (int i = 0; i < NUM_SAMPLES; i++) { 
// Serial.print(ax_buffer[i]); Serial.print("\t"); 
// Serial.print(ay_buffer[i]); Serial.print("\t"); 
// Serial.println(az_buffer[i]); 
// } 
// } 
void setup() { 
    Serial.begin(115200); 
    while (!Serial);
    Wire.begin(); // // Initialize as I2C SLAVE 
    // Wire.begin(SENSOR_I2C_ADDR); 
    // Wire.onRequest(onRequestHandler); 
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
        lastSampleTime = now; sensor_read(x, y, z); 
        ax_buffer[writeIndex] = x; 
        ay_buffer[writeIndex] = y; 
        az_buffer[writeIndex] = z; 
        
        writeIndex = (writeIndex + 1) % NUM_SAMPLES; 
        Serial.print("X = "); Serial.print(x); 
        Serial.print("Y = "); Serial.print(y); 
        Serial.print("Z = "); Serial.println(z); 
        // delay(100); } 
        // // Update FFT/classification every 300 ms (real-time update) // if (millis() - lastFFT >= 300) { // lastFFT = millis(); // // TODO: Call FFT processing (runFFT(ax_buffer)) // // TODO: tremor detection (detectTremor()) // // TODO: update TFT screen (updateScreen()) // // Debug: print sliding window // // printWindow(); // 
        } 
    }