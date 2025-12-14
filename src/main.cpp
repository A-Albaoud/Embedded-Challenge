#include <Arduino.h>
#include <Wire.h>
#include <stdint.h>

#include "ui.h"
#include "sensor.h"
#include "fft_processing.h"

// ----------------------------
// Window timing/spec
// ----------------------------
static const uint32_t RAW_FS_HZ = 52;
static const uint16_t RAW_SECS  = 3;
static const uint16_t RAW_N     = (uint16_t)(RAW_FS_HZ * RAW_SECS); // 156 raw samples

static const size_t FFT_N     = 64;
static const float  FFT_FS_HZ = 64.0f / 3.0f; // 21.333...

// Q10 scaling
static const float   ACC_SCALE_F = 1024.0f;
static const int16_t ACC_SCALE_I = 1024;

// FFT buffers (Q10)
static int16_t axBuf[FFT_N];
static int16_t ayBuf[FFT_N];
static int16_t azBuf[FFT_N];

// Sampling scheduler
static const unsigned long RAW_SAMPLE_PERIOD_US = (1000000UL / RAW_FS_HZ);
static unsigned long nextSampleUs = 0;

// Analysis scheduler
static const unsigned long ANALYSIS_PERIOD_MS = 3000;
static unsigned long nextAnalysisMs = 0;

// Downsample state (156 -> 64)
static uint8_t outIdx = 0;
static int32_t err    = 0;

// Count real raw samples for this 3-second window
static uint16_t rawCount = 0;

// Last sample for display
static float gravZ = 9.80665f;

static inline void resetWindow() {
  outIdx = 0;
  err = 0;
  rawCount = 0;
}

// ------------------------------------------------------------
// 3D mean-square (mean removed) in integer math, Q20 units
// ------------------------------------------------------------
static uint32_t meanSquare3D_Q20_meanRemoved(const int16_t* xq,
                                             const int16_t* yq,
                                             const int16_t* zq,
                                             size_t n)
{
  if (n == 0) return 0;

  int32_t sx=0, sy=0, sz=0;
  for (size_t i=0;i<n;++i){ sx += xq[i]; sy += yq[i]; sz += zq[i]; }
  int32_t mx = sx / (int32_t)n;
  int32_t my = sy / (int32_t)n;
  int32_t mz = sz / (int32_t)n;

  uint64_t sumSq = 0;
  for (size_t i=0;i<n;++i){
    int32_t dx = (int32_t)xq[i] - mx; // Q10
    int32_t dy = (int32_t)yq[i] - my;
    int32_t dz = (int32_t)zq[i] - mz;
    sumSq += (uint64_t)((int64_t)dx*dx + (int64_t)dy*dy + (int64_t)dz*dz); // Q20
  }
  return (uint32_t)(sumSq / (uint64_t)n);
}

// ------------------------------------------------------------
// Build UI result (decide once per full 3s window)
// - Pattern-first: only classify when ma.rhythmic37
// - FAST: accept after 1 window (3 seconds)
// - Band-power based T vs D (more reliable than peak bin alone)
// ------------------------------------------------------------
static inline AnalysisResult buildStableUiResult(const MovementAnalysis& ma, bool isRest) {
  float candClassHz = 0.0f;

  if (!isRest && ma.rhythmic37) {
    const float MARGIN = 1.15f;
    if (ma.dyskinesiaPower > ma.tremorPower * MARGIN) {
      candClassHz = 6.0f;
    } else if (ma.tremorPower > ma.dyskinesiaPower * MARGIN) {
      candClassHz = 4.0f;
    } else {
      float f = ma.peakFreqHz;
      if (f >= 3.0f && f < 5.0f)       candClassHz = 4.0f;
      else if (f >= 5.0f && f <= 7.0f) candClassHz = 6.0f;
    }
  }

  static float accepted = 0.0f;
  static float pending  = 0.0f;
  static uint8_t confirm = 0;

  if (isRest) {
    accepted = 0.0f; pending = 0.0f; confirm = 0;
  } else if (!ma.rhythmic37 || candClassHz == 0.0f) {
    accepted = 0.0f; pending = 0.0f; confirm = 0;
  } else if (candClassHz != accepted) {
    if (candClassHz == pending) confirm++;
    else { pending = candClassHz; confirm = 1; }

    // FAST: 1 window confirmation (3 seconds)
    if (confirm >= 1) { accepted = candClassHz; confirm = 0; }
  } else {
    pending = accepted;
    confirm = 0;
  }

  AnalysisResult r = {0};
  r.overallRms   = isRest ? 0.0f : 0.20f;
  r.dominantFreq = (accepted == 0.0f) ? 0.0f : ma.peakFreqHz;
  return r;
}

