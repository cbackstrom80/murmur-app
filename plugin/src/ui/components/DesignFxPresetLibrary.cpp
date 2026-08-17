#include "DesignFxPresetLibrary.h"

#include "DesignFxUiState.h"
#include "FxEffectPlayParams.h"
#include "pw8/content/ContentPaths.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::File designFxContentDir()
        {
            for (const auto& root : pw8::content::presetSearchRoots())
            {
                const juce::File parent = juce::File(root).getParentDirectory();
                const juce::File designFx = parent.getChildFile("design-fx");
                if (designFx.isDirectory())
                    return designFx;
            }

            const juce::File cwdDesignFx = juce::File::getCurrentWorkingDirectory().getChildFile("content/design-fx");
            if (cwdDesignFx.isDirectory())
                return cwdDesignFx;

            return {};
        }

        [[nodiscard]] juce::File designFxUserDir()
        {
            const juce::File dir = designFxContentDir().getChildFile("user");
            if (!dir.isDirectory())
                dir.createDirectory();
            return dir;
        }

        [[nodiscard]] bool isDiscreteParamSuffix(const juce::String& suffix)
        {
            return suffix == "SaturationCharacter" || suffix == "ReverbCharacter" || suffix == "CompCharacter"
                   || suffix == "Type" || suffix == "TapePanMode" || suffix == "VocoderBandCount";
        }

        void setFloatParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
        }

        void setIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, int value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
        }

        [[nodiscard]] juce::String sanitizedFileStem(const juce::String& name)
        {
            juce::String stem = name.trim();
            if (stem.isEmpty())
                stem = "user-preset";
            stem = stem.replaceCharacters(" /\\:*?\"<>|", "-----------");
            return stem;
        }

        [[nodiscard]] juce::File resolveContentPath(const juce::String& relativePath)
        {
            const juce::File rel(relativePath);
            if (juce::File::isAbsolutePath(relativePath) && rel.existsAsFile())
                return rel;

            for (const auto& root : pw8::content::presetSearchRoots())
            {
                const juce::File parent = juce::File(root).getParentDirectory();
                const juce::File candidate = parent.getChildFile(relativePath);
                if (candidate.existsAsFile())
                    return candidate;
            }

            const juce::File cwdCandidate = juce::File::getCurrentWorkingDirectory().getChildFile(relativePath);
            if (cwdCandidate.existsAsFile())
                return cwdCandidate;

            return {};
        }

        [[nodiscard]] std::array<float, kNumEffectSlotFields> effectSlotValues(const effects::EffectSlotParams& p)
        {
            return {
                static_cast<float>(p.type), p.mix,        p.saturationDriveDb, p.chorusRateHz,   p.chorusDepthMs,
                p.chorusBaseDelayMs,        p.tapeDelayMs, p.tapeFeedback,      p.tapeDriveDb,    p.tapeDuckAmount,
                p.tapeDriftDepthMs,         p.tapeDriftRateHz, static_cast<float>(p.tapePanMode), p.nodeInsanity,
                p.freqShiftHz,              p.freqShiftDelayMs, p.freqShiftFeedback, p.freqShiftLowCutHz,
                p.freqShiftHighCutHz,       p.fractalMorph, p.fractalBaseDelayMs, p.fractalRatio, p.fractalSpreadMs,
                p.reverbSizeParam,          p.reverbDecaySeconds, p.reverbPreDelayMs,
                p.reverbHighRatio,          p.reverbHighCrossoverHz, p.reverbLowRatio, p.reverbLowCrossoverHz,
                p.reverbDiffusion,          p.reverbDensity, p.reverbModDepth,    p.reverbModRateHz,
                p.reverbEarlyLevel,         p.reverbLateLevel, p.reverbRollOffHz, p.reverbVlfCutDb,
                p.eqLowFreqHz,              p.eqLowGainDb,  p.eqMidFreqHz,       p.eqMidGainDb,    p.eqMidQ,
                p.eqHighFreqHz,             p.eqHighGainDb,
                p.compThresholdDb,          p.compRatio,    p.compAttackMs,      p.compReleaseMs,  p.compKneeDb,
                p.compMakeupDb,
                p.compTransformerCore,      p.compTransformerBrand, p.compTransformerAmount,
                p.limiterCeilingDb,         p.limiterLookaheadMs, p.limiterReleaseMs,
                static_cast<float>(p.tapeDelaySync), static_cast<float>(p.tapeDelaySyncDivisionIndex),
                p.compAutoMakeup ? 1.0f : 0.0f, static_cast<float>(p.compCharacter),
                static_cast<float>(p.vocoderBandCount), p.vocoderFormant, p.vocoderSibilance, p.vocoderScGainDb,
                p.vocoderReleaseMs,
                static_cast<float>(p.reverbCharacter),
                static_cast<float>(p.saturationCharacter),
                p.eqOutGainDb,
            };
        }

        void appendSlotParams(DesignFxPresetEntry& entry, const effects::EffectSlotParams& slotParams)
        {
            const auto values = effectSlotValues(slotParams);
            const auto upsertFloat = [&](const juce::String& suffix, float value) {
                for (auto& pair : entry.floatParams)
                {
                    if (pair.first == suffix)
                    {
                        pair.second = value;
                        return;
                    }
                }
                entry.floatParams.emplace_back(suffix, value);
            };
            const auto upsertInt = [&](const juce::String& suffix, int value) {
                for (auto& pair : entry.intParams)
                {
                    if (pair.first == suffix)
                    {
                        pair.second = value;
                        return;
                    }
                }
                entry.intParams.emplace_back(suffix, value);
            };

            for (std::size_t i = 1; i < kNumEffectSlotFields; ++i)
            {
                const auto& spec = kEffectSlotFieldSpecs[i];
                if (spec.idSuffix == nullptr || spec.idSuffix[0] == '\0')
                    continue;
                const float value = values[i];
                const juce::String suffix(spec.idSuffix);
                if (spec.discrete)
                    upsertInt(suffix, static_cast<int>(value + 0.5f));
                else
                    upsertFloat(suffix, value);
            }
        }
    } // namespace

    void DesignFxPresetLibrary::rescan()
    {
        for (auto& list : byChip_)
            list.clear();

        const juce::File dir = designFxContentDir();
        if (!dir.isDirectory())
            return;

        juce::Array<juce::File> files;
        dir.findChildFiles(files, juce::File::findFiles, true, "*.json");
        files.sort();

        for (const auto& file : files)
            (void)loadFile(file);
    }

    bool DesignFxPresetLibrary::loadFile(const juce::File& file)
    {
        const auto parsed = juce::JSON::parse(file);
        if (!parsed.isObject())
            return false;

        DesignFxPresetEntry entry;
        entry.chipIndex = static_cast<std::size_t>(static_cast<int>(parsed.getProperty("chip", -1)));
        if (entry.chipIndex >= byChip_.size())
            return false;

        entry.name = parsed.getProperty("name", file.getFileNameWithoutExtension()).toString();
        entry.modePill = parsed.getProperty("modePill", juce::String()).toString();
        entry.sourceFile = file;

        const auto params = parsed.getProperty("params", juce::var());
        if (params.isObject())
        {
            if (auto* obj = params.getDynamicObject())
            {
                for (const auto& prop : obj->getProperties())
                {
                    const juce::String suffix = prop.name.toString();
                    const auto value = prop.value;
                    if (value.isInt() || value.isInt64())
                        entry.intParams.emplace_back(suffix, static_cast<int>(value));
                    else
                        entry.floatParams.emplace_back(suffix, static_cast<float>(value));
                }
            }
        }

        const auto uiKnobs = parsed.getProperty("uiKnobs", juce::var());
        if (uiKnobs.isArray())
        {
            entry.hasUiKnobs = true;
            for (int i = 0; i < static_cast<int>(entry.uiKnobs.size()) && i < uiKnobs.size(); ++i)
                entry.uiKnobs[static_cast<std::size_t>(i)] = static_cast<float>(uiKnobs[i]);
        }

        mergePw8Sidecar(entry, parsed);

        byChip_[entry.chipIndex].push_back(std::move(entry));
        return true;
    }

    void DesignFxPresetLibrary::mergePw8Sidecar(DesignFxPresetEntry& entry, const juce::var& parsed) const
    {
        const juce::String pw8Ref = parsed.getProperty("pw8Ref", juce::String()).toString();
        if (pw8Ref.isEmpty())
            return;

        const juce::File pw8File = resolveContentPath(pw8Ref);
        if (!pw8File.existsAsFile())
            return;

        const auto loaded = patch::loadPatchFromJson(pw8File.loadFileAsString().toStdString());
        if (!loaded.ok)
            return;

        const int slotIndex = static_cast<int>(parsed.getProperty("pw8Slot", 0));
        const bool masterSlot = static_cast<bool>(parsed.getProperty("pw8Master", false));

        if (masterSlot)
        {
            if (slotIndex < 0 || slotIndex >= static_cast<int>(effects::kNumMasterSlots))
                return;
            appendSlotParams(entry, loaded.patch.masterEffects[static_cast<std::size_t>(slotIndex)]);
        }
        else
        {
            if (slotIndex < 0 || slotIndex >= static_cast<int>(effects::kNumLayerInsertSlots))
                return;
            appendSlotParams(entry, loaded.patch.layerA.insertEffects[static_cast<std::size_t>(slotIndex)]);
        }
    }

    std::size_t DesignFxPresetLibrary::countForChip(std::size_t chipIndex) const
    {
        if (chipIndex >= byChip_.size())
            return 0;
        return byChip_[chipIndex].size();
    }

    bool DesignFxPresetLibrary::hasEntriesForChip(std::size_t chipIndex) const
    {
        return countForChip(chipIndex) > 0;
    }

    const DesignFxPresetEntry& DesignFxPresetLibrary::entry(std::size_t chipIndex, std::size_t presetIndex) const
    {
        static const DesignFxPresetEntry kEmpty{};
        if (chipIndex >= byChip_.size() || byChip_[chipIndex].empty())
            return kEmpty;
        return byChip_[chipIndex][presetIndex % byChip_[chipIndex].size()];
    }

    void DesignFxPresetLibrary::applyEntry(const DesignFxPresetEntry& preset,
                                           juce::AudioProcessorValueTreeState& apvts,
                                           const juce::String& paramPrefix, DesignFxUiState* uiState) const
    {
        for (const auto& [suffix, value] : preset.floatParams)
            setFloatParam(apvts, paramPrefix + suffix, value);
        for (const auto& [suffix, value] : preset.intParams)
            setIntParam(apvts, paramPrefix + suffix, value);

        if (uiState != nullptr && preset.hasUiKnobs)
        {
            for (std::size_t i = 0; i < preset.uiKnobs.size(); ++i)
                uiState->setKnobValue(preset.chipIndex, i, preset.uiKnobs[i]);
        }
    }

    DesignFxPresetEntry DesignFxPresetLibrary::captureEntry(std::size_t chipIndex,
                                                            juce::AudioProcessorValueTreeState& apvts,
                                                            const juce::String& paramPrefix,
                                                            const DesignFxUiState& uiState,
                                                            const juce::String& modePill,
                                                            const juce::String& name) const
    {
        DesignFxPresetEntry entry;
        entry.chipIndex = chipIndex;
        entry.name = name;
        entry.modePill = modePill;

        const auto& spec = fxDesignSpecForChip(chipIndex);
        for (const auto& knob : spec.knobs)
        {
            if (knob.fieldSuffix == nullptr || knob.fieldSuffix[0] == '\0')
                continue;

            const juce::String id = paramPrefix + knob.fieldSuffix;
            if (auto* raw = apvts.getRawParameterValue(id))
            {
                const float value = raw->load();
                if (isDiscreteParamSuffix(knob.fieldSuffix))
                    entry.intParams.emplace_back(knob.fieldSuffix, static_cast<int>(value + 0.5f));
                else
                    entry.floatParams.emplace_back(knob.fieldSuffix, value);
            }
        }

        entry.hasUiKnobs = true;
        for (std::size_t i = 0; i < entry.uiKnobs.size(); ++i)
            entry.uiKnobs[i] = uiState.knobValue(chipIndex, i);

        return entry;
    }

    bool DesignFxPresetLibrary::saveEntry(const DesignFxPresetEntry& entry) const
    {
        const juce::File userDir = designFxUserDir();
        if (!userDir.isDirectory())
            return false;

        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        root->setProperty("chip", static_cast<int>(entry.chipIndex));
        root->setProperty("name", entry.name);
        if (entry.modePill.isNotEmpty())
            root->setProperty("modePill", entry.modePill);

        juce::DynamicObject::Ptr params = new juce::DynamicObject();
        for (const auto& [suffix, value] : entry.floatParams)
            params->setProperty(suffix, value);
        for (const auto& [suffix, value] : entry.intParams)
            params->setProperty(suffix, value);
        root->setProperty("params", juce::var(params.get()));

        if (entry.hasUiKnobs)
        {
            juce::Array<juce::var> uiKnobs;
            for (float v : entry.uiKnobs)
                uiKnobs.add(v);
            root->setProperty("uiKnobs", uiKnobs);
        }

        const juce::String stamp = juce::Time::getCurrentTime().formatted("%Y%m%d-%H%M%S");
        const juce::File file =
            userDir.getChildFile("user-" + juce::String(static_cast<int>(entry.chipIndex)) + "-" + sanitizedFileStem(entry.name)
                               + "-" + stamp + ".json");

        const juce::var json(root.get());
        return file.replaceWithText(juce::JSON::toString(json, true));
    }

    bool DesignFxPresetLibrary::isUserPreset(const DesignFxPresetEntry& entry) const
    {
        if (!entry.sourceFile.existsAsFile())
            return false;
        const juce::File userDir = designFxUserDir();
        return entry.sourceFile.isAChildOf(userDir);
    }

    bool DesignFxPresetLibrary::deleteEntry(std::size_t chipIndex, std::size_t presetIndex) const
    {
        const auto& preset = entry(chipIndex, presetIndex);
        if (!isUserPreset(preset))
            return false;
        return preset.sourceFile.deleteFile();
    }

    bool DesignFxPresetLibrary::renameEntry(std::size_t chipIndex, std::size_t presetIndex,
                                            const juce::String& newName) const
    {
        const auto& preset = entry(chipIndex, presetIndex);
        if (!isUserPreset(preset) || newName.trim().isEmpty())
            return false;

        const auto parsed = juce::JSON::parse(preset.sourceFile);
        if (!parsed.isObject())
            return false;

        if (auto* obj = parsed.getDynamicObject())
            obj->setProperty("name", newName.trim());

        return preset.sourceFile.replaceWithText(juce::JSON::toString(parsed, true));
    }

} // namespace pw8::plugin::ui
