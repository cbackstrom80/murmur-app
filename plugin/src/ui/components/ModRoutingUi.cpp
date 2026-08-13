#include "ModRoutingUi.h"

#include "ModSourceChip.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        bool isDefaultMacroName(const patch::Macro& macro, std::size_t index)
        {
            if (macro.name.empty())
                return true;
            return macro.name == kMacroParameterNames[index];
        }

        bool macroHasActiveRoute(const patch::Patch& patch, std::size_t macroIndex)
        {
            const auto source = static_cast<modulation::ModSource>(static_cast<int>(modulation::ModSource::Macro1) +
                                                                   static_cast<int>(macroIndex));
            for (const auto& route : patch.layerA.modRoutes)
            {
                if (route.isActive() && route.source == source)
                    return true;
            }
            return false;
        }
    } // namespace

    float defaultModAmountFor(modulation::ModDestination destination) noexcept
    {
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff: return 36.0f;
            case modulation::ModDestination::FilterResonance: return 0.4f;
            case modulation::ModDestination::OperatorFilterCutoff: return 36.0f;
            case modulation::ModDestination::OperatorFilterResonance: return 0.4f;
            case modulation::ModDestination::OperatorLevel: return 0.35f;
            case modulation::ModDestination::OperatorWavetablePosition: return 0.25f;
            case modulation::ModDestination::Pan: return 0.4f;
            default: return 0.0f;
        }
    }

    std::optional<ModDestinationParam> modDestinationParam(modulation::ModDestination destination,
                                                           std::uint8_t targetIndex)
    {
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff:
                return ModDestinationParam{juce::String(kFilterIdPrefix) + "CutoffHz", "Filter Cutoff"};
            case modulation::ModDestination::FilterResonance:
                return ModDestinationParam{juce::String(kFilterIdPrefix) + "Resonance", "Filter Resonance"};
            case modulation::ModDestination::OperatorFilterCutoff:
                return ModDestinationParam{operatorFilterParamId(targetIndex, "FilterCutoffHz"),
                                           "Eng " + juce::String(static_cast<int>(targetIndex)) + " Cutoff"};
            case modulation::ModDestination::OperatorFilterResonance:
                return ModDestinationParam{operatorFilterParamId(targetIndex, "FilterResonance"),
                                           "Eng " + juce::String(static_cast<int>(targetIndex)) + " Resonance"};
            case modulation::ModDestination::OperatorLevel:
                return ModDestinationParam{operatorParamId(targetIndex, "Level"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " Level"};
            case modulation::ModDestination::OperatorWavetablePosition:
                return ModDestinationParam{operatorParamId(targetIndex, "WavetablePos"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " WT Pos"};
            case modulation::ModDestination::Pan:
                return ModDestinationParam{juce::String(kLayerPanId), "Layer Pan"};
            case modulation::ModDestination::None:
                break;
        }
        return std::nullopt;
    }

    ModAmountRange modAmountRangeFor(modulation::ModDestination destination) noexcept
    {
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff:
            case modulation::ModDestination::OperatorFilterCutoff:
                return {-72.0f, 72.0f};
            case modulation::ModDestination::FilterResonance:
            case modulation::ModDestination::OperatorFilterResonance:
                return {-1.0f, 1.0f};
            case modulation::ModDestination::OperatorLevel:
                return {-1.0f, 1.0f};
            case modulation::ModDestination::OperatorWavetablePosition:
                return {-1.0f, 1.0f};
            case modulation::ModDestination::Pan:
                return {-1.0f, 1.0f};
            default:
                return {-1.0f, 1.0f};
        }
    }

    void assignModRoute(PatchworkEightProcessor& processor, modulation::ModSource source,
                        modulation::ModDestination destination, std::uint8_t targetIndex)
    {
        assignModRoute(processor, source, destination, targetIndex, defaultModAmountFor(destination));
    }

    void assignModRoute(PatchworkEightProcessor& processor, modulation::ModSource source,
                        modulation::ModDestination destination, std::uint8_t targetIndex, float amount)
    {
        processor.setOrReplaceModRouteLive(source, destination, targetIndex, amount);
    }

    void updateModRouteAmount(PatchworkEightProcessor& processor, const modulation::ModRoute& route, float amount)
    {
        const auto range = modAmountRangeFor(route.destination);
        const float clamped = juce::jlimit(range.min, range.max, amount);
        processor.setOrReplaceModRouteLive(route.source, route.destination, route.targetIndex, clamped, route.scope);
    }

    std::vector<PatchFocusKnobSpec> inferPatchFocusKnobs(const patch::Patch& patch, std::size_t maxKnobs)
    {
        const auto cap = patch.uiFocus.knobs.empty()
                             ? maxKnobs
                             : juce::jmin(maxKnobs, patch.uiFocus.maxKnobs);

        if (!patch.uiFocus.knobs.empty())
        {
            std::vector<PatchFocusKnobSpec> authored;
            authored.reserve(cap);
            for (const auto& entry : patch.uiFocus.knobs)
            {
                if (authored.size() >= cap)
                    break;
                if (entry.kind == patch::UiFocusKnobKind::Macro)
                {
                    if (entry.macroIndex >= patch.macros.size())
                        continue;
                    const auto& macro = patch.macros[entry.macroIndex];
                    const auto label = entry.label.empty()
                                           ? (macro.name.empty() ? juce::String(kMacroParameterNames[entry.macroIndex])
                                                                 : juce::String(macro.name))
                                           : juce::String(entry.label);
                    authored.push_back({PatchFocusKnobKind::Macro, entry.macroIndex, {}, label});
                }
                else if (entry.kind == patch::UiFocusKnobKind::Param && !entry.paramId.empty())
                {
                    const auto label =
                        entry.label.empty() ? juce::String(entry.paramId) : juce::String(entry.label);
                    authored.push_back({PatchFocusKnobKind::ApvtsParam, 0, juce::String(entry.paramId), label});
                }
            }
            if (!authored.empty())
                return authored;
        }

        std::vector<PatchFocusKnobSpec> specs;
        specs.reserve(cap);
        juce::StringArray seenParamIds;

        auto pushMacro = [&](std::size_t index) {
            if (specs.size() >= cap)
                return;
            for (const auto& existing : specs)
            {
                if (existing.kind == PatchFocusKnobKind::Macro && existing.macroIndex == index)
                    return;
            }
            const auto& macro = patch.macros[index];
            const auto label =
                macro.name.empty() ? juce::String(kMacroParameterNames[index]) : juce::String(macro.name);
            specs.push_back({PatchFocusKnobKind::Macro, index, {}, label});
        };

        auto pushParam = [&](const juce::String& paramId, const juce::String& label) {
            if (specs.size() >= cap || paramId.isEmpty() || seenParamIds.contains(paramId))
                return;
            seenParamIds.add(paramId);
            specs.push_back({PatchFocusKnobKind::ApvtsParam, 0, paramId, label});
        };

        for (std::size_t i = 0; i < patch.macros.size(); ++i)
        {
            if (!isDefaultMacroName(patch.macros[i], i) || macroHasActiveRoute(patch, i))
                pushMacro(i);
        }

        for (const auto& route : patch.layerA.modRoutes)
        {
            if (!route.isActive())
                continue;
            if (const auto param = modDestinationParam(route.destination, route.targetIndex))
                pushParam(param->paramId, param->label);
        }

        if (specs.empty())
        {
            for (std::size_t i = 0; i < 4 && i < patch.macros.size(); ++i)
                pushMacro(i);
        }

        if (specs.size() > cap)
            specs.resize(cap);
        return specs;
    }

    juce::String formatModRouteAmount(modulation::ModDestination destination, float amount)
    {
        const auto sign = amount >= 0.0f ? "+" : "";
        switch (destination)
        {
            case modulation::ModDestination::FilterCutoff:
            case modulation::ModDestination::OperatorFilterCutoff:
                return sign + juce::String(amount, 1) + " st";
            case modulation::ModDestination::FilterResonance:
            case modulation::ModDestination::OperatorFilterResonance:
                return sign + juce::String(amount, 2);
            default:
                return sign + juce::String(amount, 2);
        }
    }

} // namespace pw8::plugin::ui
