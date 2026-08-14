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
