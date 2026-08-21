#include "state/PluginState.h"

namespace pw8::plugin
{
    // clang-format off

    // Matches op::OperatorParams: engine, classic.waveform, classic.morph,
    // classic.pulseWidth, wavetableFramePosition, frequencyRatio, fixedFrequencyHz,
    // keyTrack, level, fmModulatorRatio, fmModulatorIndex, fmModulatorFeedback,
    // fmModulatorWaveform, noiseVariant, noiseRate, phaseBend, phaseFold,
    // phaseAsymmetry, phaseShape, additivePartialCount, additiveTilt,
    // additiveOddEven, additiveStretch, resonatorStructure, resonatorDecay,
    // resonatorDamping, resonatorBrightness, resonatorModeCount, grainDensity,
    // grainSizeMs, grainPositionJitter, grainPitchJitter, wtBend, wtAsymmetry,
    // wtSyncRatio, wtSyncAmount, wtFormantShift.
    const std::array<ParamFieldSpec, kNumOperatorFields> kOperatorFieldSpecs = {{
        {"Engine",             "Engine",             0.0f,   8.0f, 0.0f,   true},
        {"Waveform",           "Waveform",           0.0f,   3.0f, 2.0f,   true},
        {"Morph",               "Morph",             -1.0f,   1.0f, -1.0f,  false},
        {"PulseWidth",         "Pulse Width",         0.01f,  0.99f, 0.5f,  false},
        {"WavetablePos",       "Wavetable Pos",       0.0f,   1.0f, 0.0f,   false},
        {"FreqRatio",          "Freq Ratio",          0.001f, 128.0f, 1.0f, false},
        {"FixedHz",            "Fixed Hz",            0.01f,  24000.0f, 440.0f, false},
        {"KeyTrack",           "Key Track",           0.0f,   1.0f, 1.0f,   true},
        {"Level",              "Level",               0.0f,   4.0f, 1.0f,   false},
        {"FmModulatorRatio",   "FM Mod Ratio",        0.001f, 32.0f, 1.0f,  false},
        {"FmModulatorIndex",   "FM Mod Index",        0.0f,   2.0f, 0.5f,   false},
        {"FmModulatorFeedback","FM Mod Feedback",     0.0f,   1.0f, 0.0f,   false},
        {"FmModulatorWaveform","FM Mod Waveform",     0.0f,   3.0f, 0.0f,   true},
        {"NoiseVariant",       "Noise Variant",       0.0f,   6.0f, 0.0f,   true},
        {"NoiseRate",          "Noise Rate",          0.5f,   2000.0f, 200.0f, false},
        {"PhaseBend",          "Phase Bend",         -1.0f,   1.0f, 0.0f,   false},
        {"PhaseFold",          "Phase Fold",          0.0f,   1.0f, 0.0f,   false},
        {"PhaseAsymmetry",     "Phase Asymmetry",    -1.0f,   1.0f, 0.0f,   false},
        {"PhaseShape",         "Phase Shape",         0.0f,   1.0f, 0.0f,   false},
        {"AdditivePartialCount", "Partials",  1.0f,  64.0f, 32.0f, true},
        {"AdditiveTilt",         "Tilt",     -1.0f,   1.0f, 0.0f,  false},
        {"AdditiveOddEven",      "Odd/Even",  0.0f,   1.0f, 0.5f,  false},
        {"AdditiveStretch",      "Stretch",  -1.0f,   1.0f, 0.0f,  false},
        {"ResonatorStructure",  "Structure",   0.0f, 1.0f, 0.3f, false},
        {"ResonatorDecay",      "Decay",       0.0f, 1.0f, 0.5f, false},
        {"ResonatorDamping",    "Damping",     0.0f, 1.0f, 0.5f, false},
        {"ResonatorBrightness", "Brightness",  0.0f, 1.0f, 0.5f, false},
        {"ResonatorModeCount",  "Modes",       2.0f, 8.0f, 6.0f, true},
        {"GrainDensity",         "Density",   0.5f, 200.0f, 20.0f, false},
        {"GrainSizeMs",          "Size Ms",   1.0f, 500.0f, 60.0f, false},
        {"GrainPositionJitter",  "Pos Jit",   0.0f,   1.0f, 0.1f,  false},
        {"GrainPitchJitter",     "Pitch Jit", 0.0f,   1.0f, 0.0f,  false},
        {"WtBend",               "WT Bend",  -1.0f,   1.0f, 0.0f,  false},
        {"WtAsymmetry",          "WT Asym",  -1.0f,   1.0f, 0.0f,  false},
        // WtSyncRatio / WtFormantShift: sync ratio APVTS-only; formant has PLAY knob (Week 5).
        // WtSyncAmount is on PLAY OSC page (Week 4 sync DSP).
        // (see docs/adr/play-warp-knobs.md). Not exposed on PLAY OSC page — Bend + Asym only.
        {"WtSyncRatio",          "WT Sync Ratio", 1.0f, 16.0f, 1.0f, false},
        {"WtSyncAmount",         "WT Sync Amt",   0.0f,  1.0f, 0.0f, false},
        {"WtFormantShift",       "WT Formant",   -1.0f,  1.0f, 0.0f, false},
        {"WtMorphMode",          "WT Morph Mode", 0.0f,  2.0f, 0.0f, true},
        {"ExternalInputSource",  "External Input", 0.0f,  3.0f, 0.0f, true},
    }};

