#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "pw8/modulation/ModMatrixTypes.hpp"

// PoliMATHS-inspired "Spread" labeling: one macro knob → many weighted mod destinations.
// Used by PLAY UI hints and agentic preset introspection (see docs/MAKE_NOISE_POLIMATHS_RESEARCH.md).

namespace pw8::modulation
{
    /// Compact destination label for spread summaries (e.g. "Filter", "WT", "Pan").
    [[nodiscard]] inline std::string modDestinationShortName(ModDestination destination,
                                                             std::uint8_t targetIndex) noexcept
    {
        switch (destination)
        {
            case ModDestination::FilterCutoff: return "Filter";
            case ModDestination::FilterResonance: return "Reso";
            case ModDestination::OperatorFilterCutoff:
                return targetIndex == 0 ? "Filter" : "Filter " + std::to_string(static_cast<int>(targetIndex));
            case ModDestination::OperatorFilterResonance:
                return targetIndex == 0 ? "Reso" : "Reso " + std::to_string(static_cast<int>(targetIndex));
            case ModDestination::OperatorLevel:
                return targetIndex == 0 ? "Level" : "Level " + std::to_string(static_cast<int>(targetIndex));
            case ModDestination::Pan: return "Pan";
            case ModDestination::OperatorWavetablePosition: return "WT";
            case ModDestination::OperatorWavetableBend: return "Bend";
            case ModDestination::OperatorWavetableAsymmetry: return "Asym";
            case ModDestination::OperatorWavetableSyncRatio: return "Sync";
            case ModDestination::OperatorWavetableFormant: return "Formant";
            case ModDestination::OperatorWavetableSyncAmount: return "Sync Amt";
            case ModDestination::MasterFxMix: return "MFX Mix";
            case ModDestination::MasterReverbMix: return "Rev Mix";
            case ModDestination::MasterReverbSize: return "Rev Size";
            case ModDestination::MasterReverbDecay: return "Rev Decay";
            case ModDestination::MasterReverbPreDelay: return "Rev Pre";
            case ModDestination::MasterReverbDiffusion: return "Rev Diff";
            case ModDestination::MasterReverbModDepth: return "Rev Mod";
            case ModDestination::MasterGain: return "Master";
            case ModDestination::VocoderMix: return "Vox Mix";
            case ModDestination::VocoderFormant: return "Vox Form";
            case ModDestination::ModRouteDepth: return "Route Depth";
            case ModDestination::MorphPosition: return "Morph";
            case ModDestination::QuasarQsr1Angle: return "QSR1 Az";
            case ModDestination::QuasarQsr2Angle: return "QSR2 Az";
            case ModDestination::QuasarRoomAmount: return "QSR Room";
            case ModDestination::QuasarCrossfeed: return "QSR Xfeed";
            case ModDestination::QuasarDelayVolume: return "QSR Delay";
            case ModDestination::QuasarQsr1Distance: return "QSR1 Dist";
            case ModDestination::QuasarQsr2Distance: return "QSR2 Dist";
            case ModDestination::QuasarDelayTime: return "QSR Dly T";
            case ModDestination::QuasarDelayFeedback: return "QSR Fdbk";
            case ModDestination::QuasarQsr1Height: return "QSR1 Ht";
            case ModDestination::QuasarQsr2Height: return "QSR2 Ht";
            case ModDestination::QuasarCntrLevel: return "QSR CNTR";
            case ModDestination::QuasarQsr1Level: return "QSR1 Lvl";
            case ModDestination::QuasarQsr2Level: return "QSR2 Lvl";
            case ModDestination::OperatorFmModulatorRatio: return "FM Ratio";
            case ModDestination::OperatorFmModulatorIndex: return "FM Index";
            case ModDestination::OperatorFmModulatorFeedback: return "FM Fdbk";
            case ModDestination::OperatorFreqRatio: return "Ratio";
            case ModDestination::OperatorPhaseBend: return "Ph Bend";
            case ModDestination::OperatorPhaseFold: return "Ph Fold";
            case ModDestination::OperatorPhaseAsymmetry: return "Ph Asym";
            case ModDestination::OperatorAdditivePartialCount: return "Partials";
            case ModDestination::OperatorAdditiveTilt: return "Tilt";
            case ModDestination::OperatorAdditiveOddEven: return "Odd/Even";
            case ModDestination::OperatorAdditiveStretch: return "Stretch";
            case ModDestination::OperatorResonatorStructure: return "Res Str";
            case ModDestination::OperatorResonatorDecay: return "Res Dec";
            case ModDestination::OperatorResonatorDamping: return "Res Damp";
            case ModDestination::OperatorResonatorBrightness: return "Res Bright";
            case ModDestination::OperatorResonatorModeCount: return "Res Modes";
            case ModDestination::OperatorGrainDensity: return "Grain Dens";
            case ModDestination::OperatorGrainSizeMs: return "Grain Size";
            case ModDestination::OperatorGrainPositionJitter: return "Pos Jit";
            case ModDestination::OperatorGrainPitchJitter: return "Pit Jit";
            case ModDestination::UnisonVoices: return "Unison V";
            case ModDestination::UnisonDetune: return "Unison Det";
            case ModDestination::UnisonSpread: return "Unison W";
            case ModDestination::None: break;
        }
        return "-";
    }

    template <typename RouteContainer>
    [[nodiscard]] std::string spreadSummaryForMacro(std::size_t macroIndex, const RouteContainer& modRoutes)
    {
        if (macroIndex >= 8)
            return {};

        const auto source = static_cast<ModSource>(static_cast<int>(ModSource::Macro1) + static_cast<int>(macroIndex));

        std::vector<std::string> labels;
        labels.reserve(4);

        for (const auto& route : modRoutes)
        {
            if (!route.isActive() || route.source != source)
                continue;

            const auto label = modDestinationShortName(route.destination, route.targetIndex);
            bool duplicate = false;
            for (const auto& existing : labels)
            {
                if (existing == label)
                {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
                labels.push_back(label);
        }

        std::string result;
        for (std::size_t i = 0; i < labels.size(); ++i)
        {
            if (i > 0)
                result += ", ";
            result += labels[i];
        }
        return result;
    }

    template <typename RouteContainer>
    [[nodiscard]] std::size_t spreadDestinationCountForMacro(std::size_t macroIndex, const RouteContainer& modRoutes)
    {
        if (macroIndex >= 8)
            return 0;

        const auto source = static_cast<ModSource>(static_cast<int>(ModSource::Macro1) + static_cast<int>(macroIndex));

        std::vector<std::string> labels;
        for (const auto& route : modRoutes)
        {
            if (!route.isActive() || route.source != source)
                continue;

            const auto label = modDestinationShortName(route.destination, route.targetIndex);
            bool duplicate = false;
            for (const auto& existing : labels)
            {
                if (existing == label)
                {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
                labels.push_back(label);
        }
        return labels.size();
    }

} // namespace pw8::modulation
