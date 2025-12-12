#include <Arduino.h>
#include "fft_processor.h"
#include "ui.h"

// ------ GLOBAL VARIABLES -------------------------
const float FS = 52.0f;    // sampling frequency
const size_t N  = 256;     // chosen FFT size (upper bound power of 2)
const unsigned long SAMPLE_PERIOD_MS = 1000.0 / FS;  // ≈ 19.23 ms



void setup() {
    Serial.begin(115200);
    delay(200);

    uiBegin();
  

}

void loop() {
  uiUpdate();
  

}