    // Matches filter::FilterParams: enabled, mode, cutoffHz, resonance, keyTrack.
    const std::array<ParamFieldSpec, kNumFilterFields> kFilterFieldSpecs = {{
        {"Enabled",   "Global Filter Enabled", 0.0f,  1.0f,     0.0f,    true},
        {"Mode",      "Global Filter Mode",    0.0f,  4.0f,     0.0f,    true},
        {"CutoffHz",  "Global Cutoff Hz",      10.0f, 24000.0f, 8000.0f, false},
        {"Resonance", "Global Resonance",      0.0f,  1.0f,     0.2f,    false},
        {"KeyTrack",  "Global Key Track",     -1.0f,  1.0f,     0.0f,    false},
        {"ModeMorph", "Global Filter Mode Morph", 0.0f, 1.0f, 0.0f, false},
    }};

    const std::array<ParamFieldSpec, kNumFilter2Fields> kFilter2FieldSpecs = {{
        {"Enabled",   "Filter 2 Enabled", 0.0f,  1.0f,     0.0f,    true},
        {"CutoffHz",  "Filter 2 Cutoff",  20.0f, 24000.0f, 4000.0f, false},
        {"Resonance", "Filter 2 Reso",    0.0f,  1.0f,     0.3f,    false},
        {"Drive",     "Filter 2 Drive",   0.0f,  1.0f,     0.0f,    false},
        {"KeyTrack",  "Filter 2 Key Trk", -1.0f,  1.0f,     0.0f,    false},
        {"CutoffOffsetSemis", "Filter 2 Cutoff Offset Semis", -48.0f, 48.0f, 0.0f, false},
        {"ModeMorph", "Filter 2 Mode Morph", 0.0f, 1.0f, 0.0f, false},
    }};

    const std::array<ParamFieldSpec, kNumOperatorFilterFields> kOperatorFilterFieldSpecs = {{
        {"FilterEnabled",   "Engine Filter Enabled", 0.0f,  1.0f,     0.0f,    true},
        {"FilterMode",      "Engine Filter Mode",    0.0f,  4.0f,     0.0f,    true},
        {"FilterCutoffHz",  "Engine Cutoff Hz",      10.0f, 24000.0f, 8000.0f, false},
        {"FilterResonance", "Engine Resonance",      0.0f,  1.0f,     0.2f,    false},
        {"FilterKeyTrack",  "Engine Key Track",     -1.0f,  1.0f,     0.0f,    false},
    }};

    // Matches lfo::LfoParams: waveform, mode, rateHz, syncDivisionIndex, phaseOffset.
    const std::array<ParamFieldSpec, kNumLfoFields> kLfoFieldSpecs = {{
        {"Waveform",         "Waveform",           0.0f,   5.0f,  0.0f, true},
        {"Mode",             "Mode",               0.0f,   3.0f,  0.0f, true},
        {"RateHz",           "Rate Hz",            0.001f, 50.0f, 2.0f, false},
        {"SyncDivisionIndex", "Sync Division",      0.0f,   9.0f,  4.0f, true},
        {"PhaseOffset",      "Phase Offset",       0.0f,   1.0f,  0.0f, false},
    }};

    // Matches envelope::DahdsrParams: delaySeconds, attackSeconds, holdSeconds,
    // decaySeconds, sustainLevel, releaseSeconds, curveShape, legato.
    const std::array<ParamFieldSpec, kNumEnvelopeFields> kEnvelopeFieldSpecs = {{
        {"Delay",   "Delay",   0.0f, 60.0f, 0.0f,   false},
        {"Attack",  "Attack",  0.0f, 60.0f, 0.005f, false},
        {"Hold",    "Hold",    0.0f, 60.0f, 0.0f,   false},
        {"Decay",   "Decay",   0.0f, 60.0f, 0.2f,   false},
        {"Sustain", "Sustain", 0.0f, 1.0f,  0.7f,   false},
        {"Release", "Release", 0.0f, 60.0f, 0.3f,   false},
        {"Curve",   "Curve",   0.0f, 16.0f, 2.0f,   false},
        {"Legato",  "Legato",  0.0f, 1.0f,  0.0f,   true},
    }};

