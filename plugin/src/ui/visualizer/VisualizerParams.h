#pragma once

namespace murmur8
{

/** Parameter snapshot for FX preview shaders (message thread writes, GL thread reads). */
struct FxPreviewParams
{
    int fxKind = 0;
    float mix = 1.0f;
    float paramA0 = 0.0f;
    float paramA1 = 0.0f;
    float paramA2 = 0.0f;
    float paramA3 = 0.0f;
    float paramB0 = 0.0f;
    float paramB1 = 0.0f;
    float paramB2 = 0.0f;
    float paramB3 = 0.0f;
    float paramC0 = 0.0f;
    float paramC1 = 0.0f;
    float paramC2 = 0.0f;
    float paramC3 = 0.0f;
};

struct QuasarFieldParams
{
    float qsr1AngleDeg = 30.0f;
    float qsr1Distance = 0.35f;
    float qsr1Height = 0.0f;
    float qsr1Level = 1.0f;
    float qsr2AngleDeg = 330.0f;
    float qsr2Distance = 0.35f;
    float qsr2Height = 0.0f;
    float qsr2Level = 1.0f;
    float crossfeed = 0.0f;
    float width = 0.5f;
    float mix = 1.0f;
};

struct LfoPreviewParams
{
    float waveform = 0.0f;
    float rateHz = 2.0f;
    float phase = 0.0f;
};

struct FilterPreviewParams
{
    int mode = 0;
    float cutoffNorm = 0.5f;
    float resonance = 0.2f;
};

struct EnvelopeCurveParams
{
    float attackNorm = 0.15f;
    float decayNorm = 0.2f;
    float sustain = 0.7f;
    float releaseNorm = 0.25f;
    float envLevel = 0.0f;
    float envProgress = 0.0f;
};

struct WavetablePreviewParams
{
    float morph = 0.0f;
};

struct GranularPreviewParams
{
    float density = 0.35f;
    float mix = 1.0f;
};

struct VuMeterParams
{
    float peakL = 0.0f;
    float peakR = 0.0f;
    float rmsL = 0.0f;
    float rmsR = 0.0f;
    bool vertical = true;
};

} // namespace murmur8