// ------------------------------------------------------------
// One raw sample: sensor read, waveform feed, downsample capture
// ------------------------------------------------------------
static inline void sampleOnce() {
  float x, y, z;
  sensor_read(x, y, z);

  // waveform display: high-pass Z
  gravZ = 0.98f * gravZ + 0.02f * z;
  float s = (z - gravZ) / 4.0f;
  if (s < -1.0f) s = -1.0f;
  if (s >  1.0f) s =  1.0f;
  uiPushSample(s);

  // Q10 conversion
  int16_t xq = (int16_t)(x * ACC_SCALE_F);
  int16_t yq = (int16_t)(y * ACC_SCALE_F);
  int16_t zq = (int16_t)(z * ACC_SCALE_F);

  // Boundary downsample (156 -> 64)
  err += (int32_t)FFT_N;
  if (err >= (int32_t)RAW_N) {
    if (outIdx < FFT_N) {
      axBuf[outIdx] = xq;
      ayBuf[outIdx] = yq;
      azBuf[outIdx] = zq;
      outIdx++;
    }
    err -= (int32_t)RAW_N;
  }

  rawCount++;
}

void setup() {
  uiBegin();
  sensor_init();
  initFFT(FFT_N, FFT_FS_HZ);

  resetWindow();
  nextSampleUs   = micros();
  nextAnalysisMs = millis() + ANALYSIS_PERIOD_MS;

  for (size_t i = 0; i < FFT_N; ++i) {
    axBuf[i] = 0;
    ayBuf[i] = 0;
    azBuf[i] = (int16_t)(9.80665f * ACC_SCALE_F);
  }
}

void loop() {
  uiUpdate();

  // 52 Hz sampling, bounded catch-up
  unsigned long nowUs = micros();
  uint8_t loops = 0;
  while ((long)(nowUs - nextSampleUs) >= 0) {
    nextSampleUs += RAW_SAMPLE_PERIOD_US;
    sampleOnce();
    if (++loops >= 8) break;
    nowUs = micros();
  }

  // Analyze once per 3 seconds ONLY when full window is ready
  unsigned long nowMs = millis();
  if ((long)(nowMs - nextAnalysisMs) >= 0) {

    if (rawCount < RAW_N || outIdx < FFT_N) {
      nextAnalysisMs = nowMs + 20;
      return;
    }

    nextAnalysisMs = nowMs + ANALYSIS_PERIOD_MS;

    MovementAnalysis ma =
      analyzeWindow3D_Magnitude_Q10(axBuf, ayBuf, azBuf, FFT_N, ACC_SCALE_I);

    // REST hysteresis tuned for hand-held stillness (faster return to rest)
    const float REST_ENTER_RMS = 0.115f;
    const float REST_EXIT_RMS  = 0.140f;

    const int32_t ENTER_Q10 = (int32_t)(REST_ENTER_RMS * (float)ACC_SCALE_I + 0.5f);
    const int32_t EXIT_Q10  = (int32_t)(REST_EXIT_RMS  * (float)ACC_SCALE_I + 0.5f);

    const uint32_t ENTER_Q20 = (uint32_t)((int64_t)ENTER_Q10 * (int64_t)ENTER_Q10);
    const uint32_t EXIT_Q20  = (uint32_t)((int64_t)EXIT_Q10  * (int64_t)EXIT_Q10);

    uint32_t msQ20 = meanSquare3D_Q20_meanRemoved(axBuf, ayBuf, azBuf, FFT_N);

    static bool restState = true;
    if (restState) {
      if (msQ20 > EXIT_Q20) restState = false;
    } else {
      if (msQ20 < ENTER_Q20) restState = true;
    }

    uiSetLatestAnalysis(buildStableUiResult(ma, restState));

    resetWindow();
  }
}