    // Matches effects::EffectSlotParams's scalar fields (excludes `nodes[]` and
    // FractalEcho's `fractalSeedA/B` -- see PluginState.h for why).
    const std::array<ParamFieldSpec, kNumEffectSlotFields> kEffectSlotFieldSpecs = {{
        {"Type",               "Type",                 0.0f,     13.0f,    0.0f,   true},
        {"Mix",                "Mix",                  0.0f,     1.0f,     1.0f,   false},
        {"SaturationDrive",    "Saturation Drive",      0.0f,     48.0f,    6.0f,   false},
        {"ChorusRate",         "Chorus Rate",           0.01f,    10.0f,    0.5f,   false},
        {"ChorusDepth",        "Chorus Depth",          0.0f,     20.0f,    4.0f,   false},
        {"ChorusBaseDelay",    "Chorus Base Delay",     1.0f,     40.0f,    12.0f,  false},
        {"TapeDelayMs",        "Tape Delay Ms",         1.0f,     2000.0f,  350.0f, false},
        {"TapeFeedback",       "Tape Feedback",         0.0f,     0.98f,    0.4f,   false},
        {"TapeDrive",          "Tape Drive",            0.0f,     48.0f,    3.0f,   false},
        {"TapeDuck",           "Tape Duck",             0.0f,     1.0f,     0.0f,   false},
        {"TapeDriftDepth",     "Tape Drift Depth",      0.0f,     20.0f,    1.5f,   false},
        {"TapeDriftRate",      "Tape Drift Rate",       0.0f,     10.0f,    0.3f,   false},
        {"TapePanMode",        "Tape Pan Mode",         0.0f,     2.0f,     0.0f,   true},
        {"NodeInsanity",       "Node Insanity",         0.0f,     1.0f,     0.0f,   false},
        {"FreqShiftHz",        "Freq Shift Hz",        -2000.0f,  2000.0f,  7.0f,   false},
        {"FreqShiftDelayMs",   "Freq Shift Delay Ms",   1.0f,     2000.0f,  280.0f, false},
        {"FreqShiftFeedback",  "Freq Shift Feedback",   0.0f,     0.98f,    0.55f,  false},
        {"FreqShiftLowCutHz",  "Freq Shift Low Cut Hz", 5.0f,     20000.0f, 120.0f, false},
        {"FreqShiftHighCutHz", "Freq Shift High Cut Hz", 20.0f,   20000.0f, 8000.0f, false},
        {"FractalMorph",       "Fractal Morph",         0.0f,     1.0f,     0.0f,   false},
        {"FractalBaseDelayMs", "Fractal Base Delay Ms", 1.0f,     1500.0f,  180.0f, false},
        {"FractalRatio",       "Fractal Ratio",         0.1f,     0.95f,    0.62f,  false},
        {"FractalSpreadMs",    "Fractal Spread Ms",     0.0f,     100.0f,   15.0f,  false},
        {"ReverbSize",              "Reverb Size",               0.2f,    3.0f,     1.0f,    false},
        {"ReverbDecaySeconds",      "Reverb Decay Seconds",      0.05f,   20.0f,    2.0f,    false},
        {"ReverbPreDelayMs",        "Reverb Pre-Delay Ms",       0.0f,    200.0f,   20.0f,   false},
        {"ReverbHighRatio",         "Reverb High Ratio",         0.2f,    1.0f,     0.6f,    false},
        {"ReverbHighCrossoverHz",   "Reverb High Crossover Hz",  200.0f,  16000.0f, 4500.0f, false},
        {"ReverbLowRatio",          "Reverb Low Ratio",          0.2f,    4.0f,     1.3f,    false},
        {"ReverbLowCrossoverHz",    "Reverb Low Crossover Hz",   80.0f,   4800.0f,  400.0f,  false},
        {"ReverbDiffusion",         "Reverb Diffusion",          0.0f,    1.0f,     0.65f,   false},
        {"ReverbDensity",           "Reverb Density",            0.0f,    1.0f,     0.85f,   false},
        {"ReverbModDepth",          "Reverb Mod Depth",          0.0f,    1.0f,     0.35f,   false},
        {"ReverbModRateHz",         "Reverb Mod Rate Hz",        0.05f,   2.0f,     0.4f,    false},
        {"ReverbEarlyLevel",        "Reverb Early Level",        0.0f,    1.0f,     0.5f,    false},
        {"ReverbLateLevel",         "Reverb Late Level",         0.0f,    1.0f,     1.0f,    false},
        {"ReverbRollOffHz",         "Reverb Roll Off Hz",        80.0f,   20000.0f, 12000.0f, false},
        {"ReverbVlfCutDb",          "Reverb Vlf Cut Db",        -18.0f,   0.0f,     0.0f,    false},
        {"EqLowFreqHz",        "Eq Low Freq Hz",        20.0f,    20000.0f, 200.0f, false},
        {"EqLowGainDb",        "Eq Low Gain Db",       -24.0f,    24.0f,    0.0f,   false},
        {"EqMidFreqHz",        "Eq Mid Freq Hz",        20.0f,    20000.0f, 1000.0f, false},
        {"EqMidGainDb",        "Eq Mid Gain Db",       -24.0f,    24.0f,    0.0f,   false},
        {"EqMidQ",             "Eq Mid Q",              0.1f,     10.0f,    0.8f,   false},
        {"EqHighFreqHz",       "Eq High Freq Hz",       20.0f,    20000.0f, 6000.0f, false},
        {"EqHighGainDb",       "Eq High Gain Db",      -24.0f,    24.0f,    0.0f,   false},
        {"CompThresholdDb",    "Comp Threshold Db",    -60.0f,    0.0f,    -18.0f,  false},
        {"CompRatio",          "Comp Ratio",            1.0f,     20.0f,    3.0f,   false},
        {"CompAttackMs",       "Comp Attack Ms",        0.1f,     500.0f,   8.0f,   false},
        {"CompReleaseMs",      "Comp Release Ms",       1.0f,     2000.0f,  120.0f, false},
        {"CompKneeDb",         "Comp Knee Db",          0.0f,     24.0f,    6.0f,   false},
        {"CompMakeupDb",       "Comp Makeup Db",        0.0f,     24.0f,    0.0f,   false},
        {"CompTransformerCore", "Comp Transformer Core", 0.0f,     3.0f,     0.0f,   true},
        {"CompTransformerBrand", "Comp Transformer Brand", 0.0f,    3.0f,     0.0f,   true},
        {"CompTransformerAmount", "Comp Transformer Amount", 0.0f,  1.0f,     1.0f,   false},
        {"LimiterCeilingDb",   "Limiter Ceiling Db",   -12.0f,    0.0f,    -0.3f,   false},
        {"LimiterLookaheadMs", "Limiter Lookahead Ms",  0.5f,     20.0f,    5.0f,   false},
        {"LimiterReleaseMs",   "Limiter Release Ms",    1.0f,     2000.0f,  60.0f,  false},
        {"TapeDelaySync",      "Tape Delay Sync",       0.0f,     1.0f,     0.0f,   true},
        {"TapeDelaySyncDivision", "Tape Delay Sync Division", 0.0f, 8.0f, 2.0f, true},
        {"CompAutoMakeup",     "Comp Auto Makeup",      0.0f,     1.0f,     0.0f,   true},
        {"CompCharacter",      "Comp Character",        0.0f,     2.0f,     0.0f,   true},
        {"VocoderBandCount",   "Vocoder Bands",         8.0f,     16.0f,    8.0f,   true},
        {"VocoderFormant",     "Vocoder Formant",       0.0f,     1.0f,     0.5f,   false},
        {"VocoderSibilance",   "Vocoder Sibilance",     0.0f,     1.0f,     0.0f,   false},
        {"VocoderScGainDb",    "Vocoder SC Gain Db",    0.0f,     48.0f,    0.0f,   false},
        {"VocoderReleaseMs",   "Vocoder Release Ms",    5.0f,     250.0f,   40.0f,  false},
        {"ReverbCharacter",    "Reverb Character",      0.0f,     4.0f,     0.0f,   true},
        {"SaturationCharacter", "Saturation Character",   0.0f,     4.0f,     0.0f,   true},
        {"EqOutGainDb",        "Eq Out Gain Db",       -12.0f,    12.0f,    0.0f,   false},
        {"CloudsDensity",      "Clouds Density",        0.0f,     1.0f,     0.35f,  false},
        {"CloudsGrainSizeMs",  "Clouds Grain Size Ms",  5.0f,     500.0f,   80.0f,  false},
        {"CloudsPitch",        "Clouds Pitch",          0.25f,    4.0f,     1.0f,   false},
        {"CloudsFreeze",       "Clouds Freeze",         0.0f,     1.0f,     0.0f,   false},
        {"CloudsMode",         "Clouds Mode",           0.0f,     2.0f,     0.0f,   true},
        {"Qsr1Level",          "QSR1 Level",            0.0f,     1.0f,     0.65f,  false},
        {"Qsr2Level",          "QSR2 Level",            0.0f,     1.0f,     0.55f,  false},
        {"CntrLevel",          "Center Level",          0.0f,     1.0f,     0.85f,  false},
        {"InputSplitHpfHz",    "Input Split HPF Hz",    20.0f,    500.0f,   120.0f, false},
        {"CntrHpfHz",          "Center HPF Hz",         20.0f,    300.0f,   80.0f,  false},
        {"Qsr1Height",         "QSR1 Height",          -1.0f,     1.0f,     0.0f,   false},
        {"Qsr1AngleDeg",       "QSR1 Angle Deg",        0.0f,     360.0f,   30.0f,  false},
        {"Qsr1Distance",       "QSR1 Distance",         0.0f,     1.0f,     0.35f,  false},
        {"Qsr2Height",         "QSR2 Height",          -1.0f,     1.0f,     0.0f,   false},
        {"Qsr2AngleDeg",       "QSR2 Angle Deg",        0.0f,     360.0f,   330.0f, false},
        {"Qsr2Distance",       "QSR2 Distance",         0.0f,     1.0f,     0.4f,   false},
        {"Qsr1RoomAmount",     "QSR1 Room Amount",      0.0f,     1.0f,     0.45f,  false},
        {"Qsr1RoomSize",       "QSR1 Room Size",        0.2f,     3.0f,     1.0f,   false},
        {"Qsr1RoomDamping",    "QSR1 Room Damping",     0.0f,     1.0f,     0.55f,  false},
        {"Qsr2RoomAmount",     "QSR2 Room Amount",      0.0f,     1.0f,     0.40f,  false},
        {"Qsr2RoomSize",       "QSR2 Room Size",        0.2f,     3.0f,     1.1f,   false},
        {"Qsr2RoomDamping",    "QSR2 Room Damping",     0.0f,     1.0f,     0.50f,  false},
        {"QuasarDelayTimeMs",  "Quasar Delay Time Ms",  3.0f,     20000.0f, 450.0f, false},
        {"QuasarDelayFeedback", "Quasar Delay Feedback", 0.0f,    0.95f,    0.35f,  false},
        {"QuasarDelayVolume",  "Quasar Delay Volume",   0.0f,     1.0f,     0.25f,  false},
        {"QuasarOutputMode",   "Quasar Output Mode",    0.0f,     2.0f,     0.0f,   true},
        {"QuasarCrossfeed",    "Quasar Crossfeed",      0.0f,     1.0f,     0.0f,   false},
        {"QuasarDelaySync",    "Quasar Delay Sync",     0.0f,     1.0f,     0.0f,   true},
        {"QuasarDelaySyncDivision", "Quasar Delay Sync Division", 0.0f, 8.0f, 2.0f, true},
        {"QsrStereoSplit",     "QSR Stereo Split",      0.0f,     1.0f,     1.0f,   true},
    }};

