#include "ModRoutingUi.h"

#include "ModSourceChip.h"
#include "pw8/modulation/MacroSpread.hpp"
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
            case modulation::ModDestination::MasterFxMix:
            case modulation::ModDestination::MasterReverbMix: return 0.35f;
            case modulation::ModDestination::MasterReverbSize: return 0.5f;
            case modulation::ModDestination::MasterReverbDecay: return 1.5f;
            case modulation::ModDestination::MasterReverbPreDelay: return 15.0f;
            case modulation::ModDestination::MasterReverbDiffusion: return 0.25f;
            case modulation::ModDestination::MasterReverbModDepth: return 0.2f;
            case modulation::ModDestination::MasterGain: return 0.25f;
            case modulation::ModDestination::QuasarQsr1Distance:
            case modulation::ModDestination::QuasarQsr2Distance: return 0.4f;
            case modulation::ModDestination::QuasarQsr1Angle:
            case modulation::ModDestination::QuasarQsr2Angle: return 90.0f;
            case modulation::ModDestination::QuasarQsr1Height:
            case modulation::ModDestination::QuasarQsr2Height: return 0.3f;
            case modulation::ModDestination::QuasarRoomAmount: return 0.35f;
            case modulation::ModDestination::QuasarDelayFeedback: return 0.3f;
            case modulation::ModDestination::QuasarDelayTime: return 200.0f;
            case modulation::ModDestination::QuasarCntrLevel: return 0.2f;
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
            case modulation::ModDestination::MasterFxMix:
            case modulation::ModDestination::MasterReverbMix:
                return ModDestinationParam{masterFxParamId(targetIndex, "Mix"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Mix"};
            case modulation::ModDestination::MasterReverbSize:
                return ModDestinationParam{masterFxParamId(targetIndex, "ReverbSize"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Rev Size"};
            case modulation::ModDestination::MasterReverbDecay:
                return ModDestinationParam{masterFxParamId(targetIndex, "ReverbDecaySeconds"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Rev Decay"};
            case modulation::ModDestination::MasterReverbPreDelay:
                return ModDestinationParam{masterFxParamId(targetIndex, "ReverbPreDelayMs"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Rev Pre"};
            case modulation::ModDestination::MasterReverbDiffusion:
                return ModDestinationParam{masterFxParamId(targetIndex, "ReverbDiffusion"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Rev Diff"};
            case modulation::ModDestination::MasterReverbModDepth:
                return ModDestinationParam{masterFxParamId(targetIndex, "ReverbModDepth"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Rev Mod"};
            case modulation::ModDestination::MasterGain:
                return ModDestinationParam{juce::String(kMasterGainId), "Master Gain"};
            case modulation::ModDestination::QuasarQsr1Distance:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr1Distance"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q1 Dist"};
            case modulation::ModDestination::QuasarQsr2Distance:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr2Distance"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q2 Dist"};
            case modulation::ModDestination::QuasarQsr1Angle:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr1Angle"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q1 Ang"};
            case modulation::ModDestination::QuasarQsr2Angle:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr2Angle"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q2 Ang"};
            case modulation::ModDestination::QuasarQsr1Height:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr1Height"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q1 Hgt"};
            case modulation::ModDestination::QuasarQsr2Height:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr2Height"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q2 Hgt"};
            case modulation::ModDestination::QuasarRoomAmount:
                return ModDestinationParam{masterFxParamId(targetIndex, "Qsr1RoomAmount"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q Room"};
            case modulation::ModDestination::QuasarDelayFeedback:
                return ModDestinationParam{masterFxParamId(targetIndex, "QuasarDelayFeedback"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q Dly Fb"};
            case modulation::ModDestination::QuasarDelayTime:
                return ModDestinationParam{masterFxParamId(targetIndex, "QuasarDelayTimeMs"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " Q Dly T"};
            case modulation::ModDestination::QuasarCntrLevel:
                return ModDestinationParam{masterFxParamId(targetIndex, "CntrLevel"),
                                           "Master " + juce::String(static_cast<int>(targetIndex)) + " CNTR"};
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
                if (entry.kind == patch::UiFocusKnobKind::Morph)
                {
                    if (patch.morphKoin.keyframes.size() < 2)
                        continue;
                    const auto label = entry.label.empty() ? juce::String(patch.morphKoin.label.c_str())
                                                           : juce::String(entry.label);
                    feature.push_back({PatchFocusKnobKind::Morph, 0, kMorphPositionId, label});
                    continue;
                }
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

    std::optional<modulation::ModRoute> findSidechainRoute(const patch::Patch& patch) noexcept
    {
        for (const auto& route : patch.layerA.modRoutes)
        {
            if (route.source == modulation::ModSource::Sidechain && route.isActive())
                return route;
        }
        return std::nullopt;
    }

    juce::String formatSidechainStatus(const patch::Patch& patch, float sidechainLevel01, bool sidechainActive,
                                       bool auSidechainAvailable) noexcept
    {
        const auto route = findSidechainRoute(patch);
        if (!route.has_value() && !sidechainActive && !auSidechainAvailable)
            return {};

        juce::String text = "Sidechain (AU)";
        if (route.has_value())
        {
            const auto dest = modDestinationLabel(route->destination, route->targetIndex);
            const auto depth = formatModRouteAmount(route->destination, route->amount);
            text += "  ->  " + dest + "  (" + depth + " max)";
        }
        else if (auSidechainAvailable)
        {
            text += "  — add SIDECHAIN route in MOD tab";
        }
        if (sidechainActive)
        {
            const int pct = juce::roundToInt(juce::jlimit(0.0f, 1.0f, sidechainLevel01) * 100.0f);
            text += "  " + juce::String(pct) + "%";
        }
        else if (route.has_value())
        {
            text += "  (pick bus in plugin header — no input)";
        }
        else if (auSidechainAvailable)
        {
            text += "  (pick bus in plugin header)";
        }
        return text;
    }

    juce::String formatFeatureMacroHints(const patch::Patch& patch, const std::vector<PatchFocusKnobSpec>& featureKnobs)
    {
        juce::StringArray parts;
        for (const auto& spec : featureKnobs)
        {
            if (spec.kind == PatchFocusKnobKind::Morph)
            {
                juce::String line = spec.label;
                if (!patch.morphKoin.description.empty())
                {
                    juce::String desc = juce::String::fromUTF8(patch.morphKoin.description.data(),
                                                               static_cast<int>(patch.morphKoin.description.size()))
                                            .trim();
                    if (desc.isNotEmpty())
                        line += ": " + desc;
                }
                if (patch.morphKoin.keyframes.size() >= 2)
                {
                    line += "  ·  " + juce::String(patch.morphKoin.keyframes.front().name.c_str()) + juce::String(
                                                                                                      L" \u2194 ") +
                            juce::String(patch.morphKoin.keyframes.back().name.c_str());
                }
                if (line.isNotEmpty())
                    parts.add(line);
                continue;
            }
            if (spec.kind != PatchFocusKnobKind::Macro || spec.macroIndex >= patch.macros.size())
                continue;

            const auto& macro = patch.macros[spec.macroIndex];
            const juce::String name = spec.label.isNotEmpty() ? spec.label
                                  : (macro.name.empty() ? juce::String(kMacroParameterNames[spec.macroIndex])
                                                        : juce::String(macro.name));

            const auto spread = modulation::spreadSummaryForMacro(spec.macroIndex, patch.layerA.modRoutes);
            const auto targetCount = modulation::spreadDestinationCountForMacro(spec.macroIndex, patch.layerA.modRoutes);

            juce::String line;
            if (!spread.empty())
            {
                line = name + juce::String::charToString(juce::juce_wchar(0x2192)) + " " + juce::String(spread);
                if (targetCount > 1)
                    line += " (" + juce::String(static_cast<int>(targetCount)) + " targets)";
            }

            if (macro.description.empty())
            {
                if (line.isNotEmpty())
                    parts.add(line);
                continue;
            }

            juce::String desc = juce::String::fromUTF8(macro.description.data(),
                                                      static_cast<int>(macro.description.size()))
                                    .trim();
            if (desc.isEmpty())
            {
                if (line.isNotEmpty())
                    parts.add(line);
                continue;
            }

            if (!desc.startsWithIgnoreCase(name))
                desc = name + ": " + desc;

            if (line.isNotEmpty())
                desc += "  ·  " + line;

            if (desc.length() > 72)
                desc = desc.substring(0, 69).trimEnd() + "...";
            parts.add(desc);
        }

        return parts.joinIntoString("  ·  ");
    }

    juce::String performanceHintForPatch(const patch::Patch& patch,
                                         const juce::AudioProcessorValueTreeState* apvtsForValidation)
    {
        const auto layout = inferPatchFocusLayout(patch, kMaxFeatureKoinCount, 0, apvtsForValidation);
        const auto macroHints = formatFeatureMacroHints(patch, layout.featureKnobs);
        if (macroHints.isNotEmpty())
            return macroHints;

        if (patch.metadata.description.empty())
            return {};

        juce::String text = juce::String::fromUTF8(patch.metadata.description.data(),
                                                   static_cast<int>(patch.metadata.description.size()))
                                .trim();
        int end = -1;
        for (int i = 0; i < text.length(); ++i)
        {
            const juce::juce_wchar c = text[i];
            if (c == '.' || c == '!' || c == '?')
            {
                end = i;
                break;
            }
        }

        juce::String hint = end >= 0 ? text.substring(0, end + 1).trim() : text;
        if (hint.length() > 80)
            hint = hint.substring(0, 77).trimEnd() + "...";
        return hint;
    }

    std::optional<std::pair<modulation::ModDestination, std::uint8_t>>
    findModDestinationForApvtsParam(const juce::String& paramId) noexcept
    {
        for (std::uint8_t op = 0; op < 8; ++op)
        {
            for (const auto dest :
                 {modulation::ModDestination::OperatorFilterCutoff, modulation::ModDestination::OperatorFilterResonance,
                  modulation::ModDestination::OperatorLevel, modulation::ModDestination::OperatorWavetablePosition,
                  modulation::ModDestination::OperatorWavetableBend, modulation::ModDestination::OperatorWavetableAsymmetry,
                  modulation::ModDestination::OperatorWavetableSyncRatio,
                  modulation::ModDestination::OperatorWavetableFormant,
                  modulation::ModDestination::OperatorWavetableSyncAmount})
            {
                const auto mapped = modDestinationParam(dest, op);
                if (mapped.has_value() && mapped->paramId == paramId)
                    return std::make_pair(dest, op);
            }
        }

        for (std::uint8_t slot = 0; slot < 4; ++slot)
        {
            for (const auto dest :
                 {modulation::ModDestination::MasterFxMix, modulation::ModDestination::MasterReverbMix,
                  modulation::ModDestination::MasterReverbSize, modulation::ModDestination::MasterReverbDecay,
                  modulation::ModDestination::MasterReverbPreDelay, modulation::ModDestination::MasterReverbDiffusion,
                  modulation::ModDestination::MasterReverbModDepth, modulation::ModDestination::QuasarQsr1Distance,
                  modulation::ModDestination::QuasarQsr2Distance, modulation::ModDestination::QuasarQsr1Angle,
                  modulation::ModDestination::QuasarQsr2Angle, modulation::ModDestination::QuasarQsr1Height,
                  modulation::ModDestination::QuasarQsr2Height, modulation::ModDestination::QuasarRoomAmount,
                  modulation::ModDestination::QuasarDelayFeedback, modulation::ModDestination::QuasarDelayTime,
                  modulation::ModDestination::QuasarCntrLevel})
            {
                const auto mapped = modDestinationParam(dest, slot);
                if (mapped.has_value() && mapped->paramId == paramId)
                    return std::make_pair(dest, slot);
            }
        }

        for (const auto dest :
             {modulation::ModDestination::FilterCutoff, modulation::ModDestination::FilterResonance,
              modulation::ModDestination::Pan, modulation::ModDestination::MasterGain})
        {
            const auto mapped = modDestinationParam(dest, 0);
            if (mapped.has_value() && mapped->paramId == paramId)
                return std::make_pair(dest, static_cast<std::uint8_t>(0));
        }

        return std::nullopt;
    }

} // namespace pw8::plugin::ui
