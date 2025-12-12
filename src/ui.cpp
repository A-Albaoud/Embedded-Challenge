#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Wire.h>
#include <Adafruit_TSC2007.h>

#include "ui.h"

// -----------------
// Hardware objects
// -----------------

static const int TFT_DC = 10;
static const int TFT_CS = 9;

static Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
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
static ClockTime currentTime = { 6, 0, 0 };   // demo start time
static unsigned long lastTickMs = 0;
static ClockTime lastDrawnTime = { 255, 255, 255 }; // force first draw

// Symptom / analysis state
static SymptomState currentState = STATE_REST;
static AnalysisResult lastAnalysis = {0, 0, 0, 0};

// Waveform buffer for graph
static const int WAVE_SAMPLES = 64;
static float waveBuffer[WAVE_SAMPLES];
static int waveIndex = 0;

// Screen geometry
static const int SCREEN_W = 320;
static const int SCREEN_H = 240;

// Live screen geometry (shorter chart + bottom info panel)

// Left severity bar (thin)
static const int BAR_X = 10;
static const int BAR_Y = 20;
static const int BAR_W = 14;
static const int BAR_H = 200;

// Layout spacing
static const int GAP = 8;
static const int RIGHT_MARGIN = 10;
static const int BOTTOM_MARGIN = 10;

// Chart is full width to the right, but shorter height
static const int GRAPH_X = BAR_X + BAR_W + GAP;
static const int GRAPH_Y = 20;
static const int GRAPH_W = 320 - GRAPH_X - RIGHT_MARGIN;
static const int GRAPH_H = 120;   // <-- shorter chart height (tune: 100–140)

// Bottom info panel (free space under chart)
static const int INFO_X = GRAPH_X;
static const int INFO_Y = GRAPH_Y + GRAPH_H + GAP;
static const int INFO_W = GRAPH_W;
static const int INFO_H = 240 - INFO_Y - BOTTOM_MARGIN;

// Pink color (approx) in 565
#define COLOR_PINK 0xF81F

// -----------------
// Touch calibration
// -----------------
// These ranges are typical-ish for resistive touch controllers but WILL vary.
// The clamping logic makes it usable even if slightly off.
// If touch still feels “offset”, we can tighten these after you give 2 corner readings.
static const int16_t TS_MINX = 200;
static const int16_t TS_MAXX = 3800;
static const int16_t TS_MINY = 200;
static const int16_t TS_MAXY = 3800;

// Pressure threshold: 10 is often too sensitive/noisy
static const int16_t TOUCH_Z_THRESHOLD = 80;

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

// Short labels so they never wrap in the right panel
static const char* stateToString(SymptomState s) {
    switch (s) {
        case STATE_REST:       return "REST";
        case STATE_NORMAL:     return "NORM";
        case STATE_DYSKINESIA: return "DYSK";
        case STATE_TREMOR:     return "TREM";
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

    if (p.z < TOUCH_Z_THRESHOLD) return false;

    // 🔍 DEBUG PRINT — THIS IS THE IMPORTANT PART
    Serial.print("RAW x="); Serial.print(p.x);
    Serial.print(" y=");    Serial.print(p.y);
    Serial.print(" z=");    Serial.println(p.z);

    int16_t px = map(p.y, TS_MAXY, TS_MINY, 0, tft.width());
    int16_t py = map(p.x, TS_MINX, TS_MAXX, 0, tft.height());

    // Clamp
    if (px < 0) px = 0;
    if (py < 0) py = 0;
    if (px >= tft.width())  px = tft.width() - 1;
    if (py >= tft.height()) py = tft.height() - 1;

    x = px;
    y = py;
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

    // Status value (with background so it overwrites cleanly)
    tft.setTextColor(colorForState(currentState), ILI9341_BLACK);
    tft.setCursor(205, 35);
    tft.print(stateToString(currentState));
    tft.setTextColor(ILI9341_WHITE);

    // Big clock in center
    char buf[9];
    formatClock(buf, currentTime);

    tft.setTextSize(5);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(70, 100);
    tft.print(buf);
    tft.setTextColor(ILI9341_WHITE);
}

// Called when time changed; redraw just the clock text (no flicker)
static void redrawClockTimeOnly() {
    char buf[9];
    formatClock(buf, currentTime);

    tft.setTextSize(5);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(70, 100);
    tft.print(buf);

    tft.setTextColor(ILI9341_WHITE);
}

// -----------------
// Drawing: Live screen
// -----------------

static void drawSeverityBackground() {
    // Empty bar outline (no 4 colored bands)
    tft.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, ILI9341_WHITE);
}




static void drawLiveScreenFull() {
    tft.fillScreen(ILI9341_BLACK);
    tft.drawRect(0, 0, SCREEN_W, SCREEN_H, ILI9341_DARKGREY);

    // Left severity bar background
    drawSeverityBackground();

    // ---- Graph (shorter, demonstrates waveform) ----
    tft.drawRect(GRAPH_X, GRAPH_Y, GRAPH_W, GRAPH_H, ILI9341_WHITE);
    tft.fillRect(GRAPH_X + 1, GRAPH_Y + 1, GRAPH_W - 2, GRAPH_H - 2, ILI9341_BLACK);

    // ---- Bottom info panel (status + text) ----
    tft.drawRect(INFO_X, INFO_Y, INFO_W, INFO_H, ILI9341_WHITE);
    tft.fillRect(INFO_X + 1, INFO_Y + 1, INFO_W - 2, INFO_H - 2, ILI9341_BLACK);

    // Status line
    tft.setTextSize(2);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);

    tft.setCursor(INFO_X + 8, INFO_Y + 8);
    tft.print("Status:");

    tft.setTextColor(colorForState(currentState), ILI9341_BLACK);
    tft.setCursor(INFO_X + 95, INFO_Y + 8);
    tft.print(stateToString(currentState));

    // Frequency line
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(INFO_X + 8, INFO_Y + 34);
    tft.print("Freq:");

    tft.setCursor(INFO_X + 95, INFO_Y + 34);
    tft.print(lastAnalysis.dominantFreq, 1);
    tft.print(" Hz");

    // Restore default text mode
    tft.setTextColor(ILI9341_WHITE);
}


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