    // Matches sequencer::ArpeggiatorParams's scalar fields (excludes `steps[]`).
    const std::array<ParamFieldSpec, kNumArpFields> kArpFieldSpecs = {{
        {"Enabled",          "Enabled",           0.0f, 1.0f,  0.0f, true},
        {"Mode",              "Mode",              0.0f, 6.0f,  0.0f, true},
        {"RateMode",          "Rate Mode",         0.0f, 1.0f,  1.0f, true},
        {"RateHz",            "Rate Hz",           0.1f, 100.0f, 8.0f, false},
        {"SyncDivisionIndex", "Sync Division",     0.0f, 9.0f,  6.0f, true},
        {"OctaveRange",       "Octave Range",      1.0f, 4.0f,  1.0f, true},
        {"NumSteps",          "Num Steps",         1.0f, 64.0f, 8.0f, true},
        {"Swing",             "Swing",             0.0f, 1.0f,  0.0f, false},
        {"Latch",             "Latch",             0.0f, 1.0f,  0.0f, true},
    }};

    const std::array<ParamFieldSpec, kNumGenerativeFields> kGenerativeFieldSpecs = {{
        {"DejaVu",          "Generative Deja Vu",       0.0f, 1.0f, 1.0f, true},
        {"SeedLocked",      "Generative Seed Locked",   0.0f, 1.0f, 0.0f, true},
        {"ClockTRateHz",    "Generative T Rate Hz",     0.01f, 40.0f, 0.47f, false},
        {"ClockXRateHz",    "Generative X Rate Hz",     0.01f, 40.0f, 4.7f, false},
        {"Correlation",     "Generative Correlation",   0.0f, 1.0f, 0.0f, false},
        {"Stream0Spread",   "Generative Stream 0 Spread", 0.0f, 1.0f, 0.5f, false},
        {"Stream0Bias",     "Generative Stream 0 Bias", -1.0f, 1.0f, 0.0f, false},
        {"Stream0LagMs",    "Generative Stream 0 Lag Ms", 0.1f, 5000.0f, 80.0f, false},
        {"Stream1Spread",   "Generative Stream 1 Spread", 0.0f, 1.0f, 0.5f, false},
        {"Stream1Bias",     "Generative Stream 1 Bias", -1.0f, 1.0f, 0.0f, false},
        {"Stream1LagMs",    "Generative Stream 1 Lag Ms", 0.1f, 5000.0f, 80.0f, false},
        {"Stream2Spread",   "Generative Stream 2 Spread", 0.0f, 1.0f, 0.5f, false},
        {"Stream2Bias",     "Generative Stream 2 Bias", -1.0f, 1.0f, 0.0f, false},
        {"Stream2LagMs",    "Generative Stream 2 Lag Ms", 0.1f, 5000.0f, 80.0f, false},
        {"Stream3Spread",   "Generative Stream 3 Spread", 0.0f, 1.0f, 0.5f, false},
        {"Stream3Bias",     "Generative Stream 3 Bias", -1.0f, 1.0f, 0.0f, false},
        {"Stream3LagMs",    "Generative Stream 3 Lag Ms", 0.1f, 5000.0f, 80.0f, false},
        {"OutputRoute0",    "Generative Output Route 0", 0.0f, 5.0f, 0.0f, true},
        {"OutputRoute1",    "Generative Output Route 1", 0.0f, 5.0f, 1.0f, true},
        {"OutputRoute2",    "Generative Output Route 2", 0.0f, 5.0f, 2.0f, true},
        {"OutputRoute3",    "Generative Output Route 3", 0.0f, 5.0f, 3.0f, true},
        {"OutputRoute4",    "Generative Output Route 4", 0.0f, 5.0f, 4.0f, true},
        {"OutputRoute5",    "Generative Output Route 5", 0.0f, 5.0f, 5.0f, true},
    }};

