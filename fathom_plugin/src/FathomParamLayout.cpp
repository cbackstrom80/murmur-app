#include "FathomParamLayout.h"

namespace pw8::fathom
{
    // Index order matters -- FathomProcessor::cacheParameterPointers() relies
    // on it matching this exact array, same convention QuasarParamLayout.cpp
    // already uses.
    const std::array<ParamFieldSpec, kNumFathomParams> kFathomParamSpecs = {{
        // -- Mode --
        {"reverbMode", "Mode", 0.0f, 1.0f, 0.0f, true}, // 0=Algorithmic, 1=Convolution

        // -- Algorithmic (real pw8::effects::ReverbProcessor fields, real
        //    ranges from EffectTypes.hpp's own doc comments) --
        {"mix", "Mix", 0.0f, 1.0f, 1.0f, false},
        {"reverbSizeParam", "Size", 0.2f, 3.0f, 1.0f, false},
        {"reverbDecaySeconds", "Decay", 0.1f, 30.0f, 2.0f, false},
        {"reverbPreDelayMs", "Pre-Delay", 0.0f, 200.0f, 20.0f, false},
        {"reverbHighRatio", "HF RT Mult", 0.2f, 1.0f, 0.6f, false},
        {"reverbHighCrossoverHz", "HF Crossover", 500.0f, 15000.0f, 4500.0f, false},
        {"reverbLowRatio", "LF RT Mult", 0.2f, 4.0f, 1.3f, false},
        {"reverbLowCrossoverHz", "LF Crossover", 50.0f, 2000.0f, 400.0f, false},
        {"reverbDiffusion", "Diffusion", 0.0f, 1.0f, 0.65f, false},
        {"reverbDensity", "Density", 0.0f, 1.0f, 0.85f, false},
        {"reverbModDepth", "Mod Depth", 0.0f, 1.0f, 0.35f, false},
        {"reverbModRateHz", "Mod Rate", 0.05f, 2.0f, 0.4f, false},
        {"reverbEarlyLevel", "Early Level", 0.0f, 1.0f, 0.5f, false},
        {"reverbLateLevel", "Late Level", 0.0f, 1.0f, 1.0f, false},
        {"reverbRollOffHz", "Roll Off", 80.0f, 20000.0f, 12000.0f, false},
        {"reverbVlfCutDb", "VLF Cut", -18.0f, 0.0f, 0.0f, false},
        {"reverbCharacter", "Character", 0.0f, 4.0f, 0.0f, true}, // Default/Plate/Hall/Room/Spring

        // -- Convolution (new, real -- juce::dsp::Convolution over the real
        //    bundled IRs, see FathomIrLibrary.h) --
        {"irIndex", "Impulse", 0.0f, 37.0f, 0.0f, true}, // index into kBundledIrs
        {"convPreDelayMs", "Conv Pre-Delay", 0.0f, 200.0f, 0.0f, false},
        {"convMix", "Conv Mix", 0.0f, 1.0f, 1.0f, false},
        {"convWidth", "Conv Width", 0.0f, 2.0f, 1.0f, false}, // 0=mono .. 1=natural .. 2=extra wide
        {"convLowCutHz", "Conv Low Cut", 20.0f, 2000.0f, 20.0f, false},
        {"convHighCutHz", "Conv High Cut", 1000.0f, 20000.0f, 20000.0f, false},
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
        params.reserve(kNumFathomParams);
        for (const auto& spec : kFathomParamSpecs)
            addParam(params, spec);
        return {params.begin(), params.end()};
    }

} // namespace pw8::fathom
