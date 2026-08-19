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
        const auto idx = targetIndex < 8 ? targetIndex : static_cast<std::uint8_t>(0);
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff: return out.filterCutoffSemitones;
            case modulation::ModDestination::FilterResonance: return out.filterResonanceOffset;
            case modulation::ModDestination::OperatorFilterCutoff: return out.operatorFilterCutoffSemitones[idx];
            case modulation::ModDestination::OperatorFilterResonance: return out.operatorFilterResonanceOffset[idx];
            case modulation::ModDestination::OperatorLevel: return out.operatorLevelMultiplier[idx];
            case modulation::ModDestination::Pan: return out.panOffset;
            case modulation::ModDestination::OperatorWavetablePosition:
                return out.operatorWavetablePositionOffset[idx];
            case modulation::ModDestination::OperatorWavetableBend: return out.operatorWavetableBendOffset[idx];
            case modulation::ModDestination::OperatorWavetableAsymmetry:
                return out.operatorWavetableAsymmetryOffset[idx];
            case modulation::ModDestination::OperatorWavetableSyncRatio:
                return out.operatorWavetableSyncRatioOffset[idx];
            case modulation::ModDestination::OperatorWavetableFormant: return out.operatorWavetableFormantOffset[idx];
            case modulation::ModDestination::OperatorWavetableSyncAmount:
                return out.operatorWavetableSyncAmountOffset[idx];
            case modulation::ModDestination::OperatorFmModulatorRatio:
                return out.operatorFmModulatorRatioOffset[idx];
            case modulation::ModDestination::OperatorFmModulatorIndex:
                return out.operatorFmModulatorIndexOffset[idx];
            case modulation::ModDestination::OperatorFmModulatorFeedback:
                return out.operatorFmModulatorFeedbackOffset[idx];
            case modulation::ModDestination::OperatorFreqRatio: return out.operatorFreqRatioOffset[idx];
            case modulation::ModDestination::OperatorPhaseBend: return out.operatorPhaseBendOffset[idx];
            case modulation::ModDestination::OperatorPhaseFold: return out.operatorPhaseFoldOffset[idx];
            case modulation::ModDestination::OperatorPhaseAsymmetry: return out.operatorPhaseAsymmetryOffset[idx];
            case modulation::ModDestination::OperatorAdditivePartialCount:
                return out.operatorAdditivePartialCountOffset[idx];
            case modulation::ModDestination::OperatorAdditiveTilt: return out.operatorAdditiveTiltOffset[idx];
            case modulation::ModDestination::OperatorAdditiveOddEven: return out.operatorAdditiveOddEvenOffset[idx];
            case modulation::ModDestination::OperatorAdditiveStretch: return out.operatorAdditiveStretchOffset[idx];
            case modulation::ModDestination::OperatorResonatorStructure: return out.operatorResonatorStructureOffset[idx];
            case modulation::ModDestination::OperatorResonatorDecay: return out.operatorResonatorDecayOffset[idx];
            case modulation::ModDestination::OperatorResonatorDamping: return out.operatorResonatorDampingOffset[idx];
            case modulation::ModDestination::OperatorResonatorBrightness:
                return out.operatorResonatorBrightnessOffset[idx];
            case modulation::ModDestination::OperatorResonatorModeCount:
                return out.operatorResonatorModeCountOffset[idx];
            case modulation::ModDestination::OperatorGrainDensity: return out.operatorGrainDensityOffset[idx];
            case modulation::ModDestination::OperatorGrainSizeMs: return out.operatorGrainSizeMsOffset[idx];
            case modulation::ModDestination::OperatorGrainPositionJitter:
                return out.operatorGrainPositionJitterOffset[idx];
            case modulation::ModDestination::OperatorGrainPitchJitter: return out.operatorGrainPitchJitterOffset[idx];
            case modulation::ModDestination::UnisonVoices: return out.unisonVoicesOffset;
            case modulation::ModDestination::UnisonDetune: return out.unisonDetuneOffset;
            case modulation::ModDestination::UnisonSpread: return out.unisonSpreadOffset;
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
            case modulation::ModDestination::OperatorFmModulatorRatio:
            case modulation::ModDestination::OperatorFmModulatorIndex:
            case modulation::ModDestination::OperatorFmModulatorFeedback:
            case modulation::ModDestination::OperatorFreqRatio:
            case modulation::ModDestination::OperatorPhaseBend:
            case modulation::ModDestination::OperatorPhaseFold:
            case modulation::ModDestination::OperatorPhaseAsymmetry:
            case modulation::ModDestination::OperatorAdditivePartialCount:
            case modulation::ModDestination::OperatorAdditiveTilt:
            case modulation::ModDestination::OperatorAdditiveOddEven:
            case modulation::ModDestination::OperatorAdditiveStretch:
            case modulation::ModDestination::OperatorResonatorStructure:
            case modulation::ModDestination::OperatorResonatorDecay:
            case modulation::ModDestination::OperatorResonatorDamping:
            case modulation::ModDestination::OperatorResonatorBrightness:
            case modulation::ModDestination::OperatorResonatorModeCount:
            case modulation::ModDestination::OperatorGrainDensity:
            case modulation::ModDestination::OperatorGrainSizeMs:
            case modulation::ModDestination::OperatorGrainPositionJitter:
            case modulation::ModDestination::OperatorGrainPitchJitter:
            case modulation::ModDestination::UnisonVoices:
            case modulation::ModDestination::UnisonDetune:
            case modulation::ModDestination::UnisonSpread:
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