    const std::array<ParamFieldSpec, kNumPeaksUtilitySlotFields> kPeaksUtilitySlotFieldSpecs = {{
        {"Enabled",   "Peaks Utility Enabled", 0.0f, 1.0f, 0.0f, true},
        {"Mode",      "Peaks Utility Mode",    0.0f, 1.0f, 0.0f, true},
        {"AttackMs",  "Peaks Utility Attack Ms", 0.1f, 5000.0f, 5.0f, false},
        {"ReleaseMs", "Peaks Utility Release Ms", 1.0f, 5000.0f, 80.0f, false},
        {"LfoRateHz", "Peaks Utility LFO Rate Hz", 0.01f, 40.0f, 0.5f, false},
        {"LfoDepth",  "Peaks Utility LFO Depth", 0.0f, 1.0f, 0.5f, false},
    }};

    const std::array<ParamFieldSpec, kNumMasterDynamicsFields> kMasterDynamicsFieldSpecs = {{
        {"Enabled",       "Master Dynamics Enable", 0.0f, 1.0f, 0.0f, true},
        {"Mode",          "Master Dynamics Mode",   0.0f, 3.0f, 0.0f, true},
        {"ThresholdDb",   "Master Dynamics Threshold", -60.0f, 0.0f, -12.0f, false},
        {"Ratio",         "Master Dynamics Ratio",  1.0f, 20.0f, 4.0f, false},
        {"AttackMs",      "Master Dynamics Attack", 0.1f, 500.0f, 5.0f, false},
        {"ReleaseMs",     "Master Dynamics Release", 1.0f, 5000.0f, 80.0f, false},
        {"SidechainGain", "Master Dynamics Sidechain Gain", 0.0f, 2.0f, 1.0f, false},
        {"VactrolSlewMs", "Master Dynamics Vactrol Slew", 1.0f, 5000.0f, 40.0f, false},
        {"MakeupDb",      "Master Dynamics Makeup", 0.0f, 24.0f, 0.0f, false},
        {"Mix",           "Master Dynamics Mix",    0.0f, 1.0f, 1.0f, false},
    }};

