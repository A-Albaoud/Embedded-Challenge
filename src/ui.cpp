#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>

#include "ui.h"

// -----------------
// Hardware objects
// -----------------


static const int TFT_DC  = 10;
static const int TFT_CS  = 9;
static Adafruit_ILI9341 tft(TFT_CS, TFT_DC);

// Touch controller (TSC2007 on FeatherWing V2)
static Adafruit_TSC2007 touch;
// -----------------
// UI state
// -----------------

enum UiScreen {
    SCREEN_CLOCK,
    SCREEN_LIVE
};

static UiScreen currentScreen = SCREEN_CLOCK;

// Clock state
static ClockTime currentTime = { 6, 0, 0 };   // start at 06:00:00
static unsigned long lastTickMs = 0;
static ClockTime lastDrawnTime = { 255, 255, 255 }; // force first draw

// Symptom / analysis state
static SymptomState currentState = STATE_REST;
static AnalysisResult lastAnalysis = {0,0,0,0};

// Waveform buffer for graph
static const int WAVE_SAMPLES = 64;
static float waveBuffer[WAVE_SAMPLES];
static int waveIndex = 0;

// Screen geometry
static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

// Live screen geometry
static const int GRAPH_X = 60;
static const int GRAPH_Y = 20;
static const int GRAPH_W = 200;
static const int GRAPH_H = 200;

static const int BAR_X = 10;
static const int BAR_Y = 20;
static const int BAR_W = 40;
static const int BAR_H = 200;

static const int STOP_X = 220;
static const int STOP_Y = 190;
static const int STOP_W = 90;
static const int STOP_H = 40;

// Pink color (approx) in 565
#define COLOR_PINK 0xF81F  // magenta-ish

// -----------------
// Utility helpers
// -----------------

static uint16_t colorForState(SymptomState s) {
    switch (s) {
        case STATE_REST:       return ILI9341_DARKGREY;
        case STATE_NORMAL:     return ILI9341_GREEN;
        case STATE_DYSKINESIA: return COLOR_PINK;
        case STATE_TREMOR:     return ILI9341_RED;
        default:               return ILI9341_WHITE;
    }
}

static const char* stateToString(SymptomState s) {
    switch (s) {
        case STATE_REST:       return "Resting";
        case STATE_NORMAL:     return "Normal";
        case STATE_DYSKINESIA: return "Dyskinesia";
        case STATE_TREMOR:     return "Tremor";
        default:               return "?";
    }
}

static void formatClock(char* buf, const ClockTime &t) {
    // "HH:MM"
    sprintf(buf, "%02d:%02d", t.hours, t.minutes);
}


static void tickClock() {
    unsigned long now = millis();
    if (now - lastTickMs >= 1000) {
        lastTickMs += 1000;

        currentTime.seconds++;
        if (currentTime.seconds >= 60) {
            currentTime.seconds = 0;
            currentTime.minutes++;
            if (currentTime.minutes >= 60) {
                currentTime.minutes = 0;
                currentTime.hours++;
                if (currentTime.hours >= 24) {
                    currentTime.hours = 0;
                }
            }
        }
    }
}

// -----------------
// Touch handling
// -----------------


static bool getTouch(int16_t &x, int16_t &y) {
    TS_Point p = touch.getPoint();

    // No touch -> pressure is near zero
    if (p.z < 10) return false;

    // Map raw values to screen coordinates
    x = map(p.y, 3800, 200, 0, 320);
    y = map(p.x, 200, 3800, 0, 240);

    return true;
}



// -----------------
// Drawing: Clock screen
// -----------------

static void drawClockScreenFull() {
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(0, 0, SCREEN_W, SCREEN_H, ILI9341_DARKGREY);

    // Status box at top-right
    tft.drawRect(200, 10, 110, 50, ILI9341_WHITE);

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(205, 15);
    tft.print("Status");

    tft.setCursor(205, 35);
    tft.setTextColor(colorForState(currentState));
    tft.print(stateToString(currentState));

    // Big clock in center
    char buf[9];
    formatClock(buf, currentTime);

    tft.setTextSize(5);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(70, 100);
    tft.print(buf);
}

