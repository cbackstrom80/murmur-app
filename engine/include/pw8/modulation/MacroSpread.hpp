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
            case ModDestination::QuasarQsr1Distance: return "Q1 Dist";
            case ModDestination::QuasarQsr2Distance: return "Q2 Dist";
            case ModDestination::QuasarQsr1Angle: return "Q1 Ang";
            case ModDestination::QuasarQsr2Angle: return "Q2 Ang";
            case ModDestination::QuasarQsr1Height: return "Q1 Hgt";
            case ModDestination::QuasarQsr2Height: return "Q2 Hgt";
            case ModDestination::QuasarRoomAmount: return "Q Room";
            case ModDestination::QuasarDelayFeedback: return "Q Dly Fb";
            case ModDestination::QuasarDelayTime: return "Q Dly T";
            case ModDestination::QuasarCntrLevel: return "CNTR";
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