    const std::array<ParamFieldSpec, kNumOperatorMixFields> kOperatorMixFieldSpecs = {{
        {"MixEnabled", "Mix Enabled", 0.0f, 1.0f, 1.0f, true},
        {"MixMute",    "Mix Mute",    0.0f, 1.0f, 0.0f, true},
        {"MixSolo",    "Mix Solo",    0.0f, 1.0f, 0.0f, true},
    }};

    // clang-format on

    juce::String operatorParamId(std::size_t opIndex, const char* fieldSuffix)
    {
        return "op" + juce::String(static_cast<int>(opIndex)) + fieldSuffix;
    }

    juce::String operatorMixParamId(std::size_t opIndex, const char* fieldSuffix)
    {
        return operatorParamId(opIndex, fieldSuffix);
    }

    juce::String operatorFilterParamId(std::size_t opIndex, const char* fieldSuffix)
    {
        return operatorParamId(opIndex, fieldSuffix);
    }

    juce::String lfoParamId(std::size_t lfoIndex, const char* fieldSuffix)
    {
        return "lfo" + juce::String(static_cast<int>(lfoIndex)) + fieldSuffix;
    }

    juce::String envelopeParamId(std::size_t envIndex, const char* fieldSuffix)
    {
        return "env" + juce::String(static_cast<int>(envIndex)) + fieldSuffix;
    }

    juce::String insertFxParamId(std::size_t slot, const char* fieldSuffix)
    {
        return "insertFx" + juce::String(static_cast<int>(slot)) + fieldSuffix;
    }

    juce::String masterFxParamId(std::size_t slot, const char* fieldSuffix)
    {
        return "masterFx" + juce::String(static_cast<int>(slot)) + fieldSuffix;
    }

    juce::String peaksUtilitySlotParamId(std::size_t slotIndex, const char* fieldSuffix)
    {
        return juce::String(kPeaksUtilityIdPrefix) + "Slot" + juce::String(static_cast<int>(slotIndex)) + fieldSuffix;
    }

