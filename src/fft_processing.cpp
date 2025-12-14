#include <Arduino.h>
#include "fft_processing.h"
#include <arduinoFFT.h>

static const size_t MAX_FFT = 64;

static size_t N  = 0;
static float  FS = 0.0f;

static ArduinoFFT<float> FFT;
static float vReal[MAX_FFT];
static float vImag[MAX_FFT];

void initFFT(size_t nSamples, float fs) {
    if (nSamples > MAX_FFT) nSamples = MAX_FFT;
    N = nSamples;
    FS = fs;

    for (size_t i = 0; i < N; ++i) {
        vReal[i] = 0.0f;
        vImag[i] = 0.0f;
    }
}

MovementAnalysis analyzeWindow3D_Magnitude_Q10(const int16_t* axQ10,
                                              const int16_t* ayQ10,
                                              const int16_t* azQ10,
                                              size_t nSamples,
                                              int16_t scaleQ10)
{
    MovementAnalysis analysis{};
    if (N == 0) return analysis;

    if (nSamples > N) nSamples = N;
    if (scaleQ10 == 0) scaleQ10 = 1;
    const float inv = 1.0f / (float)scaleQ10;

    // 1) mean vector
    int32_t sx = 0, sy = 0, sz = 0;
    for (size_t i = 0; i < nSamples; ++i) {
        sx += axQ10[i];
        sy += ayQ10[i];
        sz += azQ10[i];
    }
    const int32_t mxQ = sx / (int32_t)nSamples;
    const int32_t myQ = sy / (int32_t)nSamples;
    const int32_t mzQ = sz / (int32_t)nSamples;

    // 2) SIGNED 3D motion signal (preserves fundamental frequency)
    //    s[i] = dx + dy + dz  where d = a - mean(a)
    for (size_t i = 0; i < N; ++i) {
        float s = 0.0f;
        if (i < nSamples) {
            float dx = (float)((int32_t)axQ10[i] - mxQ) * inv;
            float dy = (float)((int32_t)ayQ10[i] - myQ) * inv;
            float dz = (float)((int32_t)azQ10[i] - mzQ) * inv;
            s = dx + dy + dz;   // <-- key change: signed, still 3D
        }
        vReal[i] = s;
        vImag[i] = 0.0f;
    }

    // 3) remove DC
    float meanS = 0.0f;
    for (size_t i = 0; i < N; ++i) meanS += vReal[i];
    meanS /= (float)N;
    for (size_t i = 0; i < N; ++i) vReal[i] -= meanS;

    // 4) FFT
    FFT.windowing(vReal, N, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(vReal, vImag, N, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, N);

    const float df = FS / (float)N;

    float trem = 0.0f, dysk = 0.0f;
    float band37 = 0.0f;
    float slow = 0.0f;     // <2 Hz (tilt/walk)
    float total = 0.0f;

    float peakP  = 0.0f;
    int   peakI  = -1;
    float peakF  = 0.0f;

    for (size_t i = 1; i < N/2; ++i) {
        float f = i * df;
        float p = vReal[i] * vReal[i];
        total += p;

        if (f < 2.0f) slow += p;

        if (f >= 3.0f && f < 5.0f) trem += p;
        else if (f >= 5.0f && f <= 7.0f) dysk += p;

        if (f >= 3.0f && f <= 7.0f) {
            band37 += p;
            if (p > peakP) { peakP = p; peakI = (int)i; peakF = f; }
        }
    }

    float peakiness = 0.0f;
    if (peakI > 1 && peakI + 1 < (int)(N/2)) {
        float n1 = vReal[peakI - 1] * vReal[peakI - 1];
        float n2 = vReal[peakI + 1] * vReal[peakI + 1];
        float navg = 0.5f * (n1 + n2);
        if (navg > 0.0f) peakiness = peakP / navg;
    }

    // Pattern-first rhythmic gate (sensitive; rejects slow tilt/walk)
    const float DOM_RATIO_MIN   = 0.14f;
    const float PEAKINESS_MIN   = 1.00f;
    const float BAND_SHARE_MIN  = 0.06f;
    const float SLOW_REJECT_MUL = 0.65f;

    float domRatio  = (band37 > 0.0f) ? (peakP / band37) : 0.0f;
    float bandShare = (total  > 0.0f) ? (band37 / total) : 0.0f;

    bool rhythmic = (bandShare >= BAND_SHARE_MIN) &&
                    ((domRatio >= DOM_RATIO_MIN) || (peakiness >= PEAKINESS_MIN)) &&
                    (band37 >= SLOW_REJECT_MUL * slow);

    analysis.tremorPower     = trem;
    analysis.dyskinesiaPower = dysk;
    analysis.rhythmic37      = rhythmic;
    analysis.peakFreqHz      = rhythmic ? peakF : 0.0f;

    return analysis;
}
