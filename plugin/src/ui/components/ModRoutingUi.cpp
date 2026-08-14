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

        [[nodiscard]] std::size_t featureKoinCap(std::size_t maxKnobs) noexcept
        {
            return static_cast<std::size_t>(
                juce::jlimit(static_cast<int>(kMinFeatureKoinCount), static_cast<int>(kMaxFeatureKoinCount),
                             static_cast<int>(maxKnobs)));
        }

        void pushMacroKnob(std::vector<PatchFocusKnobSpec>& specs, const patch::Patch& patch, std::size_t macroIndex,
                           std::size_t cap)
        {
            if (specs.size() >= cap || macroIndex >= patch.macros.size())
                return;
            for (const auto& existing : specs)
            {
                if (existing.macroIndex == macroIndex)
                    return;
            }
            const auto& macro = patch.macros[macroIndex];
            const auto label =
                macro.name.empty() ? juce::String(kMacroParameterNames[macroIndex]) : juce::String(macro.name);
            specs.push_back({PatchFocusKnobKind::Macro, macroIndex, {}, label});
        }

        void pushMacroIfRouted(std::vector<PatchFocusKnobSpec>& specs, const patch::Patch& patch,
                               std::size_t macroIndex, std::size_t cap)
        {
            if (!macroHasActiveRoute(patch, macroIndex))
                return;
            pushMacroKnob(specs, patch, macroIndex, cap);
        }

        void ensureMinimumFeatureKnobs(std::vector<PatchFocusKnobSpec>& specs, const patch::Patch& patch,
                                       std::size_t cap)
        {
            if (specs.size() >= kMinFeatureKoinCount)
                return;

            for (std::size_t i = 0; i < patch.macros.size() && specs.size() < cap; ++i)
                pushMacroIfRouted(specs, patch, i, cap);

            if (specs.size() < kMinFeatureKoinCount)
                pushMacroIfRouted(specs, patch, 0, cap);
        }

        void appendStandardParamKnobs(std::vector<PatchFocusKnobSpec>& specs, std::size_t cap,
                                      const juce::AudioProcessorValueTreeState* apvtsForValidation)
        {
            const auto apvtsHasParam = [&](const juce::String& paramId) {
                return apvtsForValidation == nullptr || apvtsForValidation->getParameter(paramId) != nullptr;
            };

            juce::StringArray seenParamIds;
            for (const auto& existing : specs)
            {
                if (existing.kind == PatchFocusKnobKind::ApvtsParam)
                    seenParamIds.add(existing.paramId);
            }

            static constexpr const char* kStandardParams[][2] = {
                {"filterCutoffHz", "Cutoff"},
                {"filterResonance", "Reso"},
                {"layerGain", "Layer"},
                {"layerPan", "Pan"},
                {"masterGain", "Master"},
            };

            for (const auto& pad : kStandardParams)
            {
                if (specs.size() >= cap)
                    break;
                const juce::String paramId = pad[0];
                if (seenParamIds.contains(paramId) || !apvtsHasParam(paramId))
                    continue;
                seenParamIds.add(paramId);
                specs.push_back({PatchFocusKnobKind::ApvtsParam, 0, paramId, juce::String(pad[1])});
            }
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

    PatchFocusLayout inferPatchFocusLayout(const patch::Patch& patch, std::size_t maxFeatureKnobs,
                                           std::size_t maxStandardKnobs,
                                           const juce::AudioProcessorValueTreeState* apvtsForValidation)
    {
        const std::size_t featureCap = featureKoinCap(maxFeatureKnobs);
        std::vector<PatchFocusKnobSpec> feature;
        feature.reserve(featureCap);

        if (!patch.uiFocus.knobs.empty())
        {
            const std::size_t authoredCap =
                juce::jmin(featureCap, patch.uiFocus.maxKnobs > 0 ? patch.uiFocus.maxKnobs : featureCap);
            for (const auto& entry : patch.uiFocus.knobs)
            {
                if (feature.size() >= authoredCap)
                    break;
                if (entry.kind != patch::UiFocusKnobKind::Macro || entry.macroIndex >= patch.macros.size())
                    continue;
                if (!macroHasActiveRoute(patch, entry.macroIndex))
                    continue;

                bool duplicate = false;
                for (const auto& existing : feature)
                {
                    if (existing.macroIndex == entry.macroIndex)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (duplicate)
                    continue;

                const auto& macro = patch.macros[entry.macroIndex];
                const auto label = entry.label.empty()
                                       ? (macro.name.empty() ? juce::String(kMacroParameterNames[entry.macroIndex])
                                                             : juce::String(macro.name))
                                       : juce::String(entry.label);
                feature.push_back({PatchFocusKnobKind::Macro, entry.macroIndex, {}, label});
            }
        }

        if (feature.empty())
        {
            for (std::size_t i = 0; i < 3 && feature.size() < featureCap; ++i)
                pushMacroIfRouted(feature, patch, i, featureCap);
            for (std::size_t i = 3; i < patch.macros.size() && feature.size() < featureCap; ++i)
                pushMacroIfRouted(feature, patch, i, featureCap);
        }

        ensureMinimumFeatureKnobs(feature, patch, featureCap);

        std::vector<PatchFocusKnobSpec> standard;
        standard.reserve(maxStandardKnobs);
        appendStandardParamKnobs(standard, maxStandardKnobs, apvtsForValidation);

        return {std::move(feature), std::move(standard)};
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