// Called when only time changed; redraw just the clock text
static void redrawClockTimeOnly() {
    char buf[9];
    formatClock(buf, currentTime);

    // Clear previous time area (rough bounding box)
    tft.fillRect(60, 90, 200, 60, ILI9341_BLACK);

    tft.setTextSize(5);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(70, 100);
    tft.print(buf);
}

// -----------------
// Drawing: Live screen
// -----------------

static void drawSeverityBackground() {
    int band = BAR_H / 4;

    // From bottom up: Rest (gray), Normal (green), Dyskinesia (pink), Tremor (red)
    tft.fillRect(BAR_X, BAR_Y + 3*band, BAR_W, band, ILI9341_DARKGREY);
    tft.fillRect(BAR_X, BAR_Y + 2*band, BAR_W, band, ILI9341_GREEN);
    tft.fillRect(BAR_X, BAR_Y + 1*band, BAR_W, band, COLOR_PINK);
    tft.fillRect(BAR_X, BAR_Y + 0*band, BAR_W, band, ILI9341_RED);
}

static void drawStopButton(bool pressed = false) {
    uint16_t border = ILI9341_WHITE;
    uint16_t fill   = pressed ? ILI9341_RED : ILI9341_BLACK;

    tft.drawRoundRect(STOP_X, STOP_Y, STOP_W, STOP_H, 5, border);
    tft.fillRoundRect(STOP_X + 2, STOP_Y + 2, STOP_W - 4, STOP_H - 4, 5, fill);

    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(STOP_X + 20, STOP_Y + 12);
    tft.print("Stop");
}

static void drawLiveScreenFull() {
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(0, 0, SCREEN_W, SCREEN_H, ILI9341_DARKGREY);

    // Left severity bar background
    drawSeverityBackground();

    // Graph box
    tft.drawRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, ILI9341_WHITE);

    // Status panel on right
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(200, 10);
    tft.print("Status:");

    tft.setCursor(200, 35);
    tft.setTextColor(colorForState(currentState));
    tft.print(stateToString(currentState));

    // Dominant freq text
    tft.setCursor(200, 60);
    tft.setTextColor(ILI9341_WHITE);
    tft.print("Freq:");

    tft.setCursor(200, 80);
    tft.print(lastAnalysis.dominantFreq, 1);
    tft.print(" Hz");

    // Stop button
    drawStopButton();

    // Clear waveform area
    tft.fillRect(GRAPH_X + 1, GRAPH_Y + 1, GRAPH_W - 2, GRAPH_H - 2, ILI9341_BLACK);
}

// Draw waveform from waveBuffer[]
static void redrawWaveform() {
    // Clear inside of graph
    tft.fillRect(GRAPH_X + 1, GRAPH_Y + 1, GRAPH_W - 2, GRAPH_H - 2, ILI9341_BLACK);

    float minVal = -1.0f;
    float maxVal =  1.0f;

    int prevX = 0, prevY = 0;
    for (int i = 0; i < WAVE_SAMPLES; ++i) {
        int idx = (waveIndex + i) % WAVE_SAMPLES;
        float v = waveBuffer[idx];

        int x = GRAPH_X + (i * (GRAPH_W - 2)) / (WAVE_SAMPLES - 1) + 1;
        int y = GRAPH_Y + GRAPH_H / 2
                - (int)((v - (minVal + maxVal) / 2.0f) * (GRAPH_H / (maxVal - minVal)));

        if (i > 0) {
            tft.drawLine(prevX, prevY, x, y, ILI9341_CYAN);
        }
        prevX = x;
        prevY = y;
    }
}

