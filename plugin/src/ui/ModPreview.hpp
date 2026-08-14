#pragma once

#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>

#include <juce_audio_processors/juce_audio_processors.h>

#include "pw8/modulation/ModMatrixExecutor.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    [[nodiscard]] inline float modOffsetForDestination(const modulation::ModOutputs& out,
                                                       modulation::ModDestination destination,
                                                       std::uint8_t targetIndex) noexcept
    {
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff: return out.filterCutoffSemitones;
            case modulation::ModDestination::FilterResonance: return out.filterResonanceOffset;
            case modulation::ModDestination::OperatorFilterCutoff:
                return out.operatorFilterCutoffSemitones[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorFilterResonance:
                return out.operatorFilterResonanceOffset[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorLevel:
                return out.operatorLevelMultiplier[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::Pan: return out.panOffset;
            case modulation::ModDestination::OperatorWavetablePosition:
                return out.operatorWavetablePositionOffset[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorWavetableBend:
                return out.operatorWavetableBendOffset[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorWavetableAsymmetry:
                return out.operatorWavetableAsymmetryOffset[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorWavetableSyncRatio:
                return out.operatorWavetableSyncRatioOffset[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorWavetableFormant:
                return out.operatorWavetableFormantOffset[targetIndex < 8 ? targetIndex : 0];
            case modulation::ModDestination::OperatorWavetableSyncAmount:
                return out.operatorWavetableSyncAmountOffset[targetIndex < 8 ? targetIndex : 0];
            default: return 0.0f;
        }
    }

    [[nodiscard]] inline float modulatedParamValue(modulation::ModDestination destination, float baseValue,
                                                   float offset) noexcept
    {
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff:
            case modulation::ModDestination::OperatorFilterCutoff:
                return baseValue * std::pow(2.0f, offset / 12.0f);
            case modulation::ModDestination::OperatorLevel:
                return baseValue * offset;
            case modulation::ModDestination::FilterResonance:
            case modulation::ModDestination::OperatorFilterResonance:
            case modulation::ModDestination::Pan:
            case modulation::ModDestination::OperatorWavetablePosition:
            case modulation::ModDestination::OperatorWavetableBend:
            case modulation::ModDestination::OperatorWavetableAsymmetry:
            case modulation::ModDestination::OperatorWavetableFormant:
            case modulation::ModDestination::OperatorWavetableSyncAmount:
                return baseValue + offset;
            case modulation::ModDestination::OperatorWavetableSyncRatio:
                return baseValue + offset;
            default:
                return baseValue;
        }
    }

    [[nodiscard]] inline float normalizedModulatedValue(juce::RangedAudioParameter* param, float baseValue,
                                                        modulation::ModDestination destination, float offset) noexcept
    {
        if (param == nullptr)
            return 0.0f;

        const float modulated = modulatedParamValue(destination, baseValue, offset);
        const auto range = param->getNormalisableRange();
        return juce::jlimit(0.0f, 1.0f, param->convertTo0to1(juce::jlimit(range.start, range.end, modulated)));
    }

} // namespace pw8::plugin::ui