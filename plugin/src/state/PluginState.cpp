#include "state/PluginState.h"

namespace pw8::plugin
{
    // clang-format off

    // Matches op::OperatorParams: engine, classic.waveform, classic.morph,
    // classic.pulseWidth, wavetableFramePosition, frequencyRatio, fixedFrequencyHz,
    // keyTrack, level.
    const std::array<ParamFieldSpec, kNumOperatorFields> kOperatorFieldSpecs = {{
        {"Engine",       "Engine",        0.0f,   7.0f, 0.0f,   true},
        {"Waveform",     "Waveform",      0.0f,   3.0f, 2.0f,   true},
        {"Morph",        "Morph",        -1.0f,   1.0f, -1.0f,  false},
        {"PulseWidth",   "Pulse Width",   0.01f,  0.99f, 0.5f,  false},
        {"WavetablePos", "Wavetable Pos", 0.0f,   1.0f, 0.0f,   false},
        {"FreqRatio",    "Freq Ratio",    0.001f, 128.0f, 1.0f, false},
        {"FixedHz",      "Fixed Hz",      0.01f,  24000.0f, 440.0f, false},
        {"KeyTrack",     "Key Track",     0.0f,   1.0f, 1.0f,   true},
        {"Level",        "Level",         0.0f,   4.0f, 1.0f,   false},
    }};

    // Matches filter::FilterParams: enabled, mode, cutoffHz, resonance, keyTrack.
    const std::array<ParamFieldSpec, kNumFilterFields> kFilterFieldSpecs = {{
        {"Enabled",   "Enabled",    0.0f,  1.0f,     0.0f,    true},
        {"Mode",      "Mode",       0.0f,  4.0f,     0.0f,    true},
        {"CutoffHz",  "Cutoff Hz",  10.0f, 24000.0f, 8000.0f, false},
        {"Resonance", "Resonance",  0.0f,  1.0f,     0.2f,    false},
        {"KeyTrack",  "Key Track", -1.0f,  1.0f,     0.0f,    false},
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
        {"Type",               "Type",                 0.0f,     6.0f,     0.0f,   true},
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
        params.reserve(8 + kNumFilterFields + kNumLfos * kNumLfoFields + kNumOperators * kNumOperatorFields +
                       kNumEnvelopes * kNumEnvelopeFields + 3 +
                       (kNumInsertFxSlots + kNumMasterFxSlots) * kNumEffectSlotFields + kNumArpFields);

        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            params.push_back(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID{kMacroParameterIds[i], 1}, kMacroParameterNames[i],
                juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));

        for (const auto& spec : kFilterFieldSpecs)
            addParam(params, juce::String(kFilterIdPrefix) + spec.idSuffix, spec);

        for (std::size_t lfo = 0; lfo < kNumLfos; ++lfo)
            for (const auto& spec : kLfoFieldSpecs)
                addParam(params, lfoParamId(lfo, spec.idSuffix), spec);

        for (std::size_t op = 0; op < kNumOperators; ++op)
            for (const auto& spec : kOperatorFieldSpecs)
                addParam(params, operatorParamId(op, spec.idSuffix), spec);

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