// Update severity overlay based on currentState + lastAnalysis
static void updateSeverityOverlay() {
    float bandMax = max(lastAnalysis.tremorPower, lastAnalysis.dyskPower);
    const float MAX_EXPECTED_POWER = 1.0f; // tweak later
    float intensity = bandMax / MAX_EXPECTED_POWER;
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;

    int filledHeight = (int)(BAR_H * intensity);
    int bottom = BAR_Y + BAR_H;

    // Redraw background then overlay
    drawSeverityBackground();

    uint16_t c = colorForState(currentState);
    tft.fillRect(BAR_X, bottom - filledHeight, BAR_W, filledHeight, c);
}

// -----------------
// Classification
// -----------------

static SymptomState classify(const AnalysisResult &r) {
    const float POWER_THRESH = 0.2f;
    const float REST_THRESH  = 0.05f;

    float bandMax = max(r.tremorPower, r.dyskPower);

    if (r.overallRms < REST_THRESH && bandMax < POWER_THRESH) {
        return STATE_REST;
    }
    if (bandMax < POWER_THRESH) {
        return STATE_NORMAL;
    }
    if (r.tremorPower > r.dyskPower) {
        return STATE_TREMOR;
    } else {
        return STATE_DYSKINESIA;
    }
}



void uiBegin() {
    tft.begin();
    tft.setRotation(1);  // landscape: 320x240

    Wire.begin();

    if (!touch.begin()) {
        Serial.println("TSC2007 not found!");
        while (1) {
            delay(10);  // stop if touch is missing
        }
    }

    // Clear waveform buffer
    for (int i = 0; i < WAVE_SAMPLES; ++i) {
        waveBuffer[i] = 0.0f;
    }

    currentScreen = SCREEN_CLOCK;
    drawClockScreenFull();
    lastDrawnTime = currentTime;


}

void uiUpdate() {
    tickClock();

    int16_t tx, ty;
    if (getTouch(tx, ty)) {
        if (currentScreen == SCREEN_CLOCK) {
            // Any touch -> go to live screen
            currentScreen = SCREEN_LIVE;
            drawLiveScreenFull();
        } else if (currentScreen == SCREEN_LIVE) {
            // Check Stop button
            if (tx >= STOP_X && tx <= STOP_X + STOP_W &&
                ty >= STOP_Y && ty <= STOP_Y + STOP_H) {

                drawStopButton(true);
                delay(100);
                drawStopButton(false);

                // Back to clock screen
                currentScreen = SCREEN_CLOCK;
                drawClockScreenFull();
                lastDrawnTime = currentTime;
            }
        }
    }

    if (currentScreen == SCREEN_CLOCK) {
        // Only redraw when seconds changed
        if (currentTime.seconds != lastDrawnTime.seconds) {
            lastDrawnTime = currentTime;
            redrawClockTimeOnly();
        }
    } else if (currentScreen == SCREEN_LIVE) {
        // For now, just redraw waveform + severity every frame.
        redrawWaveform();
        updateSeverityOverlay();
    }
}

void uiSetLatestAnalysis(const AnalysisResult &result) {
    lastAnalysis = result;
    currentState = classify(result);

    if (currentScreen == SCREEN_LIVE) {
        // Update text in Status panel
        tft.setTextSize(2);

        // Clear & rewrite classification line
        tft.fillRect(200, 35, 110, 16, ILI9341_BLACK);
        tft.setCursor(200, 35);
        tft.setTextColor(colorForState(currentState));
        tft.print(stateToString(currentState));

        // Clear & rewrite frequency line
        tft.fillRect(200, 80, 110, 16, ILI9341_BLACK);
        tft.setCursor(200, 80);
        tft.setTextColor(ILI9341_WHITE);
        tft.print(lastAnalysis.dominantFreq, 1);
        tft.print(" Hz");
    }
}

void uiPushSample(float sample) {
    // Clamp sample to [-1, 1] so we don't go crazy on the graph
    if (sample < -1.0f) sample = -1.0f;
    if (sample >  1.0f) sample =  1.0f;

    waveIndex = (waveIndex + 1) % WAVE_SAMPLES;
    waveBuffer[waveIndex] = sample;
}
