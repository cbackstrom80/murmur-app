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
        {"Engine",             "Engine",             0.0f,   7.0f, 0.0f,   true},
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
    }};

    // Matches filter::FilterParams: enabled, mode, cutoffHz, resonance, keyTrack.
    const std::array<ParamFieldSpec, kNumFilterFields> kFilterFieldSpecs = {{
        {"Enabled",   "Global Filter Enabled", 0.0f,  1.0f,     0.0f,    true},
        {"Mode",      "Global Filter Mode",    0.0f,  4.0f,     0.0f,    true},
        {"CutoffHz",  "Global Cutoff Hz",      10.0f, 24000.0f, 8000.0f, false},
        {"Resonance", "Global Resonance",      0.0f,  1.0f,     0.2f,    false},
        {"KeyTrack",  "Global Key Track",     -1.0f,  1.0f,     0.0f,    false},
    }};

    const std::array<ParamFieldSpec, kNumFilter2Fields> kFilter2FieldSpecs = {{
        {"Enabled",   "Filter 2 Enabled", 0.0f,  1.0f,     0.0f,    true},
        {"CutoffHz",  "Filter 2 Cutoff",  20.0f, 24000.0f, 4000.0f, false},
        {"Resonance", "Filter 2 Reso",    0.0f,  1.0f,     0.3f,    false},
        {"Drive",     "Filter 2 Drive",   0.0f,  1.0f,     0.0f,    false},
        {"KeyTrack",  "Filter 2 Key Trk", -1.0f,  1.0f,     0.0f,    false},
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
        {"Type",               "Type",                 0.0f,     10.0f,    0.0f,   true},
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
        {"Latch",             "Latch",             0.0f, 1.0f,  0.0f, true},
    }};

    // clang-format on

    juce::String operatorParamId(std::size_t opIndex, const char* fieldSuffix)
    {
        return "op" + juce::String(static_cast<int>(opIndex)) + fieldSuffix;
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
                       kNumLfos * kNumLfoFields + kNumOperators * kNumOperatorFields + kNumEnvelopes * kNumEnvelopeFields +
                       3 + (kNumInsertFxSlots + kNumMasterFxSlots) * kNumEffectSlotFields + kNumArpFields);

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{kMacroParameterIds[i], 1}, kMacroParameterNames[i],
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{kModWheelId, 1}, kModWheelName, juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{kExpressionId, 1}, kExpressionName, juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        for (const auto& spec : kFilterFieldSpecs)
            addParam(params, juce::String(kFilterIdPrefix) + spec.idSuffix, spec);

        for (const auto& spec : kFilter2FieldSpecs)
            addParam(params, juce::String(kFilter2IdPrefix) + spec.idSuffix, spec);

        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
            for (const auto& spec : kLfoFieldSpecs)
                addParam(params, lfoParamId(lfo, spec.idSuffix), spec);

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (const auto& spec : kOperatorFieldSpecs)
                addParam(params, operatorParamId(op, spec.idSuffix), spec);

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (const auto& spec : kOperatorFilterFieldSpecs)
                addParam(params, operatorFilterParamId(op, spec.idSuffix), spec);

        for (std::size_t env = 0; env < kNumEnvelopes; ++env)
            for (const auto& spec : kEnvelopeFieldSpecs)
                addParam(params, envelopeParamId(env, spec.idSuffix), spec);

        addParam(params, kLayerGainId, ParamFieldSpec{"", "Layer Gain", 0.0f, 4.0f, 1.0f, false});
        addParam(params, kLayerPanId, ParamFieldSpec{"", "Layer Pan", -1.0f, 1.0f, 0.0f, false});
        addParam(params, kMasterGainId, ParamFieldSpec{"", "Master Gain", 0.0f, 4.0f, 1.0f, false});

        for (std::size_t slot = 0; slot < kNumInsertFxSlots; ++slot)
            for (const auto& spec : kEffectSlotFieldSpecs)
                addParam(params, insertFxParamId(slot, spec.idSuffix), spec);

        for (std::size_t slot = 0; slot < kNumMasterFxSlots; ++slot)
            for (const auto& spec : kEffectSlotFieldSpecs)
                addParam(params, masterFxParamId(slot, spec.idSuffix), spec);

        for (const auto& spec : kArpFieldSpecs)
            addParam(params, juce::String(kArpIdPrefix) + spec.idSuffix, spec);

        return {params.begin(), params.end()};
    }

} // namespace pw8::plugin