static void updateSeverityOverlay() {
    // Tune this to your expected dominant frequency range
    const float MAX_HZ = 5.7f;   // map 0..12 Hz -> 0..100% bar

    float target = lastAnalysis.dominantFreq / MAX_HZ;
    if (target < 0.0f) target = 0.0f;
    if (target > 1.0f) target = 1.0f;

    // Smooth the rise (prevents jitter)
    static float level = 0.0f;
    level = 0.70f * level + 0.30f * target; 

    int filledHeight = (int)(BAR_H * level);
    int bottom = BAR_Y + BAR_H;

    // Clear inside of bar (keep outline)
    tft.fillRect(BAR_X + 1, BAR_Y + 1, BAR_W - 2, BAR_H - 2, ILI9341_BLACK);

    // Fill from bottom up, using current classification color
    uint16_t c = colorForState(currentState);
    tft.fillRect(BAR_X + 1, bottom - filledHeight, BAR_W - 2, filledHeight, c);
}


// -----------------
// Classification
// -----------------

static SymptomState classify(const AnalysisResult &r) {
    const float REST_RMS = 0.05f;     // tune later with real data
    const float ACTIVE_RMS = 0.12f;   // below this but above REST -> NORMAL

    // If there’s barely any movement, call it REST
    if (r.overallRms < REST_RMS) {
        return STATE_REST;
    }

    float f = r.dominantFreq;

    // Band-based classification (per assignment spec)
    if (f >= 3.0f && f < 5.0f) {
        return STATE_TREMOR;
    }
    if (f >= 5.0f && f <= 7.0f) {
        return STATE_DYSKINESIA;
    }

    // Otherwise: movement exists but not in the defined rhythmic bands
    if (r.overallRms < ACTIVE_RMS) {
        return STATE_NORMAL;
    } else {
        // “Active but not in 3–7 Hz” — safest label is Normal unless your spec adds more states
        return STATE_NORMAL;
    }
}


// -----------------
// Public API
// -----------------

void uiBegin() {
    tft.begin();
    tft.setRotation(1);       // landscape
    tft.setTextWrap(false);   // CRITICAL: prevent weird wrapping like ":OP" or vertical "ng"

    Wire.begin();

    if (!touch.begin()) {
        Serial.println("TSC2007 not found!");
        while (1) { delay(10); }
    }

    for (int i = 0; i < WAVE_SAMPLES; ++i) {
        waveBuffer[i] = 0.0f;
    }

    currentScreen = SCREEN_CLOCK;
    drawClockScreenFull();
    lastDrawnTime = currentTime;
}

void uiUpdate() {
    tickClock();

    // Tap debounce (edge-trigger)
    static bool wasTouched = false;

    int16_t tx, ty;
    bool isTouched = getTouch(tx, ty);

    if (isTouched && !wasTouched) {
        if (currentScreen == SCREEN_CLOCK) {
            currentScreen = SCREEN_LIVE;
            drawLiveScreenFull();
        } else if (currentScreen == SCREEN_LIVE) {
            // Any tap -> back to clock screen
            currentScreen = SCREEN_CLOCK;
            drawClockScreenFull();
            lastDrawnTime = currentTime;
        }
    }

    wasTouched = isTouched;

    if (currentScreen == SCREEN_CLOCK) {
        // Only redraw when what we DISPLAY changes (HH:MM)
        if (currentTime.minutes != lastDrawnTime.minutes ||
            currentTime.hours   != lastDrawnTime.hours) {
            lastDrawnTime = currentTime;
            redrawClockTimeOnly();
        }
    } else if (currentScreen == SCREEN_LIVE) {
        // Throttle graph a bit so it doesn’t hog SPI
        static unsigned long lastGraphMs = 0;
        const unsigned long GRAPH_PERIOD_MS = 50; // ~20 Hz

        unsigned long now = millis();
        if (now - lastGraphMs >= GRAPH_PERIOD_MS) {
            lastGraphMs = now;
            redrawWaveform();
            updateSeverityOverlay();
        }
    }
}

void uiSetLatestAnalysis(const AnalysisResult &result) {
    lastAnalysis = result;
    currentState = classify(result);

    if (currentScreen == SCREEN_LIVE) {
        // Update status
        tft.setTextSize(2);
        tft.setTextColor(colorForState(currentState), ILI9341_BLACK);
        tft.setCursor(200, 35);
        tft.print(stateToString(currentState));

        // Update freq
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(200, 80);
        tft.print(lastAnalysis.dominantFreq, 1);
        tft.print(" Hz");

        tft.setTextColor(ILI9341_WHITE);
    }
}

void uiPushSample(float sample) {
    if (sample < -1.0f) sample = -1.0f;
    if (sample >  1.0f) sample =  1.0f;

    waveIndex = (waveIndex + 1) % WAVE_SAMPLES;
    waveBuffer[waveIndex] = sample;
}
