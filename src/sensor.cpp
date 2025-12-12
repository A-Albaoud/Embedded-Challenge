#include <Arduino.h> 
#include <Wire.h> 
#include <Adafruit_ADXL345_U.h> 
#include "sensor.h" 

// Create an ADXL345 object 
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345);
//set ID for sensor 
//I2C communication 
void sensor_init() {
    // Serial.begin(115200); //Baud rate 
    delay(500); 
    Serial.println("Initializing ADXL345..."); 
    // if (!accel.begin()) {
    //     //check if sensor is connected 
    //     Serial.println("ADXL345 not detected. Check wiring!"); 
    //     while (1); 
    // } 
    int tries = 0;
    while (!accel.begin() && tries < 5) {
        Serial.println("Retrying ADXL345...");
        delay(500);
        tries++;
    }
    if (tries == 5) {
        Serial.println("Failed to initialize sensor.");
    }
    accel.setRange(ADXL345_RANGE_2_G);
    //+-2g range 
    Serial.println("ADXL345 ready."); 
} 

// void sensor_read(float &x, float &y, float &z) { 
// sensors_event_t event; // accel.getEvent(&event);
//detect data from the sensor 
// Serial.print("X: "); Serial.print(event.acceleration.x);//print axis data 
// Serial.print("Y: "); Serial.print(event.acceleration.y); 
// Serial.print("Z: "); Serial.println(event.acceleration.z); 
 // delay(19); 
 // ~52 Hz sampling (1000ms / 52 ≈ 19.2ms) 
 // } 
 
 void sensor_read(float &x, float &y, float &z) { 
    sensors_event_t event; 
    accel.getEvent(&event); 

    x = event.acceleration.x; 
    y = event.acceleration.y; 
    z = event.acceleration.z; 
}