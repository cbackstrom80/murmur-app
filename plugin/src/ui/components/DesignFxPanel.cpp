#include "DesignFxPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "FxEffectPlayParams.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] int readEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        void setEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, int typeOrdinal)
        {
            if (auto* param = apvts.getParameter(prefix + "Type"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(typeOrdinal)));
        }

        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const char* id, float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        void setBoolParam(juce::AudioProcessorValueTreeState& apvts, const char* id, bool on)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(on ? 1.0f : 0.0f);
        }

        void setFloatParam(juce::AudioProcessorValueTreeState& apvts, const char* id, float value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(value));
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

        void applyFxDesignPresetValues(std::size_t chipIndex, std::size_t presetIndex,
                                       juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            const auto setF = [&](const char* suffix, float value) { setFloatParam(apvts, prefix + suffix, value); };
            const auto setI = [&](const char* suffix, int value) { setIntParam(apvts, prefix + suffix, value); };

            switch (chipIndex)
            {
                case 1:
                    switch (presetIndex % 4)
                    {
                        case 0: setI("SaturationCharacter", 0); setF("SaturationDrive", 18.0f); break;
                        case 1: setI("SaturationCharacter", 1); setF("SaturationDrive", 12.0f); break;
                        case 2: setI("SaturationCharacter", 2); setF("SaturationDrive", 24.0f); break;
                        default: setI("SaturationCharacter", 3); setF("SaturationDrive", 32.0f); break;
                    }
                    break;
                case 2:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("ChorusRate", 0.45f); setF("ChorusDepth", 8.0f); break;
                        case 1: setF("ChorusRate", 0.2f); setF("ChorusDepth", 3.0f); break;
                        default: setF("ChorusRate", 0.8f); setF("ChorusDepth", 12.0f); break;
                    }
                    break;
                case 3:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("TapeDriftRate", 0.25f); setF("TapeDriftDepth", 2.0f); setF("TapeDrive", 4.0f); break;
                        case 1: setF("TapeDriftRate", 0.6f); setF("TapeDriftDepth", 5.0f); setF("TapeDrive", 8.0f); break;
                        default: setF("TapeDelayMs", 280.0f); setF("TapeFeedback", 0.35f); break;
                    }
                    break;
                case 4:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("Mix", 0.55f); break;
                        case 1: setF("Mix", 0.65f); break;
                        default: setF("Mix", 0.45f); break;
                    }
                    break;
                case 5:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("FreqShiftHz", 7.0f); setF("FreqShiftFeedback", 0.55f); break;
                        case 1: setF("FreqShiftHz", -12.0f); setF("FreqShiftFeedback", 0.35f); break;
                        default: setF("FreqShiftHz", 14.0f); setF("FreqShiftFeedback", 0.65f); break;
                    }
                    break;
                case 6:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("FractalMorph", 0.35f); break;
                        case 1: setF("FractalMorph", 0.65f); break;
                        default: setF("FractalMorph", 0.85f); break;
                    }
                    break;
                case 7:
                    switch (presetIndex % 5)
                    {
                        case 0:
                            setI("ReverbCharacter", 2);
                            setF("ReverbDecaySeconds", 3.5f);
                            setF("ReverbSize", 1.4f);
                            break;
                        case 1:
                            setI("ReverbCharacter", 1);
                            setF("ReverbDecaySeconds", 2.2f);
                            setF("ReverbDiffusion", 0.85f);
                            break;
                        case 2:
                            setI("ReverbCharacter", 3);
                            setF("ReverbDecaySeconds", 1.1f);
                            setF("ReverbPreDelayMs", 8.0f);
                            break;
                        case 3:
                            setI("ReverbCharacter", 4);
                            setF("ReverbModDepth", 0.45f);
                            break;
                        default:
                            setI("ReverbCharacter", 0);
                            setF("ReverbModDepth", 0.85f);
                            setF("ReverbHighRatio", 0.95f);
                            break;
                    }
                    break;
                case 8:
                    switch (presetIndex % 4)
                    {
                        case 0:
                            setF("EqLowGainDb", 0.0f);
                            setF("EqMidGainDb", 0.0f);
                            setF("EqHighGainDb", 0.0f);
                            setF("EqLowFreqHz", 200.0f);
                            setF("EqMidFreqHz", 1000.0f);
                            setF("EqHighFreqHz", 6000.0f);
                            setF("EqMidQ", 0.8f);
                            break;
                        case 1:
                            setF("EqLowGainDb", -5.0f);
                            setF("EqMidGainDb", -1.5f);
                            setF("EqLowFreqHz", 120.0f);
                            break;
                        case 2:
                            setF("EqHighGainDb", 4.0f);
                            setF("EqMidGainDb", 1.0f);
                            setF("EqHighFreqHz", 10000.0f);
                            break;
                        default:
                            setF("EqMidGainDb", -4.0f);
                            setF("EqLowGainDb", 1.5f);
                            setF("EqMidFreqHz", 750.0f);
                            setF("EqMidQ", 1.4f);
                            break;
                    }
                    break;
                case 9:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("CompThresholdDb", -18.0f); setF("CompRatio", 3.0f); setI("CompCharacter", 0); break;
                        case 1: setF("CompThresholdDb", -14.0f); setF("CompRatio", 4.0f); setI("CompCharacter", 0); break;
                        default: setF("CompThresholdDb", -22.0f); setF("CompRatio", 6.0f); setI("CompCharacter", 1); break;
                    }
                    break;
                case 10:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("LimiterCeilingDb", -0.3f); setF("LimiterLookaheadMs", 5.0f); break;
                        case 1: setF("LimiterCeilingDb", -1.0f); setF("LimiterReleaseMs", 80.0f); break;
                        default: setF("LimiterCeilingDb", -0.1f); setF("LimiterReleaseMs", 40.0f); break;
                    }
                    break;
                case 11:
                    switch (presetIndex % 3)
                    {
                        case 0: setF("VocoderBandCount", 16.0f); setF("VocoderFormant", 0.5f); break;
                        case 1: setF("VocoderBandCount", 12.0f); setF("VocoderFormant", 0.35f); break;
                        default: setF("VocoderBandCount", 16.0f); setF("VocoderFormant", 0.72f); break;
                    }
                    break;
                default:
                    break;
            }
        }

        [[nodiscard]] float valueFromTrackX(juce::Rectangle<int> track, int x)
        {
            if (track.isEmpty())
                return 0.0f;
            return juce::jlimit(0.0f, 1.0f, static_cast<float>(x - track.getX()) / static_cast<float>(track.getWidth()));
        }
    } // namespace

    DesignFxPanel::DesignFxPanel(PatchworkEightProcessor& processor,
                                 ModAssignmentController& modAssignmentController)
        : processor_(processor), signalChain_(processor.apvts), detailStrip_(processor),
          heroViz_(processor.apvts)
    {
        juce::ignoreUnused(modAssignmentController);

        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        titleLabel_.setText("FX RACK", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        presetLabel_.setText("PRESET: CLASS A TUBE CRUNCH", juce::dontSendNotification);
        presetLabel_.setFont(fonts::label(8.0f));
        presetLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(presetLabel_);

        addAndMakeVisible(signalChain_);
        signalChain_.setUiState(&uiState_);
        uiState_.loadUserPreferences();
        {
            std::array<std::size_t, layout::kDesignFxPageSlotCount> order{};
            for (std::size_t i = 0; i < order.size(); ++i)
                order[i] = uiState_.chipAtDisplayIndex(i);
            processor_.applyFxProcessOrderFromChipDisplay(order);
        }
        signalChain_.onDisplayOrderChanged = [this]() {
            std::array<std::size_t, layout::kDesignFxPageSlotCount> order{};
            for (std::size_t i = 0; i < order.size(); ++i)
                order[i] = uiState_.chipAtDisplayIndex(i);
            processor_.applyFxProcessOrderFromChipDisplay(order);
            uiState_.saveUserPreferences();
        };
        signalChain_.onChipSelected = [this](std::size_t chipIndex) { bindSelectedChip(chipIndex); };

        detailStrip_.setDesignFxPageMode(true);
        detailStrip_.onVocoderLabRequested = [this](std::size_t slotIndex) {
            if (onVocoderLabRequested)
                onVocoderLabRequested(slotIndex);
        };
        detailStrip_.onDesignModeChanged = [this](const juce::String& pill) {
            heroViz_.setDesignModePill(pill);
        };
        detailStrip_.setDesignFxUiState(&uiState_);
        detailStrip_.onDesignUiChanged = [this]() {
            syncStubKnobsToApvts();
            heroViz_.setDesignFxUiState(&uiState_);
            heroViz_.repaint();
            uiState_.saveUserPreferences();
        };
        addAndMakeVisible(detailStrip_);
        addAndMakeVisible(heroViz_);
        heroViz_.setDesignFxUiState(&uiState_);
        heroViz_.onUiPreferenceChanged = [this]() { uiState_.saveUserPreferences(); };

        presetLibrary_.rescan();
        bindSelectedChip(1);
        startTimerHz(8);
    }

    void DesignFxPanel::syncStubKnobsToApvts()
    {
        const auto prefix = selectedParamPrefix();
        if (prefix.isEmpty())
            return;

        if (selectedChip_ == 4)
            applyMoodKnobsToEq(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 1)
            applySatStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 2)
            applyChrStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 3)
            applyTapeStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 5)
            applyFshfStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 6)
            applyFracStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 8)
            applyEqStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 10)
            applyLimStubKnobs(processor_.apvts, prefix, uiState_);
    }

    void DesignFxPanel::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        backButton_.setVisible(embedded);
        titleLabel_.setVisible(embedded);
        resized();
    }

    void DesignFxPanel::bindSelectedChip(std::size_t chipIndex)
    {
        selectedChip_ = juce::jlimit<std::size_t>(0, layout::kDesignFxPageSlotCount - 1, chipIndex);
        signalChain_.setSelectedChip(selectedChip_);

        static constexpr struct
        {
            int effectType;
            int engineSlot;
            bool disabled;
            bool uiOnly;
        } kChipMap[layout::kDesignFxPageSlotCount] = {
            {0, 0, false, false},  {1, 0, false, false}, {2, 1, false, false}, {3, 2, false, false},
            {8, 2, false, false},  {5, 3, false, false}, {6, 4, false, false}, {7, 2, false, false},
            {8, 5, false, false},  {9, 6, false, false}, {10, 6, false, false}, {11, 2, false, false},
        };

        const auto& chip = kChipMap[selectedChip_];
        if (chip.disabled || chip.engineSlot < 0)
            return;

        const auto prefix = designFxEngineSlotPrefix(chip.engineSlot);

        if (selectedChip_ == 4)
            applyMoodKnobsToEq(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 1)
            applySatStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 2)
            applyChrStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 3)
            applyTapeStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 5)
            applyFshfStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 6)
            applyFracStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 8)
            applyEqStubKnobs(processor_.apvts, prefix, uiState_);
        else if (selectedChip_ == 10)
            applyLimStubKnobs(processor_.apvts, prefix, uiState_);

        detailStrip_.selectEngineSlot(static_cast<std::size_t>(chip.engineSlot));
        detailStrip_.setDesignFxChipIndex(selectedChip_);
        heroViz_.bindChip(selectedChip_, prefix, &processor_, chip.engineSlot);

        const auto& modeSpec = fxDesignSpecForChip(selectedChip_);
        if (modeSpec.modePillCount > 0)
        {
            juce::String pill = modeSpec.modePills[0] != nullptr ? modeSpec.modePills[0] : juce::String();
            if (selectedChip_ == 1)
            {
                const int character =
                    processor_.apvts.getRawParameterValue(prefix + "SaturationCharacter") != nullptr
                        ? static_cast<int>(
                              processor_.apvts.getRawParameterValue(prefix + "SaturationCharacter")->load() + 0.5f)
                        : 0;
                pill = saturationDesignPillFromCharacter(character);
            }
            else if (selectedChip_ == 7)
            {
                const int character =
                    processor_.apvts.getRawParameterValue(prefix + "ReverbCharacter") != nullptr
                        ? static_cast<int>(
                              processor_.apvts.getRawParameterValue(prefix + "ReverbCharacter")->load() + 0.5f)
                        : 0;
                const float modDepth = readParam(processor_.apvts, (prefix + "ReverbModDepth").toRawUTF8(), 0.0f);
                pill = reverbDesignPillFromCharacter(character, modDepth);
            }
            else if (selectedChip_ == 9)
            {
                const int character = processor_.apvts.getRawParameterValue(prefix + "CompCharacter") != nullptr
                                          ? static_cast<int>(processor_.apvts.getRawParameterValue(prefix + "CompCharacter")
                                                                 ->load()
                                                             + 0.5f)
                                          : 0;
                pill = compDesignPillFromCharacter(character);
            }
            else if (selectedChip_ == 4)
                pill = uiState_.moodPill();
            heroViz_.setDesignModePill(pill);
        }

        syncPresetLabelFromChip();
        resized();
    }

    void DesignFxPanel::syncPresetLabelFromChip()
    {
        const auto count = presetLibrary_.hasEntriesForChip(selectedChip_)
                               ? presetLibrary_.countForChip(selectedChip_)
                               : fxDesignPresetCount(selectedChip_);
        presetIndices_[selectedChip_] %= juce::jmax<std::size_t>(1, count);

        juce::String label;
        if (presetLibrary_.hasEntriesForChip(selectedChip_))
            label = presetLibrary_.entry(selectedChip_, presetIndices_[selectedChip_]).name;
        else
            label = fxDesignPresetLabel(selectedChip_, presetIndices_[selectedChip_]);

        presetLabel_.setText("PRESET: " + label, juce::dontSendNotification);
    }

    void DesignFxPanel::applyDesignPreset(std::size_t presetIndex)
    {
        const auto count = presetLibrary_.hasEntriesForChip(selectedChip_)
                               ? presetLibrary_.countForChip(selectedChip_)
                               : fxDesignPresetCount(selectedChip_);
        if (count == 0)
            return;

        presetIndices_[selectedChip_] = presetIndex % count;
        const auto prefix = selectedParamPrefix();
        if (prefix.isEmpty())
            return;

        if (presetLibrary_.hasEntriesForChip(selectedChip_))
        {
            const auto& entry = presetLibrary_.entry(selectedChip_, presetIndices_[selectedChip_]);
            presetLibrary_.applyEntry(entry, processor_.apvts, prefix, &uiState_);
            if (entry.modePill.isNotEmpty())
            {
                if (selectedChip_ == 4)
                    uiState_.setMoodPill(entry.modePill);
                detailStrip_.applyDesignModePill(entry.modePill);
            }
        }
        else
        {
            applyFxDesignPresetValues(selectedChip_, presetIndices_[selectedChip_], processor_.apvts, prefix);
        }

        bindSelectedChip(selectedChip_);
        detailStrip_.setDesignFxChipIndex(selectedChip_);
        syncStubKnobsToApvts();
    }

    void DesignFxPanel::renameCurrentPreset()
    {
        if (!presetLibrary_.hasEntriesForChip(selectedChip_))
            return;

        const auto& current = presetLibrary_.entry(selectedChip_, presetIndices_[selectedChip_]);
        if (!presetLibrary_.isUserPreset(current))
            return;

        auto* dialog = new juce::AlertWindow("Rename preset", "Enter a new name for this saved preset:",
                                             juce::AlertWindow::QuestionIcon);
        dialog->addTextEditor("name", current.name, "Preset name");
        dialog->addButton("Rename", 1, juce::KeyPress(juce::KeyPress::returnKey));
        dialog->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

        dialog->enterModalState(
            true,
            juce::ModalCallbackFunction::create([this, dialog](int result) {
                if (result == 1)
                {
                    const juce::String newName = dialog->getTextEditorContents("name").trim();
                    if (newName.isNotEmpty()
                        && presetLibrary_.renameEntry(selectedChip_, presetIndices_[selectedChip_], newName))
                    {
                        presetLibrary_.rescan();
                        syncPresetLabelFromChip();
                    }
                }
                delete dialog;
            }),
            true);
    }

    juce::String DesignFxPanel::currentModePillForChip() const
    {
        const auto prefix = selectedParamPrefix();
        if (prefix.isEmpty())
            return {};

        if (selectedChip_ == 1)
        {
            const int character =
                processor_.apvts.getRawParameterValue(prefix + "SaturationCharacter") != nullptr
                    ? static_cast<int>(processor_.apvts.getRawParameterValue(prefix + "SaturationCharacter")->load() + 0.5f)
                    : 0;
            return saturationDesignPillFromCharacter(character);
        }
        if (selectedChip_ == 4)
            return uiState_.moodPill();
        if (selectedChip_ == 7)
        {
            const int character = processor_.apvts.getRawParameterValue(prefix + "ReverbCharacter") != nullptr
                                      ? static_cast<int>(processor_.apvts.getRawParameterValue(prefix + "ReverbCharacter")->load()
                                                         + 0.5f)
                                      : 0;
            const float modDepth = readParam(processor_.apvts, (prefix + "ReverbModDepth").toRawUTF8(), 0.0f);
            return reverbDesignPillFromCharacter(character, modDepth);
        }
        if (selectedChip_ == 9)
        {
            const int character = processor_.apvts.getRawParameterValue(prefix + "CompCharacter") != nullptr
                                      ? static_cast<int>(processor_.apvts.getRawParameterValue(prefix + "CompCharacter")->load()
                                                         + 0.5f)
                                      : 0;
            return compDesignPillFromCharacter(character);
        }
        return {};
    }

    void DesignFxPanel::saveCurrentPreset()
    {
        const auto prefix = selectedParamPrefix();
        if (prefix.isEmpty() || selectedChip_ == 0)
            return;

        const juce::String pill = currentModePillForChip();
        juce::String name = presetLabel_.getText();
        if (name.startsWithIgnoreCase("PRESET: "))
            name = name.substring(8).trim();
        if (name.isEmpty())
            name = "USER CAPTURE";

        const auto captured =
            presetLibrary_.captureEntry(selectedChip_, processor_.apvts, prefix, uiState_, pill, name);
        if (!presetLibrary_.saveEntry(captured))
            return;

        presetLibrary_.rescan();
        presetIndices_[selectedChip_] = presetLibrary_.countForChip(selectedChip_) - 1;
        syncPresetLabelFromChip();
    }

    void DesignFxPanel::deleteCurrentPreset()
    {
        if (!presetLibrary_.hasEntriesForChip(selectedChip_))
            return;

        const auto& current = presetLibrary_.entry(selectedChip_, presetIndices_[selectedChip_]);
        if (!presetLibrary_.isUserPreset(current))
            return;

        if (!presetLibrary_.deleteEntry(selectedChip_, presetIndices_[selectedChip_]))
            return;

        presetLibrary_.rescan();
        presetIndices_[selectedChip_] = 0;
        syncPresetLabelFromChip();
        if (presetLibrary_.countForChip(selectedChip_) > 0)
            applyDesignPreset(presetIndices_[selectedChip_]);
        else
            bindSelectedChip(selectedChip_);
    }

    void DesignFxPanel::showPresetMenu(juce::Point<int> anchor)
    {
        const auto count = presetLibrary_.hasEntriesForChip(selectedChip_)
                               ? presetLibrary_.countForChip(selectedChip_)
                               : fxDesignPresetCount(selectedChip_);

        juce::PopupMenu menu;
        if (count == 0)
        {
            menu.addItem(1, "Default", true);
        }
        else
        {
            for (std::size_t i = 0; i < count; ++i)
            {
                juce::String label;
                if (presetLibrary_.hasEntriesForChip(selectedChip_))
                    label = presetLibrary_.entry(selectedChip_, i).name;
                else
                    label = fxDesignPresetLabel(selectedChip_, i);

                menu.addItem(static_cast<int>(i + 1), label, presetIndices_[selectedChip_] == i);
            }
        }

        menu.addSeparator();
        menu.addItem(9001, "Save current settings...");
        if (presetLibrary_.hasEntriesForChip(selectedChip_)
            && presetLibrary_.isUserPreset(presetLibrary_.entry(selectedChip_, presetIndices_[selectedChip_])))
        {
            menu.addItem(9002, "Delete saved preset...");
            menu.addItem(9003, "Rename saved preset...");
        }

        menu.showMenuAsync(
            juce::PopupMenu::Options().withTargetComponent(this).withTargetScreenArea(
                juce::Rectangle<int>(anchor.x, anchor.y, 1, 1)),
            [this, count](int result) {
                if (result == 9001)
                    saveCurrentPreset();
                else if (result == 9002)
                    deleteCurrentPreset();
                else if (result == 9003)
                    renameCurrentPreset();
                else if (result > 0)
                    applyDesignPreset(static_cast<std::size_t>(result - 1) % juce::jmax<std::size_t>(1, count));
            });
    }

    juce::String DesignFxPanel::selectedParamPrefix() const
    {
        const int slot = selectedEngineSlot();
        if (slot < 0)
            return {};
        return designFxEngineSlotPrefix(slot);
    }

    int DesignFxPanel::selectedEngineSlot() const
    {
        static constexpr int kSlots[layout::kDesignFxPageSlotCount] = {0, 0, 1, 2, 2, 3, 4, 2, 5, 6, 6, 2};
        return kSlots[selectedChip_];
    }

    juce::String DesignFxPanel::selectedChipTitle() const
    {
        const auto& spec = fxDesignSpecForChip(selectedChip_);
        if (spec.slotLabel[0] == '\0')
            return spec.heroTitle;
        return juce::String(spec.heroTitle) + " (SLOT " + spec.slotLabel + ")";
    }

    void DesignFxPanel::timerCallback()
    {
        repaint(headerBounds_);
        repaint(routingBounds_);
    }

    void DesignFxPanel::mouseDown(const juce::MouseEvent& event)
    {
        if (presetChipBounds_.contains(event.getPosition()))
        {
            showPresetMenu(presetChipBounds_.getBottomLeft());
            return;
        }

        if (sidechainChipBounds_.contains(event.getPosition()) && selectedChip_ == 11 && onVocoderLabRequested)
        {
            onVocoderLabRequested(static_cast<std::size_t>(selectedEngineSlot()));
            return;
        }

        if (activeToggleBounds_.contains(event.getPosition()))
        {
            const int slot = selectedEngineSlot();
            if (slot >= 0)
            {
                const auto prefix = selectedParamPrefix();
                const int current = readEffectType(processor_.apvts, prefix);
                if (current == 0)
                {
                    static constexpr int kTypes[layout::kDesignFxPageSlotCount] = {
                        0, 1, 2, 3, 8, 5, 6, 7, 8, 9, 10, 11};
                    setEffectType(processor_.apvts, prefix, kTypes[selectedChip_]);
                    bindSelectedChip(selectedChip_);
                }
                else
                    setEffectType(processor_.apvts, prefix, 0);
                repaint(headerBounds_);
            }
            return;
        }

        if (prePostButtonBounds_.contains(event.getPosition()))
        {
            const bool next = readParam(processor_.apvts, kFxRoutingPrePostId) < 0.5f;
            setBoolParam(processor_.apvts, kFxRoutingPrePostId, next);
            repaint(routingBounds_);
            return;
        }
        if (bypassButtonBounds_.contains(event.getPosition()))
        {
            const bool next = readParam(processor_.apvts, kFxGlobalBypassId) < 0.5f;
            setBoolParam(processor_.apvts, kFxGlobalBypassId, next);
            repaint(routingBounds_);
            return;
        }
        if (globalWetTrackBounds_.contains(event.getPosition()))
        {
            activeDrag_ = DragTarget::GlobalWet;
            setFloatParam(processor_.apvts, kFxGlobalWetMixId,
                          valueFromTrackX(globalWetTrackBounds_, event.getPosition().x));
            repaint(routingBounds_);
            return;
        }
        if (sendATrackBounds_.contains(event.getPosition()))
        {
            activeDrag_ = DragTarget::SendA;
            setFloatParam(processor_.apvts, kFxSendAId, valueFromTrackX(sendATrackBounds_, event.getPosition().x));
            repaint(routingBounds_);
            return;
        }
        if (sendBTrackBounds_.contains(event.getPosition()))
        {
            activeDrag_ = DragTarget::SendB;
            setFloatParam(processor_.apvts, kFxSendBId, valueFromTrackX(sendBTrackBounds_, event.getPosition().x));
            repaint(routingBounds_);
        }
    }

    void DesignFxPanel::mouseDrag(const juce::MouseEvent& event)
    {
        switch (activeDrag_)
        {
            case DragTarget::GlobalWet:
                setFloatParam(processor_.apvts, kFxGlobalWetMixId,
                              valueFromTrackX(globalWetTrackBounds_, event.getPosition().x));
                break;
            case DragTarget::SendA:
                setFloatParam(processor_.apvts, kFxSendAId, valueFromTrackX(sendATrackBounds_, event.getPosition().x));
                break;
            case DragTarget::SendB:
                setFloatParam(processor_.apvts, kFxSendBId, valueFromTrackX(sendBTrackBounds_, event.getPosition().x));
                break;
            default:
                return;
        }
        repaint(routingBounds_);
    }

    void DesignFxPanel::paintFocusedHeader(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        const int slot = selectedEngineSlot();
        const bool active = slot >= 0 && readEffectType(processor_.apvts, selectedParamPrefix()) != 0;

        g.setColour(active ? palette::kAccent.withAlpha(0.25f) : palette::kPanel);
        g.fillEllipse(static_cast<float>(bounds.getX()), static_cast<float>(bounds.getCentreY() - 7), 14.0f, 14.0f);
        g.setColour(active ? palette::kAccent : palette::kTextDim);
        g.fillEllipse(static_cast<float>(bounds.getX() + 5), static_cast<float>(bounds.getCentreY() - 2), 4.0f, 4.0f);

        auto titleArea = bounds.withTrimmedLeft(24);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        g.drawText(selectedChipTitle(), titleArea.removeFromLeft(juce::jmin(420, titleArea.getWidth() - 200)),
                   juce::Justification::centredLeft);

        titleArea.removeFromLeft(8);
        g.setColour(palette::kBorder);
        g.fillRect(titleArea.removeFromLeft(1).withHeight(14).withY(bounds.getY() + 2));
        titleArea.removeFromLeft(8);

        auto presetArea = titleArea.removeFromLeft(160);
        presetChipBounds_ = presetArea;
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(presetArea.toFloat(), 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(presetArea.toFloat(), 4.0f, 0.8f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(presetLabel_.getText(), presetArea.reduced(8, 4), juce::Justification::centredLeft);

        auto right = bounds.withTrimmedLeft(bounds.getWidth() - 132);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("ACTIVE FX", right.removeFromLeft(64), juce::Justification::centredRight);
        right.removeFromLeft(8);
        activeToggleBounds_ = right.withSize(right.getWidth(), 16);
        g.setColour(active ? palette::kFigmaPillActive : palette::kPanel);
        g.fillRoundedRectangle(activeToggleBounds_.toFloat(), 4.0f);
        g.setColour(active ? palette::kAccent : palette::kTextDim);
        g.drawText("ACTIVE", activeToggleBounds_, juce::Justification::centred);
    }

    void DesignFxPanel::paintRoutingBar(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 8.0f, 1.0f);

        const bool prePostOn = readParam(processor_.apvts, kFxRoutingPrePostId) >= 0.5f;
        const bool bypassOn = readParam(processor_.apvts, kFxGlobalBypassId) >= 0.5f;
        const float globalWetMix = readParam(processor_.apvts, kFxGlobalWetMixId, 0.8f);
        const float sendA = readParam(processor_.apvts, kFxSendAId, 0.0f);
        const float sendB = readParam(processor_.apvts, kFxSendBId, 0.0f);

        auto inner = bounds.reduced(layout::kDesignFxPageRoutingPaddingX, 18);
        auto sidechain = inner.removeFromLeft(182);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("SIDECHAIN LINK", sidechain.removeFromLeft(65), juce::Justification::centredLeft);
        sidechain.removeFromLeft(8);
        sidechainChipBounds_ = sidechain;
        const bool vocSelected = selectedChip_ == 11;
        if (vocSelected)
        {
            g.setColour(palette::kAccent.withAlpha(0.12f));
            g.fillRoundedRectangle(sidechain.toFloat(), 4.0f);
        }
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(sidechain.toFloat(), 4.0f);
        g.setColour(palette::kBorderBright.withAlpha(0.5f));
        g.drawRoundedRectangle(sidechain.toFloat(), 4.0f, 0.8f);
        g.setColour(processor_.getSidechainActive() ? palette::kAccent : palette::kTextDim);
        g.fillEllipse(static_cast<float>(sidechain.getX() + 8), static_cast<float>(sidechain.getCentreY() - 2), 4.0f,
                      4.0f);
        g.setColour(vocSelected ? palette::kAccent : palette::kTextPrimary);
        g.drawText(vocSelected ? "SC → VOCODER LAB" : "SC BUS B → OP4 EXT", sidechain.withTrimmedLeft(18),
                   juce::Justification::centredLeft);

        auto mix = inner.withSizeKeepingCentre(246, 14);
        g.setColour(palette::kTextDim);
        g.drawText("GLOBAL WET MIX", mix.removeFromLeft(71), juce::Justification::centredLeft);
        mix.removeFromLeft(8);
        auto track = mix.removeFromLeft(140);
        globalWetTrackBounds_ = track;
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(track.toFloat(), 7.0f);
        g.setColour(palette::kAccentWarm);
        g.fillRoundedRectangle(track.getX() + 1, track.getY() + 1,
                               static_cast<int>((track.getWidth() - 2) * globalWetMix), track.getHeight() - 2, 6.0f);
        g.setColour(palette::kTextPrimary);
        g.drawText(juce::String(juce::roundToInt(globalWetMix * 100.0f)) + "%", mix, juce::Justification::centredLeft);

        auto sends = inner.removeFromRight(150);
        auto sendBRow = sends.removeFromBottom(14);
        auto sendARow = sends.removeFromBottom(16);
        g.setColour(palette::kTextDim);
        g.drawText("SEND B", sendBRow.removeFromLeft(42), juce::Justification::centredLeft);
        sendBRow.removeFromLeft(4);
        sendBTrackBounds_ = sendBRow;
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(sendBRow.toFloat(), 4.0f);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(sendBRow.getX() + 1, sendBRow.getY() + 1,
                               static_cast<int>((sendBRow.getWidth() - 2) * sendB), sendBRow.getHeight() - 2, 3.0f);

        g.setColour(palette::kTextDim);
        g.drawText("SEND A", sendARow.removeFromLeft(42), juce::Justification::centredLeft);
        sendARow.removeFromLeft(4);
        sendATrackBounds_ = sendARow;
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(sendARow.toFloat(), 4.0f);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(sendARow.getX() + 1, sendARow.getY() + 1,
                               static_cast<int>((sendARow.getWidth() - 2) * sendA), sendARow.getHeight() - 2, 3.0f);

        auto actions = inner.removeFromRight(183);
        auto prePost = actions.removeFromLeft(90);
        prePostButtonBounds_ = prePost;
        g.setColour(prePostOn ? palette::kFigmaPillActive : palette::kPanel);
        g.fillRoundedRectangle(prePost.toFloat(), 4.0f);
        g.setColour(prePostOn ? palette::kAccent : palette::kTextDim);
        g.drawText(prePostOn ? "POST FADER" : "PRE/POST", prePost, juce::Justification::centred);
        actions.removeFromLeft(10);
        auto bypass = actions;
        bypassButtonBounds_ = bypass;
        g.setColour(bypassOn ? palette::kAccentWarm.withAlpha(0.35f) : palette::kAccentWarm.withAlpha(0.18f));
        g.fillRoundedRectangle(bypass.toFloat(), 4.0f);
        g.setColour(palette::kAccentWarm);
        g.drawRoundedRectangle(bypass.toFloat(), 4.0f, 1.0f);
        g.setColour(bypassOn ? palette::kTextPrimary : palette::kAccentWarm);
        g.drawText(bypassOn ? "FX BYPASSED" : "ALL FX BYPASS", bypass, juce::Justification::centred);
    }

    void DesignFxPanel::paint(juce::Graphics& g)
    {
        if (!detailChromeBounds_.isEmpty())
        {
            g.setColour(palette::kPanelRaised.withAlpha(0.55f));
            g.fillRoundedRectangle(detailChromeBounds_.toFloat(), 8.0f);
            g.setColour(palette::kBorder.withAlpha(0.45f));
            g.drawRoundedRectangle(detailChromeBounds_.toFloat().reduced(0.5f), 8.0f, 1.0f);
        }

        paintFocusedHeader(g, headerBounds_);
        paintRoutingBar(g, routingBounds_);
    }

    void DesignFxPanel::paintOverChildren(juce::Graphics& g)
    {
        juce::ignoreUnused(g);
    }

    void DesignFxPanel::resized()
    {
        auto bounds = getLocalBounds();

        if (embeddedInDesignMode_)
        {
            auto header = bounds.removeFromTop(layout::kDesignLabPanelHeaderHeight);
            backButton_.setBounds(header.removeFromLeft(120));
            header.removeFromLeft(8);
            titleLabel_.setBounds(header.removeFromLeft(160));
        }

        signalChain_.setBounds(bounds.removeFromTop(layout::kDesignFxPageSignalChainSectionHeight));
        bounds.removeFromTop(layout::kDesignFxPageSectionGap);

        routingBounds_ = bounds.removeFromBottom(layout::kDesignFxPageRoutingBarHeight);
        bounds.removeFromBottom(layout::kDesignFxPageSectionGap);

        detailChromeBounds_ = bounds;

        auto inner = detailChromeBounds_.reduced(layout::kDesignFxPageDetailPadding);
        headerBounds_ = inner.removeFromTop(layout::kDesignFxPageDetailHeaderHeight);
        inner.removeFromTop(14);

        presetLabel_.setBounds({});
        auto body = inner.removeFromTop(layout::kDesignFxPageDetailBodyHeight);
        if (selectedChip_ == 8)
        {
            auto sidebar = body.removeFromLeft(layout::kDesignFxPageEqSidebarWidth);
            body.removeFromLeft(layout::kDesignFxPageDetailControlsVizGap);
            detailStrip_.setBounds(sidebar);
            heroViz_.setBounds(body);
        }
        else
        {
            auto heroBounds = body.removeFromRight(juce::jmax(220, body.getWidth() - layout::kDesignFxPageDetailControlsWidth
                                                                         - layout::kDesignFxPageDetailControlsVizGap));
            body.removeFromRight(layout::kDesignFxPageDetailControlsVizGap);
            detailStrip_.setBounds(body);
            heroViz_.setBounds(heroBounds);
        }
    }

} // namespace pw8::plugin::ui
