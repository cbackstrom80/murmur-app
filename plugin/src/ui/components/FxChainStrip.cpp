#include "DesignFxUiState.h"
#include "FxChainStrip.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "LabLauncherChip.h"
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

        [[nodiscard]] const char* transformerCoreName(int coreOrdinal) noexcept
        {
            switch (coreOrdinal)
            {
                case 1: return "NICKEL";
                case 2: return "IRON";
                case 3: return "STEEL";
                default: return "OFF";
            }
        }

        [[nodiscard]] const char* transformerBrandName(int brandOrdinal) noexcept
        {
            switch (brandOrdinal)
            {
                case 1: return "JENSEN";
                case 2: return "CINEMAG";
                case 3: return "SOWTER";
                default: return "NEUTRAL";
            }
        }

        [[nodiscard]] int readIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        void setIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, int value)
        {
            if (auto* param = apvts.getParameter(id))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(value)));
        }
    } // namespace

    FxChainStrip::FxChainStrip(PatchworkEightProcessor& processor)
        : processor_(processor), apvts_(processor.apvts), chainFlow_(apvts_), wireframe_(apvts_)
    {
        addAndMakeVisible(panel_);

        helpLabel_.setText(
            "Signal flows left→right: Layer inserts (I1–I3) then Master bus (M1–M4). "
            "All effects below are real DSP. Click a slot in the chain, pick TYPE, tweak knobs, use Swap to reorder.",
            juce::dontSendNotification);
        helpLabel_.setJustificationType(juce::Justification::centredLeft);
        helpLabel_.setFont(fonts::value(10.0f));
        helpLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addAndMakeVisible(helpLabel_);

        panel_.addAndMakeVisible(chainFlow_);
        chainFlow_.onSlotSelected = [this](std::size_t index) { selectSlot(index); };

        panel_.addAndMakeVisible(wireframe_);
        wireframe_.attachVisualizerBus(processor_.getVisualizerBus());

        static constexpr int kDefaultInsertTypes[] = {1, 2, 3};
        static constexpr int kDefaultMasterTypes[] = {7, 8, 9, 10};

        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            auto& slot = slots_[i];
            const bool isMaster = i >= 3;
            const std::size_t localIndex = isMaster ? i - 3 : i;
            slot.paramPrefix = isMaster ? masterFxParamId(localIndex, "") : insertFxParamId(localIndex, "");
            slot.shortLabel = isMaster ? ("M" + juce::String(static_cast<int>(localIndex + 1)))
                                       : ("I" + juce::String(static_cast<int>(localIndex + 1)));
            slot.defaultEnabledType = isMaster ? kDefaultMasterTypes[localIndex] : kDefaultInsertTypes[localIndex];
            slot.lastEnabledType = readEffectType(apvts_, slot.paramPrefix);
            if (slot.lastEnabledType == 0)
                slot.lastEnabledType = slot.defaultEnabledType;

            slot.selector = std::make_unique<GlowRingButton>(slot.shortLabel);
            slot.selector->setClickingTogglesState(false);
            slot.selector->onClick = [this, i]() { selectSlot(i); };
            panel_.addAndMakeVisible(*slot.selector);

            slot.selectorLabel.setText(slot.shortLabel, juce::dontSendNotification);
            slot.selectorLabel.setJustificationType(juce::Justification::centred);
            slot.selectorLabel.setFont(fonts::label(8.5f));
            slot.selectorLabel.setColour(juce::Label::textColourId, palette::kTextDim);
            panel_.addAndMakeVisible(slot.selectorLabel);
        }

        updateFlowPrefixes();

        typeRow_ = std::make_unique<MetadataFacetRow>("TYPE");
        typeRow_->setValues(
            juce::StringArray{"SATUR", "CHORUS", "TAPE", "NODE", "FSHF", "FRACT", "REVERB", "EQ", "COMP", "LIMIT"});
        typeRow_->onChange = [this]() {
            const auto chip = typeRow_->getSelectedValue();
            if (chip.isEmpty())
                return;
            setSelectedEffectType(fxTypeOrdinalFromChipLabel(chip));
        };
        panel_.addAndMakeVisible(*typeRow_);

        designModeRow_ = std::make_unique<MetadataFacetRow>("MODE");
        designModeRow_->setCompactFxMode(true);
        designModeRow_->setVisible(false);
        designModeRow_->onChange = [this]() {
            const auto pill = designModeRow_->getSelectedValue();
            if (pill.isEmpty())
                return;
            applyDesignModePill(pill);
        };
        panel_.addAndMakeVisible(*designModeRow_);

        vocoderLabChip_ = std::make_unique<LabLauncherChip>();
        vocoderLabChip_->setLabel("VOCODER LAB →");
        vocoderLabChip_->setHighlighted(true);
        vocoderLabChip_->setIcon(LabLauncherIcon::Vocoder);
        vocoderLabChip_->setVisible(false);
        vocoderLabChip_->onClick = [this]() {
            if (onVocoderLabRequested)
                onVocoderLabRequested(selectedSlotIndex_);
        };
        panel_.addAndMakeVisible(*vocoderLabChip_);

        quasarLabChip_ = std::make_unique<LabLauncherChip>();
        quasarLabChip_->setLabel("QUASAR LAB →");
        quasarLabChip_->setAccentColour(juce::Colour(0xff7c4dff));
        quasarLabChip_->setHighlighted(true);
        quasarLabChip_->setVisible(false);
        quasarLabChip_->onClick = [this]() {
            if (onQuasarLabRequested)
                onQuasarLabRequested(selectedSlotIndex_);
        };
        panel_.addAndMakeVisible(*quasarLabChip_);

        slotTitleLabel_.setJustificationType(juce::Justification::centredLeft);
        slotTitleLabel_.setFont(fonts::label(12.0f));
        slotTitleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        panel_.addAndMakeVisible(slotTitleLabel_);

        enableButton_ = std::make_unique<GlowRingButton>("ON");
        enableButton_->setClickingTogglesState(false);
        enableButton_->onClick = [this]() { toggleSelectedSlotEnabled(); };
        panel_.addAndMakeVisible(*enableButton_);

        for (auto* btn : {&swapLeft_, &swapRight_})
        {
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            panel_.addAndMakeVisible(*btn);
        }
        swapLeft_.onClick = [this]() { swapSelectedSlot(-1); };
        swapRight_.onClick = [this]() { swapSelectedSlot(1); };

        mixKnob_ = std::make_unique<GlowKnob>(apvts_, "", "Mix");
        mixKnob_->applyFigmaContext(figma::KnobContext::DesignFxDetail);
        panel_.addAndMakeVisible(*mixKnob_);

        transCoreRow_ = std::make_unique<MetadataFacetRow>("TRANS");
        transCoreRow_->setValues(juce::StringArray{"OFF", "NICKEL", "IRON", "STEEL"});
        transCoreRow_->onChange = [this]() {
            const auto& v = transCoreRow_->getSelectedValue();
            setTransformerCore(v.isEmpty() || v == "OFF" ? 0
                                 : v == "NICKEL"             ? 1
                                 : v == "IRON"               ? 2
                                                             : 3);
        };
        panel_.addAndMakeVisible(*transCoreRow_);

        transBrandRow_ = std::make_unique<MetadataFacetRow>("BRAND");
        transBrandRow_->setValues(juce::StringArray{"NEUTRAL", "JENSEN", "CINEMAG", "SOWTER"});
        transBrandRow_->onChange = [this]() {
            const auto& v = transBrandRow_->getSelectedValue();
            setTransformerBrand(v.isEmpty() || v == "NEUTRAL" ? 0
                                    : v == "JENSEN"              ? 1
                                    : v == "CINEMAG"             ? 2
                                                                 : 3);
        };
        panel_.addAndMakeVisible(*transBrandRow_);

        compCharacterRow_ = std::make_unique<MetadataFacetRow>("CHAR");
        compCharacterRow_->setValues(juce::StringArray{"VCA", "FET", "OPTO"});
        compCharacterRow_->onChange = [this]() {
            const auto& v = compCharacterRow_->getSelectedValue();
            setCompCharacter(v == "FET" ? 1 : v == "OPTO" ? 2 : 0);
        };
        panel_.addAndMakeVisible(*compCharacterRow_);

        compAutoMakeupRow_ = std::make_unique<MetadataFacetRow>("MAKEUP");
        compAutoMakeupRow_->setValues(juce::StringArray{"MANUAL", "AUTO"});
        compAutoMakeupRow_->onChange = [this]() {
            setCompAutoMakeup(compAutoMakeupRow_->getSelectedValue() == "AUTO");
        };
        panel_.addAndMakeVisible(*compAutoMakeupRow_);

        grMeterLabel_.setJustificationType(juce::Justification::centredLeft);
        grMeterLabel_.setFont(fonts::value(11.0f));
        grMeterLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        panel_.addAndMakeVisible(grMeterLabel_);

        delaySyncRow_ = std::make_unique<MetadataFacetRow>("SYNC");
        delaySyncRow_->setValues(juce::StringArray{"FREE", "TEMPO"});
        delaySyncRow_->onChange = [this]() {
            setDelaySyncEnabled(delaySyncRow_->getSelectedValue() == "TEMPO");
        };
        panel_.addAndMakeVisible(*delaySyncRow_);

        delayDivisionRow_ = std::make_unique<MetadataFacetRow>("DIV");
        delayDivisionRow_->setValues(
            juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/4T", "1/8.", "1/8T"});
        delayDivisionRow_->onChange = [this]() {
            const auto& v = delayDivisionRow_->getSelectedValue();
            int idx = 2;
            if (v == "1/1") idx = 0;
            else if (v == "1/2") idx = 1;
            else if (v == "1/4") idx = 2;
            else if (v == "1/8") idx = 3;
            else if (v == "1/16") idx = 4;
            else if (v == "1/4.") idx = 5;
            else if (v == "1/4T") idx = 6;
            else if (v == "1/8.") idx = 7;
            else if (v == "1/8T") idx = 8;
            setDelaySyncDivision(idx);
        };
        panel_.addAndMakeVisible(*delayDivisionRow_);

        selectSlot(0);
        startTimerHz(8);
    }

    FxChainStrip::~FxChainStrip() { stopTimer(); }

    void FxChainStrip::setPlayDashboardMode(bool dashboardMode)
    {
        if (designFxPageMode_)
            return;

        playDashboardMode_ = dashboardMode;
        helpLabel_.setVisible(!playDashboardMode_);
        wireframe_.setVisible(!playDashboardMode_);
        swapLeft_.setVisible(!playDashboardMode_);
        swapRight_.setVisible(!playDashboardMode_);
        slotTitleLabel_.setVisible(!playDashboardMode_);
        for (auto& slot : slots_)
        {
            slot.selector->setVisible(!playDashboardMode_);
            slot.selectorLabel.setVisible(!playDashboardMode_);
        }
        panel_.setTitle(playDashboardMode_ ? "FX RACK" : "FX Chain — All 10 Algorithms Live");
        updatePlayDashboardUi();
        resized();
    }

    void FxChainStrip::setDesignFxPageMode(bool designMode)
    {
        designFxPageMode_ = designMode;
        playDashboardMode_ = false;
        helpLabel_.setVisible(false);
        chainFlow_.setVisible(false);
        wireframe_.setVisible(!designMode);
        swapLeft_.setVisible(false);
        swapRight_.setVisible(false);
        slotTitleLabel_.setVisible(false);
        if (typeRow_ != nullptr)
            typeRow_->setVisible(!designMode);
        if (designModeRow_ != nullptr)
            designModeRow_->setVisible(designMode);
        for (auto& slot : slots_)
        {
            slot.selector->setVisible(false);
            slot.selectorLabel.setVisible(false);
        }
        panel_.setTitle("");
        if (designMode)
            rebuildDesignFxParamKnobs();
        else
            rebuildParamKnobs();
        updatePlayDashboardUi();
        resized();
    }

    void FxChainStrip::setDesignFxChipIndex(std::size_t chipIndex)
    {
        designFxChipIndex_ = chipIndex;
        if (designFxPageMode_)
        {
            rebuildDesignFxParamKnobs();
            syncDesignModeRowFromChip();
            resized();
        }
    }

    void FxChainStrip::setDesignFxUiState(DesignFxUiState* uiState)
    {
        designFxUiState_ = uiState;
        if (designFxPageMode_)
            rebuildDesignFxParamKnobs();
    }

    void FxChainStrip::selectEngineSlot(std::size_t index) { selectSlot(index); }

    void FxChainStrip::updatePlayDashboardUi()
    {
        if (designFxPageMode_)
        {
            vocoderLabChip_->setVisible(designFxChipIndex_ == 11);
            return;
        }

        if (!playDashboardMode_)
        {
            vocoderLabChip_->setVisible(false);
            return;
        }

        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        const bool quasarSlot = type == 13 && selectedSlotIndex_ >= 3;
        const bool vocoderSlot = type == 11;
        quasarLabChip_->setVisible(quasarSlot);
        vocoderLabChip_->setVisible(vocoderSlot);
        if (typeRow_ != nullptr)
            typeRow_->setVisible(!vocoderSlot && !quasarSlot);
        for (auto& knob : paramKnobs_)
            knob->setVisible(!vocoderSlot && !quasarSlot);
        if (mixKnob_ != nullptr)
            mixKnob_->setVisible(!vocoderSlot && !quasarSlot && type != 0);
    }

    void FxChainStrip::updateFlowPrefixes()
    {
        std::array<juce::String, 7> prefixes{};
        for (std::size_t i = 0; i < slots_.size(); ++i)
            prefixes[i] = slots_[i].paramPrefix;
        chainFlow_.setSlotPrefixes(prefixes);
    }

    FxChainStrip::SlotUi& FxChainStrip::selectedSlot() { return slots_[selectedSlotIndex_]; }

    const FxChainStrip::SlotUi& FxChainStrip::selectedSlot() const { return slots_[selectedSlotIndex_]; }

    bool FxChainStrip::showsMasterCompressorControls() const
    {
        return showsCompressorControls();
    }

    bool FxChainStrip::showsCompressorControls() const
    {
        return readEffectType(apvts_, selectedSlot().paramPrefix) == 9;
    }

    bool FxChainStrip::showsDelaySyncControls() const
    {
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        return type == 3;
    }

    bool FxChainStrip::canSwapSelectedSlot(int direction) const
    {
        const bool isMaster = selectedSlotIndex_ >= 3;
        const std::size_t local = isMaster ? selectedSlotIndex_ - 3 : selectedSlotIndex_;
        const int next = static_cast<int>(local) + direction;
        if (isMaster)
            return next >= 0 && next < static_cast<int>(kNumMasterFxSlots);
        return next >= 0 && next < static_cast<int>(kNumInsertFxSlots);
    }

    void FxChainStrip::swapSelectedSlot(int direction)
    {
        if (!canSwapSelectedSlot(direction))
            return;
        const bool isMaster = selectedSlotIndex_ >= 3;
        const std::size_t local = isMaster ? selectedSlotIndex_ - 3 : selectedSlotIndex_;
        const std::size_t neighbor = static_cast<std::size_t>(static_cast<int>(local) + direction);
        if (processor_.swapEffectSlots(isMaster, local, neighbor))
        {
            updateFlowPrefixes();
            rebuildParamKnobs();
            wireframe_.bindToSlot(selectedSlot().paramPrefix);
            refreshSelectorStates();
            syncTypeRowFromParams();
            syncTransformerRowsFromParams();
        }
    }

    void FxChainStrip::rebuildParamKnobs()
    {
        if (designFxPageMode_)
        {
            rebuildDesignFxParamKnobs();
            return;
        }

        paramKnobs_.clear();
        const auto& prefix = selectedSlot().paramPrefix;
        const int type = readEffectType(apvts_, prefix);
        const auto& spec = fxPlaySpecForType(type);

        if (mixKnob_ != nullptr)
            mixKnob_->setVisible(type != 0);

        for (const auto& def : spec.params)
        {
            if (def.fieldSuffix == nullptr || def.fieldSuffix[0] == '\0')
                continue;
            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + def.fieldSuffix, def.label);
            knob->applyFigmaContext(figma::KnobContext::DesignFxDetail);
            panel_.addAndMakeVisible(*knob);
            paramKnobs_.push_back(std::move(knob));
        }

        updatePlayDashboardUi();
        resized();
    }

    void FxChainStrip::rebuildDesignFxParamKnobs()
    {
        paramKnobs_.clear();
        designKnobGrid_.clear();

        const auto& prefix = selectedSlot().paramPrefix;
        const int type = readEffectType(apvts_, prefix);
        const auto& spec = fxDesignSpecForChip(designFxChipIndex_);

        if (mixKnob_ != nullptr)
            mixKnob_->setVisible(false);

        for (std::size_t i = 0; i < spec.knobs.size(); ++i)
        {
            const auto& def = spec.knobs[i];
            if (def.fieldSuffix == nullptr || def.fieldSuffix[0] == '\0')
            {
                if (def.label != nullptr && def.label[0] != '\0')
                {
                    const float initial = designFxUiState_ != nullptr
                                              ? designFxUiState_->knobValue(designFxChipIndex_, i)
                                              : 0.5f;
                    const std::size_t chip = designFxChipIndex_;
                    auto knob = std::make_unique<GlowKnob>(
                        def.label, 0.0, 1.0, static_cast<double>(initial),
                        [this, chip, i](double value) {
                            if (designFxUiState_ != nullptr)
                                designFxUiState_->setKnobValue(chip, i, static_cast<float>(value));
                            if (onDesignUiChanged)
                                onDesignUiChanged();
                        });
                    knob->applyFigmaContext(figma::KnobContext::DesignFxDetail);
                    knob->setShowNameLabel(false);
                    knob->setShowModRouteRing(false);
                    auto* raw = knob.get();
                    panel_.addAndMakeVisible(*knob);
                    paramKnobs_.push_back(std::move(knob));
                    designKnobGrid_.push_back(raw);
                }
                else
                    designKnobGrid_.push_back(nullptr);
                continue;
            }

            if (juce::String(def.fieldSuffix) == "Mix")
            {
                if (mixKnob_ != nullptr)
                {
                    mixKnob_->setVisible(type != 0);
                    designKnobGrid_.push_back(mixKnob_.get());
                }
                else
                    designKnobGrid_.push_back(nullptr);
                continue;
            }

            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + def.fieldSuffix, def.label);
            knob->applyFigmaContext(figma::KnobContext::DesignFxDetail);
            knob->setShowNameLabel(false);
            knob->setShowModRouteRing(false);
            auto* raw = knob.get();
            panel_.addAndMakeVisible(*knob);
            paramKnobs_.push_back(std::move(knob));
            designKnobGrid_.push_back(raw);
        }

        syncDesignModeRowFromChip();
        updatePlayDashboardUi();

        if (mixKnob_ != nullptr)
        {
            mixKnob_->setShowNameLabel(false);
            mixKnob_->setShowModRouteRing(false);
        }
    }

    void FxChainStrip::syncDesignModeRowFromChip()
    {
        if (designModeRow_ == nullptr)
            return;

        const auto& spec = fxDesignSpecForChip(designFxChipIndex_);
        if (spec.modePillCount == 0)
        {
            designModeRow_->setVisible(false);
            return;
        }

        juce::StringArray pills;
        for (std::size_t i = 0; i < spec.modePillCount && i < spec.modePills.size(); ++i)
        {
            if (spec.modePills[i] != nullptr)
                pills.add(spec.modePills[i]);
        }
        designModeRow_->setVisible(designFxPageMode_ && !pills.isEmpty());
        designModeRow_->setValues(pills);

        juce::String selected = pills.isEmpty() ? juce::String() : pills[0];
        const auto& prefix = selectedSlot().paramPrefix;
        if (designFxChipIndex_ == 1)
            selected = saturationDesignPillFromCharacter(readIntParam(apvts_, prefix + "SaturationCharacter"));
        else if (designFxChipIndex_ == 7)
        {
            const int character = readIntParam(apvts_, prefix + "ReverbCharacter");
            const float modDepth = apvts_.getRawParameterValue(prefix + "ReverbModDepth") != nullptr
                                       ? apvts_.getRawParameterValue(prefix + "ReverbModDepth")->load()
                                       : 0.0f;
            selected = reverbDesignPillFromCharacter(character, modDepth);
        }
        else if (designFxChipIndex_ == 9)
            selected = compDesignPillFromCharacter(readIntParam(apvts_, prefix + "CompCharacter"));
        else if (designFxChipIndex_ == 4 && designFxUiState_ != nullptr)
            selected = designFxUiState_->moodPill();

        if (!selected.isEmpty() && pills.contains(selected))
            designModeRow_->setSelectedValue(selected);
        else if (!pills.isEmpty())
            designModeRow_->setSelectedValue(pills[0]);
    }

    void FxChainStrip::applyDesignModePill(const juce::String& pill)
    {
        const auto& prefix = selectedSlot().paramPrefix;

        if (designFxChipIndex_ == 7)
        {
            setIntParam(apvts_, prefix + "ReverbCharacter", reverbCharacterFromDesignPill(pill));
            if (pill == "SHIMMER")
            {
                if (auto* param = apvts_.getParameter(prefix + "ReverbModDepth"))
                    param->setValueNotifyingHost(param->convertTo0to1(0.85f));
                if (auto* param = apvts_.getParameter(prefix + "ReverbHighRatio"))
                    param->setValueNotifyingHost(param->convertTo0to1(0.95f));
            }
        }
        else if (designFxChipIndex_ == 1)
            setIntParam(apvts_, prefix + "SaturationCharacter", saturationCharacterFromDesignPill(pill));
        else if (designFxChipIndex_ == 9)
            setIntParam(apvts_, prefix + "CompCharacter", compCharacterFromDesignPill(pill));
        else if (designFxChipIndex_ == 4 && designFxUiState_ != nullptr)
        {
            designFxUiState_->setMoodPill(pill);
            applyMoodKnobsToEq(apvts_, prefix, *designFxUiState_);
        }

        if (onDesignModeChanged)
            onDesignModeChanged(pill);
    }

    void FxChainStrip::setSelectedEffectType(int typeOrdinal)
    {
        setEffectType(apvts_, selectedSlot().paramPrefix, typeOrdinal);
        if (typeOrdinal == 11)
        {
            const juce::String prefix = selectedSlot().paramPrefix;
            if (auto* raw = apvts_.getRawParameterValue(prefix + "VocoderScGainDb"))
            {
                if (raw->load() < 1.0f)
                {
                    if (auto* param = apvts_.getParameter(prefix + "VocoderScGainDb"))
                        param->setValueNotifyingHost(param->convertTo0to1(18.0f));
                }
            }
            if (auto* raw = apvts_.getRawParameterValue(prefix + "Mix"))
            {
                if (raw->load() < 0.05f)
                {
                    if (auto* param = apvts_.getParameter(prefix + "Mix"))
                        param->setValueNotifyingHost(param->convertTo0to1(0.85f));
                }
            }
        }
        if (typeOrdinal != 0)
            selectedSlot().lastEnabledType = typeOrdinal;
        enableButton_->setToggleState(typeOrdinal != 0, juce::dontSendNotification);
        rebuildParamKnobs();
        wireframe_.bindToSlot(selectedSlot().paramPrefix);
        refreshSelectorStates();
        refreshCompressorUi();
        refreshTransformerUi();
        refreshDelaySyncUi();
        syncTypeRowFromParams();
        syncCompressorRowsFromParams();
        syncTransformerRowsFromParams();
        syncDelaySyncRowsFromParams();
        resized();
    }

    void FxChainStrip::timerCallback()
    {
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        const auto& spec = fxPlaySpecForType(type);
        slotTitleLabel_.setText(selectedSlot().shortLabel + " · " + spec.name, juce::dontSendNotification);
        enableButton_->setToggleState(type != 0, juce::dontSendNotification);
        if (type != 0)
            selectedSlot().lastEnabledType = type;
        refreshSelectorStates();
        refreshCompressorUi();
        refreshTransformerUi();
        refreshDelaySyncUi();
        syncTypeRowFromParams();
        syncCompressorRowsFromParams();
        syncTransformerRowsFromParams();
        syncDelaySyncRowsFromParams();
        swapLeft_.setEnabled(canSwapSelectedSlot(-1));
        swapRight_.setEnabled(canSwapSelectedSlot(1));
        updateGainReductionLabel();
        updatePlayDashboardUi();
    }

    void FxChainStrip::syncTypeRowFromParams()
    {
        if (typeRow_ == nullptr)
            return;
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        typeRow_->setSelectedValue(type == 0 ? juce::String() : fxChipLabelForType(type));
    }

    void FxChainStrip::refreshTransformerUi()
    {
        const bool show = showsCompressorControls();
        if (transCoreRow_ != nullptr)
            transCoreRow_->setVisible(show);
        if (transBrandRow_ != nullptr)
            transBrandRow_->setVisible(show);

        if (show && transAmountKnob_ == nullptr)
        {
            transAmountKnob_ =
                std::make_unique<GlowKnob>(apvts_, selectedSlot().paramPrefix + "CompTransformerAmount", "Trans");
            transAmountKnob_->applyFigmaContext(figma::KnobContext::DesignFxDetail);
            panel_.addAndMakeVisible(*transAmountKnob_);
        }
        else if (!show)
        {
            transAmountKnob_.reset();
        }
    }

    void FxChainStrip::refreshCompressorUi()
    {
        const bool show = showsCompressorControls();
        if (compCharacterRow_ != nullptr)
            compCharacterRow_->setVisible(show);
        if (compAutoMakeupRow_ != nullptr)
            compAutoMakeupRow_->setVisible(show);
        grMeterLabel_.setVisible(show);
    }

    void FxChainStrip::syncCompressorRowsFromParams()
    {
        if (!showsCompressorControls())
            return;
        const auto& prefix = selectedSlot().paramPrefix;
        const int character = readIntParam(apvts_, prefix + "CompCharacter");
        if (compCharacterRow_ != nullptr)
        {
            const char* label = character == 1 ? "FET" : character == 2 ? "OPTO" : "VCA";
            compCharacterRow_->setSelectedValue(label);
        }
        if (compAutoMakeupRow_ != nullptr)
            compAutoMakeupRow_->setSelectedValue(readIntParam(apvts_, prefix + "CompAutoMakeup") != 0 ? "AUTO" : "MANUAL");
    }

    void FxChainStrip::updateGainReductionLabel()
    {
        if (!showsCompressorControls())
        {
            grMeterLabel_.setVisible(false);
            return;
        }

        float grDb = 0.0f;
        if (selectedSlotIndex_ >= 3)
            grDb = processor_.getMasterCompressorGainReductionDb(selectedSlotIndex_ - 3);
        else
            grDb = processor_.getInsertCompressorGainReductionDb(selectedSlotIndex_);

        grMeterLabel_.setVisible(true);
        grMeterLabel_.setText("GR " + juce::String(grDb, 1) + " dB", juce::dontSendNotification);
    }

    void FxChainStrip::syncTransformerRowsFromParams()
    {
        if (!showsCompressorControls())
            return;
        const auto& prefix = selectedSlot().paramPrefix;
        if (transCoreRow_ != nullptr)
            transCoreRow_->setSelectedValue(transformerCoreName(readIntParam(apvts_, prefix + "CompTransformerCore")));
        if (transBrandRow_ != nullptr)
            transBrandRow_->setSelectedValue(
                transformerBrandName(readIntParam(apvts_, prefix + "CompTransformerBrand")));
    }

    void FxChainStrip::refreshDelaySyncUi()
    {
        const bool show = showsDelaySyncControls();
        if (delaySyncRow_ != nullptr)
            delaySyncRow_->setVisible(show);
        if (delayDivisionRow_ != nullptr)
            delayDivisionRow_->setVisible(show && delaySyncRow_ != nullptr &&
                                          delaySyncRow_->getSelectedValue() == "TEMPO");
    }

    void FxChainStrip::syncDelaySyncRowsFromParams()
    {
        if (!showsDelaySyncControls())
            return;
        const auto& prefix = selectedSlot().paramPrefix;
        const bool syncOn = readIntParam(apvts_, prefix + "TapeDelaySync") != 0;
        const int div = readIntParam(apvts_, prefix + "TapeDelaySyncDivision");
        if (delaySyncRow_ != nullptr)
            delaySyncRow_->setSelectedValue(syncOn ? "TEMPO" : "FREE");
        if (delayDivisionRow_ != nullptr)
        {
            static constexpr const char* kLabels[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/4T", "1/8.", "1/8T"};
            const int idx = juce::jlimit(0, 8, div);
            delayDivisionRow_->setSelectedValue(kLabels[idx]);
        }
        refreshDelaySyncUi();
    }

    void FxChainStrip::setDelaySyncEnabled(bool enabled)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "TapeDelaySync", enabled ? 1 : 0);
        refreshDelaySyncUi();
    }

    void FxChainStrip::setDelaySyncDivision(int divisionIndex)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "TapeDelaySyncDivision", divisionIndex);
    }

    void FxChainStrip::setTransformerCore(int coreOrdinal)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "CompTransformerCore", coreOrdinal);
    }

    void FxChainStrip::setTransformerBrand(int brandOrdinal)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "CompTransformerBrand", brandOrdinal);
    }

    void FxChainStrip::setCompCharacter(int characterOrdinal)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "CompCharacter", characterOrdinal);
    }

    void FxChainStrip::setCompAutoMakeup(bool enabled)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "CompAutoMakeup", enabled ? 1 : 0);
    }

    void FxChainStrip::selectSlot(std::size_t index)
    {
        selectedSlotIndex_ = juce::jlimit<std::size_t>(0, slots_.size() - 1, index);
        chainFlow_.setSelectedSlot(selectedSlotIndex_);
        wireframe_.bindToSlot(selectedSlot().paramPrefix);

        if (mixKnob_ != nullptr)
        {
            mixKnob_.reset();
            mixKnob_ = std::make_unique<GlowKnob>(apvts_, selectedSlot().paramPrefix + "Mix", "Mix");
            mixKnob_->applyFigmaContext(figma::KnobContext::DesignFxDetail);
            panel_.addAndMakeVisible(*mixKnob_);
        }

        rebuildParamKnobs();
        refreshSelectorStates();
        refreshCompressorUi();
        refreshTransformerUi();
        refreshDelaySyncUi();
        syncTypeRowFromParams();
        syncCompressorRowsFromParams();
        syncTransformerRowsFromParams();
        syncDelaySyncRowsFromParams();

        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        slotTitleLabel_.setText(selectedSlot().shortLabel + " · " + fxPlaySpecForType(type).name,
                                juce::dontSendNotification);
        enableButton_->setToggleState(type != 0, juce::dontSendNotification);
        updatePlayDashboardUi();
        resized();
    }

    void FxChainStrip::toggleSelectedSlotEnabled()
    {
        auto& slot = selectedSlot();
        const int current = readEffectType(apvts_, slot.paramPrefix);
        if (current == 0)
            setSelectedEffectType(slot.lastEnabledType);
        else
        {
            slot.lastEnabledType = current;
            setSelectedEffectType(0);
        }
    }

    void FxChainStrip::refreshSelectorStates()
    {
        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            auto& slot = slots_[i];
            const int type = readEffectType(apvts_, slot.paramPrefix);
            slot.selector->setToggleState(type != 0, juce::dontSendNotification);
            slot.selector->setSelectionHighlight(i == selectedSlotIndex_);
            slot.selectorLabel.setColour(juce::Label::textColourId,
                                         i == selectedSlotIndex_ ? palette::kAccent : palette::kTextDim);
        }
    }

    void FxChainStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds();

        if (designFxPageMode_)
        {
            resizedDesignFxPage(content);
            return;
        }

        if (playDashboardMode_)
        {
            resizedDashboard(content);
            return;
        }

        helpLabel_.setBounds(content.removeFromTop(28));
        content.removeFromTop(4);
        chainFlow_.setBounds(content.removeFromTop(72));
        content.removeFromTop(6);

        auto selectorRow = content.removeFromTop(48);
        const int chipWidth = selectorRow.getWidth() / static_cast<int>(slots_.size());
        for (auto& slot : slots_)
        {
            auto chip = selectorRow.removeFromLeft(chipWidth).reduced(2);
            const int ringSize = juce::jmin(chip.getWidth(), 26);
            slot.selector->setBounds(chip.removeFromTop(ringSize).withSizeKeepingCentre(ringSize, ringSize));
            slot.selectorLabel.setBounds(chip.removeFromTop(14));
        }

        content.removeFromTop(6);
        auto main = content;

        auto wireBounds = main.removeFromLeft(static_cast<int>(main.getWidth() * 0.44f)).reduced(0, 2);
        wireframe_.setBounds(wireBounds);

        main = main.reduced(6, 2);
        slotTitleLabel_.setBounds(main.removeFromTop(20));
        main.removeFromTop(4);

        if (typeRow_ != nullptr)
            typeRow_->setBounds(main.removeFromTop(34));
        main.removeFromTop(4);

        if (playDashboardMode_ && (vocoderLabChip_->isVisible() || quasarLabChip_->isVisible()))
        {
            if (quasarLabChip_->isVisible())
                quasarLabChip_->setBounds(main.removeFromTop(28).reduced(2));
            else
                vocoderLabChip_->setBounds(main.removeFromTop(28).reduced(2));
            main.removeFromTop(4);
        }

        auto topRow = main.removeFromTop(36);
        const int enableSize = 32;
        enableButton_->setBounds(topRow.removeFromLeft(enableSize + 6).withSizeKeepingCentre(enableSize, enableSize));
        swapLeft_.setBounds(topRow.removeFromLeft(64));
        topRow.removeFromLeft(4);
        swapRight_.setBounds(topRow.removeFromLeft(64));

        const bool showComp = showsCompressorControls();
        if (showComp)
        {
            main.removeFromTop(6);
            if (compCharacterRow_ != nullptr)
                compCharacterRow_->setBounds(main.removeFromTop(34));
            main.removeFromTop(4);
            if (compAutoMakeupRow_ != nullptr)
                compAutoMakeupRow_->setBounds(main.removeFromTop(34));
            main.removeFromTop(4);
            grMeterLabel_.setBounds(main.removeFromTop(18));
            main.removeFromTop(4);
        }

        const bool showTrans = showComp;
        if (showTrans)
        {
            main.removeFromTop(6);
            if (transCoreRow_ != nullptr)
                transCoreRow_->setBounds(main.removeFromTop(34));
            main.removeFromTop(4);
            if (transBrandRow_ != nullptr)
                transBrandRow_->setBounds(main.removeFromTop(34));
            main.removeFromTop(6);
        }

        const bool showDelaySync = showsDelaySyncControls();
        if (showDelaySync)
        {
            main.removeFromTop(4);
            if (delaySyncRow_ != nullptr)
                delaySyncRow_->setBounds(main.removeFromTop(34));
            if (delayDivisionRow_ != nullptr && delayDivisionRow_->isVisible())
            {
                main.removeFromTop(4);
                delayDivisionRow_->setBounds(main.removeFromTop(34));
            }
            main.removeFromTop(4);
        }

        auto knobRow = main.removeFromTop(96);
        const int knobCount = static_cast<int>(paramKnobs_.size()) + (mixKnob_ != nullptr ? 1 : 0);
        if (knobCount > 0)
        {
            const int knobW = knobRow.getWidth() / knobCount;
            for (auto& knob : paramKnobs_)
                knob->setBounds(knobRow.removeFromLeft(knobW).reduced(3));
            if (mixKnob_ != nullptr)
                mixKnob_->setBounds(knobRow.removeFromLeft(knobW).reduced(3));
        }

        if (showTrans && transAmountKnob_ != nullptr)
            transAmountKnob_->setBounds(main.removeFromTop(88).removeFromLeft(72).reduced(3));
    }

    void FxChainStrip::resizedDashboard(juce::Rectangle<int> content)
    {
        chainFlow_.setBounds(content.removeFromTop(layout::kFxChainFlowHeight));
        content.removeFromTop(layout::kFxChainEditorGap);

        auto editor = content.removeFromTop(layout::kFxSlotEditorHeight);

        if (vocoderLabChip_->isVisible() || quasarLabChip_->isVisible())
        {
            if (quasarLabChip_->isVisible())
                quasarLabChip_->setBounds(editor.removeFromTop(layout::kLabChipHeight).reduced(2));
            else
                vocoderLabChip_->setBounds(editor.removeFromTop(layout::kLabChipHeight).reduced(2));
            editor.removeFromTop(4);
        }

        if (typeRow_ != nullptr && typeRow_->isVisible())
        {
            typeRow_->setBounds(editor.removeFromTop(30));
            editor.removeFromTop(4);
        }

        auto topRow = editor.removeFromTop(26);
        const int enableSize = 26;
        enableButton_->setBounds(topRow.removeFromLeft(enableSize + 4).withSizeKeepingCentre(enableSize, enableSize));

        auto knobRow = editor;
        const int knobCount = static_cast<int>(paramKnobs_.size()) + (mixKnob_ != nullptr && mixKnob_->isVisible() ? 1 : 0);
        if (knobCount > 0)
        {
            const int knobH = juce::jmin(64, knobRow.getHeight());
            knobRow = knobRow.removeFromTop(knobH);
            const int knobW = knobRow.getWidth() / knobCount;
            for (auto& knob : paramKnobs_)
            {
                if (knob->isVisible())
                {
                    knob->setMaxDialDiameter(juce::jmin(36, knobW - 4));
                    knob->setBounds(knobRow.removeFromLeft(knobW).reduced(2));
                }
            }
            if (mixKnob_ != nullptr && mixKnob_->isVisible())
            {
                mixKnob_->setMaxDialDiameter(juce::jmin(36, knobRow.getWidth() - 4));
                mixKnob_->setBounds(knobRow.removeFromLeft(knobRow.getWidth()).reduced(2));
            }
        }

        wireframe_.setBounds({});
        slotTitleLabel_.setBounds({});
        swapLeft_.setBounds({});
        swapRight_.setBounds({});
        if (transAmountKnob_ != nullptr)
            transAmountKnob_->setBounds({});
    }

    void FxChainStrip::resizedDesignFxPage(juce::Rectangle<int> content)
    {
        wireframe_.setVisible(false);
        if (typeRow_ != nullptr)
            typeRow_->setVisible(false);
        if (enableButton_ != nullptr)
            enableButton_->setVisible(false);
        if (compCharacterRow_ != nullptr)
            compCharacterRow_->setVisible(false);
        if (compAutoMakeupRow_ != nullptr)
            compAutoMakeupRow_->setVisible(false);
        if (transCoreRow_ != nullptr)
            transCoreRow_->setVisible(false);
        if (transBrandRow_ != nullptr)
            transBrandRow_->setVisible(false);
        if (delaySyncRow_ != nullptr)
            delaySyncRow_->setVisible(false);
        if (delayDivisionRow_ != nullptr)
            delayDivisionRow_->setVisible(false);
        grMeterLabel_.setVisible(false);

        const bool eqSidebar = designFxChipIndex_ == 8;
        auto controls = eqSidebar ? content : content.removeFromLeft(layout::kDesignFxPageDetailControlsWidth);

        if (eqSidebar)
        {
            designKnobGridBounds_ = controls;
            const int knobW = juce::jmin(controls.getWidth(), layout::kDesignFxPageDetailKnobWidth);
            const int knobH = layout::kDesignFxPageDetailKnobHeight;
            const int rowStep = knobH + 6;
            std::size_t row = 0;

            for (std::size_t i = 0; i < designKnobGrid_.size(); ++i)
            {
                if (designKnobGrid_[i] == nullptr || !designKnobGrid_[i]->isVisible())
                    continue;
                designKnobGrid_[i]->setMaxDialDiameter(juce::jmin(28, knobW - 4));
                designKnobGrid_[i]->setBounds(controls.getX(), controls.getY() + static_cast<int>(row) * rowStep, knobW,
                                              knobH);
                ++row;
            }

            if (designModeRow_ != nullptr)
                designModeRow_->setVisible(false);
        }
        else
        {
            designKnobGridBounds_ = controls.removeFromTop(layout::kDesignFxPageDetailKnobGridHeight);
            auto knobGrid = designKnobGridBounds_;

            const int knobW = layout::kDesignFxPageDetailKnobWidth;
            const int knobH = layout::kDesignFxPageDetailKnobHeight;
            const int colStep = knobW + layout::kDesignFxPageDetailKnobColGap;
            const int rowStep = knobH + (layout::kDesignFxPageDetailKnobRowGap - knobH);

            auto placeKnob = [&](GlowKnob* knob, std::size_t index) {
                const int col = static_cast<int>(index % 3);
                const int row = static_cast<int>(index / 3);
                const juce::Rectangle<int> cell(knobGrid.getX() + col * colStep, knobGrid.getY() + row * rowStep, knobW,
                                                knobH);

                if (knob != nullptr && knob->isVisible())
                {
                    knob->setMaxDialDiameter(layout::kDesignFxPageDetailKnobDialSize);
                    knob->setBounds(cell);
                }
            };

            for (std::size_t i = 0; i < designKnobGrid_.size(); ++i)
                placeKnob(designKnobGrid_[i], i);

            controls.removeFromTop(8);
            if (designModeRow_ != nullptr && designModeRow_->isVisible())
                designModeRow_->setBounds(controls.removeFromTop(layout::kDesignFxPageDetailModeStripHeight));
        }

        if (quasarLabChip_ != nullptr && quasarLabChip_->isVisible())
            quasarLabChip_->setBounds(controls.removeFromTop(layout::kLabChipHeight).reduced(0, 2));
        else if (vocoderLabChip_ != nullptr && vocoderLabChip_->isVisible())
            vocoderLabChip_->setBounds(controls.removeFromTop(layout::kLabChipHeight).reduced(0, 2));

        chainFlow_.setBounds({});
        helpLabel_.setBounds({});
        slotTitleLabel_.setBounds({});
        swapLeft_.setBounds({});
        swapRight_.setBounds({});
        wireframe_.setBounds({});
        if (transAmountKnob_ != nullptr)
            transAmountKnob_->setBounds({});
    }

    void FxChainStrip::paintOverChildren(juce::Graphics& g)
    {
        if (!designFxPageMode_ || designKnobGridBounds_.isEmpty())
            return;

        const auto& spec = fxDesignSpecForChip(designFxChipIndex_);
        const int knobW = layout::kDesignFxPageDetailKnobWidth;
        const int knobH = layout::kDesignFxPageDetailKnobHeight;
        const int colStep = knobW + layout::kDesignFxPageDetailKnobColGap;
        const int rowStep = knobH + (layout::kDesignFxPageDetailKnobRowGap - knobH);
        const bool eqSidebar = designFxChipIndex_ == 8;

        for (std::size_t i = 0; i < designKnobGrid_.size() && i < spec.knobs.size(); ++i)
        {
            auto* knob = designKnobGrid_[i];
            if (knob == nullptr || !knob->isVisible())
                continue;

            juce::Rectangle<int> cell;
            if (eqSidebar)
            {
                const int visibleBefore = static_cast<int>(std::count_if(
                    designKnobGrid_.begin(), designKnobGrid_.begin() + static_cast<std::ptrdiff_t>(i),
                    [](GlowKnob* k) { return k != nullptr && k->isVisible(); }));
                cell = juce::Rectangle<int>(designKnobGridBounds_.getX(),
                                            designKnobGridBounds_.getY() + visibleBefore * (knobH + 6), knobW, knobH);
            }
            else
            {
                const int col = static_cast<int>(i % 3);
                const int row = static_cast<int>(i / 3);
                cell = juce::Rectangle<int>(designKnobGridBounds_.getX() + col * colStep,
                                            designKnobGridBounds_.getY() + row * rowStep, knobW, knobH);
            }
            cell = getLocalArea(&panel_, cell);

            if (spec.knobs[i].label != nullptr && spec.knobs[i].label[0] != '\0')
            {
                g.setColour(palette::kFigmaFxMutedText);
                g.setFont(fonts::micro(7.0f));
                g.drawText(spec.knobs[i].label, cell.removeFromTop(10), juce::Justification::centred);
            }

            g.setColour(palette::kAccent);
            g.setFont(fonts::denseBold(8.0f));
            g.drawText(knob->getValueDisplayText(), cell.removeFromBottom(14), juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui
