#include "QuasarParamLayout.h"

namespace pw8::quasar
{
    const std::array<ParamFieldSpec, kNumQuasarParams> kQuasarParamSpecs = {{
        {"mix", "Mix", 0.0f, 1.0f, 1.0f, false},
        {"qsr1Level", "QSR1 Level", 0.0f, 1.0f, 0.65f, false},
        {"qsr2Level", "QSR2 Level", 0.0f, 1.0f, 0.55f, false},
        {"cntrLevel", "CNTR Level", 0.0f, 1.0f, 0.85f, false},
        {"inputSplitHpfHz", "Input Split HPF Hz", 20.0f, 500.0f, 120.0f, false},
        {"cntrHpfHz", "CNTR HPF Hz", 20.0f, 300.0f, 80.0f, false},
        {"qsr1Height", "QSR1 Height", -1.0f, 1.0f, 0.0f, false},
        {"qsr1Angle", "QSR1 Angle", 0.0f, 360.0f, 30.0f, false},
        {"qsr1Distance", "QSR1 Distance", 0.0f, 1.0f, 0.35f, false},
        {"qsr2Height", "QSR2 Height", -1.0f, 1.0f, 0.0f, false},
        {"qsr2Angle", "QSR2 Angle", 0.0f, 360.0f, 330.0f, false},
        {"qsr2Distance", "QSR2 Distance", 0.0f, 1.0f, 0.40f, false},
        {"qsr1RoomAmount", "QSR1 Room Amount", 0.0f, 1.0f, 0.45f, false},
        {"qsr1RoomSize", "QSR1 Room Size", 0.2f, 3.0f, 1.0f, false},
        {"qsr1RoomDamping", "QSR1 Room Damping", 0.0f, 1.0f, 0.55f, false},
        {"qsr2RoomAmount", "QSR2 Room Amount", 0.0f, 1.0f, 0.40f, false},
        {"qsr2RoomSize", "QSR2 Room Size", 0.2f, 3.0f, 1.1f, false},
        {"qsr2RoomDamping", "QSR2 Room Damping", 0.0f, 1.0f, 0.50f, false},
        {"quasarDelayTimeMs", "Delay Time Ms", 3.0f, 20000.0f, 450.0f, false},
        {"quasarDelayFeedback", "Delay Feedback", 0.0f, 1.0f, 0.35f, false},
        {"quasarDelayVolume", "Delay Volume", 0.0f, 1.0f, 0.25f, false},
        {"quasarOutputMode", "Output Mode", 0.0f, 2.0f, 0.0f, true},
        {"quasarCrossfeed", "Crossfeed", 0.0f, 1.0f, 0.0f, false},
        {"quasarDelaySync", "Delay Sync", 0.0f, 1.0f, 0.0f, true},
        {"quasarDelaySyncDivision", "Delay Sync Division", 0.0f, 8.0f, 2.0f, true},
    }};

    namespace
    {
        void addParam(std::vector<std::unique_ptr<juce::RangedAudioParameter>>& params, const ParamFieldSpec& spec)
        {
            const float interval = spec.discrete ? 1.0f : 0.0f;
            const juce::NormalisableRange<float> range =
                interval > 0.0f ? juce::NormalisableRange<float>(spec.minValue, spec.maxValue, interval)
                                : juce::NormalisableRange<float>(spec.minValue, spec.maxValue);
            params.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{spec.id, 1}, spec.label,
                                                                           range, spec.defaultValue));
        }
    } // namespace

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
        params.reserve(kNumQuasarParams);
        for (const auto& spec : kQuasarParamSpecs)
            addParam(params, spec);
        return {params.begin(), params.end()};
    }

} // namespace pw8::quasar
