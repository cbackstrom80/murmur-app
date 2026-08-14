#include "ModRoutingUi.h"

#include "ModSourceChip.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
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

        void padPatchFocusKnobs(std::vector<PatchFocusKnobSpec>& specs, const patch::Patch& patch, std::size_t targetCount,
                                const juce::AudioProcessorValueTreeState* apvtsForValidation)
        {
            if (specs.size() >= targetCount)
            {
                specs.resize(targetCount);
                return;
            }

            const auto apvtsHasParam = [&](const juce::String& paramId) {
                return apvtsForValidation == nullptr || apvtsForValidation->getParameter(paramId) != nullptr;
            };

            juce::StringArray seenParamIds;
            for (const auto& existing : specs)
            {
                if (existing.kind == PatchFocusKnobKind::ApvtsParam)
                    seenParamIds.add(existing.paramId);
            }

            auto pushMacro = [&](std::size_t index) {
                if (specs.size() >= targetCount)
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
                if (specs.size() >= targetCount || paramId.isEmpty() || seenParamIds.contains(paramId) ||
                    !apvtsHasParam(paramId))
                    return;
                seenParamIds.add(paramId);
                specs.push_back({PatchFocusKnobKind::ApvtsParam, 0, paramId, label});
            };

            for (std::size_t i = 0; i < patch.macros.size() && specs.size() < targetCount; ++i)
            {
                if (macroHasActiveRoute(patch, i))
                    pushMacro(i);
            }

            static constexpr const char* kPadParams[][2] = {
                {"filterCutoffHz", "Cutoff"},
                {"filterResonance", "Reso"},
                {"layerGain", "Layer"},
                {"layerPan", "Pan"},
                {"masterGain", "Master"},
                {"lfo0RateHz", "LFO Rate"},
            };
            for (const auto& pad : kPadParams)
                pushParam(juce::String(pad[0]), juce::String(pad[1]));

            for (std::size_t i = 0; i < patch.macros.size() && specs.size() < targetCount; ++i)
                pushMacro(i);
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
            case modulation::ModDestination::OperatorWavetableBend: return 0.35f;
            case modulation::ModDestination::OperatorWavetableAsymmetry: return 0.35f;
            case modulation::ModDestination::OperatorWavetableSyncRatio: return 2.0f;
            case modulation::ModDestination::OperatorWavetableFormant: return 0.35f;
            case modulation::ModDestination::OperatorWavetableSyncAmount: return 0.35f;
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
            case modulation::ModDestination::OperatorWavetableBend:
                return ModDestinationParam{operatorParamId(targetIndex, "WtBend"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " WT Bend"};
            case modulation::ModDestination::OperatorWavetableAsymmetry:
                return ModDestinationParam{operatorParamId(targetIndex, "WtAsymmetry"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " WT Asym"};
            case modulation::ModDestination::OperatorWavetableSyncRatio:
                return ModDestinationParam{operatorParamId(targetIndex, "WtSyncRatio"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " WT Sync Ratio"};
            case modulation::ModDestination::OperatorWavetableFormant:
                return ModDestinationParam{operatorParamId(targetIndex, "WtFormantShift"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " WT Formant"};
            case modulation::ModDestination::OperatorWavetableSyncAmount:
                return ModDestinationParam{operatorParamId(targetIndex, "WtSyncAmount"),
                                           "Op " + juce::String(static_cast<int>(targetIndex)) + " WT Sync Amt"};
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
            case modulation::ModDestination::OperatorWavetableBend:
            case modulation::ModDestination::OperatorWavetableAsymmetry:
            case modulation::ModDestination::OperatorWavetableFormant:
                return {-1.0f, 1.0f};
            case modulation::ModDestination::OperatorWavetableSyncRatio:
                return {-8.0f, 8.0f};
            case modulation::ModDestination::OperatorWavetableSyncAmount:
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
        float amount = defaultModAmountFor(destination);
        for (const auto& route : processor.getCurrentPatch().layerA.modRoutes)
        {
            if (route.isActive() && route.destination == destination && route.targetIndex == targetIndex)
            {
                amount = route.amount;
                break;
            }
        }
        assignModRoute(processor, source, destination, targetIndex, amount);
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

    std::vector<PatchFocusKnobSpec> inferPatchFocusKnobs(const patch::Patch& patch, std::size_t maxKnobs,
                                                         const juce::AudioProcessorValueTreeState* apvtsForValidation)
    {
        const std::size_t requestedCap = patch.uiFocus.knobs.empty()
                                             ? maxKnobs
                                             : juce::jmin(maxKnobs, patch.uiFocus.maxKnobs);
        const std::size_t cap = juce::jmax(requestedCap, juce::jmin(maxKnobs, kMinimumKoinCount));

        const auto apvtsHasParam = [&](const juce::String& paramId) {
            return apvtsForValidation == nullptr || apvtsForValidation->getParameter(paramId) != nullptr;
        };

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
                    const auto paramId = juce::String(entry.paramId);
                    if (!apvtsHasParam(paramId))
                        continue;
                    const auto label = entry.label.empty() ? paramId : juce::String(entry.label);
                    authored.push_back({PatchFocusKnobKind::ApvtsParam, 0, paramId, label});
                }
            }
            padPatchFocusKnobs(authored, patch, cap, apvtsForValidation);
            if (!authored.empty())
                return authored;
        }

        std::vector<PatchFocusKnobSpec> specs;
        specs.reserve(cap);
        juce::StringArray seenParamIds;

        auto pushMacro = [&](std::size_t index) {
            if (specs.size() >= cap || !macroHasActiveRoute(patch, index))
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
            if (specs.size() >= cap || paramId.isEmpty() || seenParamIds.contains(paramId) ||
                !apvtsHasParam(paramId))
                return;
            seenParamIds.add(paramId);
            specs.push_back({PatchFocusKnobKind::ApvtsParam, 0, paramId, label});
        };

        for (std::size_t i = 0; i < patch.macros.size(); ++i)
        {
            if (macroHasActiveRoute(patch, i))
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
            pushParam(juce::String(kFilterIdPrefix) + "CutoffHz", "Filter Cutoff");
            pushParam(juce::String(kFilterIdPrefix) + "Resonance", "Filter Resonance");
        }

        padPatchFocusKnobs(specs, patch, cap, apvtsForValidation);
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

    std::optional<modulation::ModRoute> findModWheelRoute(const patch::Patch& patch) noexcept
    {
        for (const auto& route : patch.layerA.modRoutes)
        {
            if (route.source == modulation::ModSource::ModWheel && route.isActive())
                return route;
        }
        return std::nullopt;
    }

    juce::String formatModWheelStatus(const patch::Patch& patch, float modWheelValue01) noexcept
    {
        const auto route = findModWheelRoute(patch);
        if (!route.has_value())
            return {};

        const auto dest = modDestinationLabel(route->destination, route->targetIndex);
        const auto depth = formatModRouteAmount(route->destination, route->amount);
        const int pct = juce::roundToInt(juce::jlimit(0.0f, 1.0f, modWheelValue01) * 100.0f);
        return "Mod Wheel (CC1)  " + juce::String(pct) + "%  ->  " + dest + "  (" + depth + " max)";
    }

    std::optional<modulation::ModRoute> findExpressionRoute(const patch::Patch& patch) noexcept
    {
        for (const auto& route : patch.layerA.modRoutes)
        {
            if (route.source == modulation::ModSource::Expression && route.isActive())
                return route;
        }
        return std::nullopt;
    }

    juce::String formatExpressionStatus(const patch::Patch& patch, float expressionValue01) noexcept
    {
        const auto route = findExpressionRoute(patch);
        if (!route.has_value())
            return {};

        const auto dest = modDestinationLabel(route->destination, route->targetIndex);
        const auto depth = formatModRouteAmount(route->destination, route->amount);
        const int pct = juce::roundToInt(juce::jlimit(0.0f, 1.0f, expressionValue01) * 100.0f);
        return "Expression (CC11)  " + juce::String(pct) + "%  ->  " + dest + "  (" + depth + " max)";
    }

} // namespace pw8::plugin::ui