    namespace
    {
        void addParam(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const juce::String& id,
                       const ParamFieldSpec& spec)
        {
            const float interval = spec.discrete ? 1.0f : 0.0f;
            const juce::NormalisableRange<float> range =
                interval > 0.0f ? juce::NormalisableRange<float>(spec.minValue, spec.maxValue, interval)
                                : juce::NormalisableRange<float>(spec.minValue, spec.maxValue);
            params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id, 1}, spec.label, range,
                                                                           spec.defaultValue));
        }
    } // namespace

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        params.reserve(8 + kNumFilterFields + kNumFilter2Fields + kNumOperators * kNumOperatorFilterFields +
                       kNumOperators * kNumOperatorMixFields + kNumLfos * kNumLfoFields +
                       kNumOperators * kNumOperatorFields + kNumEnvelopes * kNumEnvelopeFields +
                       3 + (kNumInsertFxSlots + kNumMasterFxSlots) * kNumEffectSlotFields + kNumArpFields);

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{kMacroParameterIds[i], 1}, kMacroParameterNames[i],
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{kModWheelId, 1}, kModWheelName, juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{kExpressionId, 1}, kExpressionName, juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{kMorphPositionId, 1}, kMorphPositionName, juce::NormalisableRange<float>(0.0f, 1.0f),
            0.0f));

        for (const auto& spec : kFilterFieldSpecs)
            addParam(params, juce::String(kFilterIdPrefix) + spec.idSuffix, spec);

        for (const auto& spec : kFilter2FieldSpecs)
            addParam(params, juce::String(kFilter2IdPrefix) + spec.idSuffix, spec);

        addParam(params, kFilterRoutingId,
                 ParamFieldSpec{"", kFilterRoutingName, 0.0f, 1.0f, 0.0f, false});

        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
            for (const auto& spec : kLfoFieldSpecs)
                addParam(params, lfoParamId(lfo, spec.idSuffix), spec);

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (const auto& spec : kOperatorFieldSpecs)
                addParam(params, operatorParamId(op, spec.idSuffix), spec);

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (const auto& spec : kOperatorFilterFieldSpecs)
                addParam(params, operatorFilterParamId(op, spec.idSuffix), spec);

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (const auto& spec : kOperatorMixFieldSpecs)
                addParam(params, operatorMixParamId(op, spec.idSuffix), spec);

        for (std::size_t env = 0; env < kNumEnvelopes; ++env)
            for (const auto& spec : kEnvelopeFieldSpecs)
                addParam(params, envelopeParamId(env, spec.idSuffix), spec);

        addParam(params, kLayerGainId, ParamFieldSpec{"", "Layer Gain", 0.0f, 4.0f, 1.0f, false});
        addParam(params, kLayerPanId, ParamFieldSpec{"", "Layer Pan", -1.0f, 1.0f, 0.0f, false});
        addParam(params, kMasterGainId, ParamFieldSpec{"", "Master Gain", 0.0f, 4.0f, 1.0f, false});
        addParam(params, kPortamentoId, ParamFieldSpec{"", kPortamentoName, 0.0f, 10.0f, 0.0f, false});

        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            for (const auto& spec : kEffectSlotFieldSpecs)
                addParam(params, insertFxParamId(slot, spec.idSuffix), spec);

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            for (const auto& spec : kEffectSlotFieldSpecs)
                addParam(params, masterFxParamId(slot, spec.idSuffix), spec);

        for (const auto& spec : kArpFieldSpecs)
            addParam(params, juce::String(kArpIdPrefix) + spec.idSuffix, spec);

        for (const auto& spec : kMasterDynamicsFieldSpecs)
            addParam(params, juce::String(kMasterDynamicsIdPrefix) + spec.idSuffix, spec);

        for (const auto& spec : kGenerativeFieldSpecs)
            addParam(params, juce::String(kGenerativeIdPrefix) + spec.idSuffix, spec);

        for (std::size_t slot = 0; slot < kNumPeaksUtilitySlots; ++slot)
            for (const auto& spec : kPeaksUtilitySlotFieldSpecs)
                addParam(params, peaksUtilitySlotParamId(slot, spec.idSuffix), spec);

        addParam(params, kUnisonVoicesId, ParamFieldSpec{"", kUnisonVoicesName, 1.0f, 16.0f, 1.0f, true});
        addParam(params, kUnisonDetuneId, ParamFieldSpec{"", kUnisonDetuneName, 0.0f, 100.0f, 0.0f, false});
        addParam(params, kUnisonSpreadId, ParamFieldSpec{"", kUnisonSpreadName, 0.0f, 1.0f, 0.0f, false});
        addParam(params, kUnisonPhaseRandomId, ParamFieldSpec{"", kUnisonPhaseRandomName, 0.0f, 1.0f, 0.0f, false});

        addParam(params, kFxRoutingPrePostId, ParamFieldSpec{"", kFxRoutingPrePostName, 0.0f, 1.0f, 0.0f, true});
        addParam(params, kFxGlobalBypassId, ParamFieldSpec{"", kFxGlobalBypassName, 0.0f, 1.0f, 0.0f, true});
        addParam(params, kFxGlobalWetMixId, ParamFieldSpec{"", kFxGlobalWetMixName, 0.0f, 1.0f, 0.8f, false});
        addParam(params, kFxSendAId, ParamFieldSpec{"", kFxSendAName, 0.0f, 1.0f, 0.0f, false});
        addParam(params, kFxSendBId, ParamFieldSpec{"", kFxSendBName, 0.0f, 1.0f, 0.0f, false});

        return {params.begin(), params.end()};
    }

    std::array<float, kNumEffectSlotFields> effectSlotFieldValues(const effects::EffectSlotParams& p) noexcept
    {
        return {
            static_cast<float>(p.type), p.mix,        p.saturationDriveDb, p.chorusRateHz,   p.chorusDepthMs,
            p.chorusBaseDelayMs,        p.tapeDelayMs, p.tapeFeedback,      p.tapeDriveDb,    p.tapeDuckAmount,
            p.tapeDriftDepthMs,         p.tapeDriftRateHz, static_cast<float>(p.tapePanMode), p.nodeInsanity,
            p.freqShiftHz,              p.freqShiftDelayMs, p.freqShiftFeedback, p.freqShiftLowCutHz,
            p.freqShiftHighCutHz,       p.fractalMorph, p.fractalBaseDelayMs, p.fractalRatio, p.fractalSpreadMs,
            p.reverbSizeParam,          p.reverbDecaySeconds, p.reverbPreDelayMs,
            p.reverbHighRatio,          p.reverbHighCrossoverHz, p.reverbLowRatio, p.reverbLowCrossoverHz,
            p.reverbDiffusion,          p.reverbDensity, p.reverbModDepth,    p.reverbModRateHz,
            p.reverbEarlyLevel,         p.reverbLateLevel, p.reverbRollOffHz, p.reverbVlfCutDb,
            p.eqLowFreqHz,              p.eqLowGainDb,  p.eqMidFreqHz,       p.eqMidGainDb,    p.eqMidQ,
            p.eqHighFreqHz,             p.eqHighGainDb,
            p.compThresholdDb,          p.compRatio,    p.compAttackMs,      p.compReleaseMs,  p.compKneeDb,
            p.compMakeupDb,
            p.compTransformerCore,      p.compTransformerBrand, p.compTransformerAmount,
            p.limiterCeilingDb,         p.limiterLookaheadMs, p.limiterReleaseMs,
            static_cast<float>(p.tapeDelaySync), static_cast<float>(p.tapeDelaySyncDivisionIndex),
            p.compAutoMakeup ? 1.0f : 0.0f, static_cast<float>(p.compCharacter),
            static_cast<float>(p.vocoderBandCount), p.vocoderFormant, p.vocoderSibilance, p.vocoderScGainDb,
            p.vocoderReleaseMs,
            static_cast<float>(p.reverbCharacter),
            static_cast<float>(p.saturationCharacter),
            p.eqOutGainDb,
            p.cloudsDensity,
            p.cloudsGrainSizeMs,
            p.cloudsPitch,
            p.cloudsFreeze,
            static_cast<float>(p.cloudsMode),
            p.qsr1Level,
            p.qsr2Level,
            p.cntrLevel,
            p.inputSplitHpfHz,
            p.cntrHpfHz,
            p.qsr1Height,
            p.qsr1AngleDeg,
            p.qsr1Distance,
            p.qsr2Height,
            p.qsr2AngleDeg,
            p.qsr2Distance,
            p.qsr1RoomAmount,
            p.qsr1RoomSize,
            p.qsr1RoomDamping,
            p.qsr2RoomAmount,
            p.qsr2RoomSize,
            p.qsr2RoomDamping,
            p.quasarDelayTimeMs,
            p.quasarDelayFeedback,
            p.quasarDelayVolume,
            static_cast<float>(p.quasarOutputMode),
            p.quasarCrossfeed,
            p.quasarDelaySync ? 1.0f : 0.0f,
            static_cast<float>(p.quasarDelaySyncDivisionIndex),
            p.qsrStereoSplit ? 1.0f : 0.0f,
        };
    }

    void applyEffectSlotFieldValues(effects::EffectSlotParams& p,
                                    const std::array<float, kNumEffectSlotFields>& v) noexcept
    {
        p.type = static_cast<effects::EffectType>(static_cast<int>(v[0] + 0.5f));
        p.mix = v[1];
        p.saturationDriveDb = v[2];
        p.chorusRateHz = v[3];
        p.chorusDepthMs = v[4];
        p.chorusBaseDelayMs = v[5];
        p.tapeDelayMs = v[6];
        p.tapeFeedback = v[7];
        p.tapeDriveDb = v[8];
        p.tapeDuckAmount = v[9];
        p.tapeDriftDepthMs = v[10];
        p.tapeDriftRateHz = v[11];
        p.tapePanMode = static_cast<effects::DelayPanMode>(static_cast<int>(v[12] + 0.5f));
        p.nodeInsanity = v[13];
        p.freqShiftHz = v[14];
        p.freqShiftDelayMs = v[15];
        p.freqShiftFeedback = v[16];
        p.freqShiftLowCutHz = v[17];
        p.freqShiftHighCutHz = v[18];
        p.fractalMorph = v[19];
        p.fractalBaseDelayMs = v[20];
        p.fractalRatio = v[21];
        p.fractalSpreadMs = v[22];
        p.reverbSizeParam = v[23];
        p.reverbDecaySeconds = v[24];
        p.reverbPreDelayMs = v[25];
        p.reverbHighRatio = v[26];
        p.reverbHighCrossoverHz = v[27];
        p.reverbLowRatio = v[28];
        p.reverbLowCrossoverHz = v[29];
        p.reverbDiffusion = v[30];
        p.reverbDensity = v[31];
        p.reverbModDepth = v[32];
        p.reverbModRateHz = v[33];
        p.reverbEarlyLevel = v[34];
        p.reverbLateLevel = v[35];
        p.reverbRollOffHz = v[36];
        p.reverbVlfCutDb = v[37];
        p.eqLowFreqHz = v[38];
        p.eqLowGainDb = v[39];
        p.eqMidFreqHz = v[40];
        p.eqMidGainDb = v[41];
        p.eqMidQ = v[42];
        p.eqHighFreqHz = v[43];
        p.eqHighGainDb = v[44];
        p.compThresholdDb = v[45];
        p.compRatio = v[46];
        p.compAttackMs = v[47];
        p.compReleaseMs = v[48];
        p.compKneeDb = v[49];
        p.compMakeupDb = v[50];
        p.compTransformerCore = v[51];
        p.compTransformerBrand = v[52];
        p.compTransformerAmount = v[53];
        p.limiterCeilingDb = v[54];
        p.limiterLookaheadMs = v[55];
        p.limiterReleaseMs = v[56];
        p.tapeDelaySync = v[57] >= 0.5f;
        p.tapeDelaySyncDivisionIndex = static_cast<int>(v[58] + 0.5f);
        p.compAutoMakeup = v[59] >= 0.5f;
        p.compCharacter = static_cast<int>(v[60] + 0.5f);
        p.vocoderBandCount = static_cast<int>(v[61] + 0.5f);
        p.vocoderFormant = v[62];
        p.vocoderSibilance = v[63];
        p.vocoderScGainDb = v[64];
        p.vocoderReleaseMs = v[65];
        p.reverbCharacter = static_cast<int>(v[66] + 0.5f);
        p.saturationCharacter = static_cast<int>(v[67] + 0.5f);
        p.eqOutGainDb = v[68];
        p.cloudsDensity = v[69];
        p.cloudsGrainSizeMs = v[70];
        p.cloudsPitch = v[71];
        p.cloudsFreeze = v[72];
        p.cloudsMode = static_cast<int>(v[73] + 0.5f);
        p.qsr1Level = v[74];
        p.qsr2Level = v[75];
        p.cntrLevel = v[76];
        p.inputSplitHpfHz = v[77];
        p.cntrHpfHz = v[78];
        p.qsr1Height = v[79];
        p.qsr1AngleDeg = v[80];
        p.qsr1Distance = v[81];
        p.qsr2Height = v[82];
        p.qsr2AngleDeg = v[83];
        p.qsr2Distance = v[84];
        p.qsr1RoomAmount = v[85];
        p.qsr1RoomSize = v[86];
        p.qsr1RoomDamping = v[87];
        p.qsr2RoomAmount = v[88];
        p.qsr2RoomSize = v[89];
        p.qsr2RoomDamping = v[90];
        p.quasarDelayTimeMs = v[91];
        p.quasarDelayFeedback = v[92];
        p.quasarDelayVolume = v[93];
        p.quasarOutputMode = static_cast<int>(v[94] + 0.5f);
        p.quasarCrossfeed = v[95];
        p.quasarDelaySync = v[96] >= 0.5f;
        p.quasarDelaySyncDivisionIndex = static_cast<int>(v[97] + 0.5f);
        p.qsrStereoSplit = v[98] >= 0.5f;
    }

} // namespace pw8::plugin
