#include "FxChainStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
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
            juce::StringArray{"SATUR", "CHORUS", "TAPE", "NODE", "FSHF", "FRACT", "REVERB", "EQ", "COMP", "LIMIT", "QUASAR", "VOCODER"});
        typeRow_->onChange = [this]() {
            const auto chip = typeRow_->getSelectedValue();
            if (chip.isEmpty())
                return;
            setSelectedEffectType(fxTypeOrdinalFromChipLabel(chip));
        };
        panel_.addAndMakeVisible(*typeRow_);

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
        return selectedSlotIndex_ >= 3 && readEffectType(apvts_, selectedSlot().paramPrefix) == 9;
    }

    bool FxChainStrip::showsDelaySyncControls() const
    {
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        return type == 3 || type == 11;
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
            panel_.addAndMakeVisible(*knob);
            paramKnobs_.push_back(std::move(knob));
        }

        resized();
    }

    void FxChainStrip::setSelectedEffectType(int typeOrdinal)
    {
        setEffectType(apvts_, selectedSlot().paramPrefix, typeOrdinal);
        if (typeOrdinal != 0)
            selectedSlot().lastEnabledType = typeOrdinal;
        enableButton_->setToggleState(typeOrdinal != 0, juce::dontSendNotification);
        rebuildParamKnobs();
        wireframe_.bindToSlot(selectedSlot().paramPrefix);
        refreshSelectorStates();
        refreshTransformerUi();
        refreshDelaySyncUi();
        syncTypeRowFromParams();
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
        refreshTransformerUi();
        refreshDelaySyncUi();
        syncTypeRowFromParams();
        syncTransformerRowsFromParams();
        syncDelaySyncRowsFromParams();
        swapLeft_.setEnabled(canSwapSelectedSlot(-1));
        swapRight_.setEnabled(canSwapSelectedSlot(1));
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
        const bool show = showsMasterCompressorControls();
        if (transCoreRow_ != nullptr)
            transCoreRow_->setVisible(show);
        if (transBrandRow_ != nullptr)
            transBrandRow_->setVisible(show);

        if (show && transAmountKnob_ == nullptr)
        {
            transAmountKnob_ =
                std::make_unique<GlowKnob>(apvts_, selectedSlot().paramPrefix + "CompTransformerAmount", "Trans");
            panel_.addAndMakeVisible(*transAmountKnob_);
        }
        else if (!show)
        {
            transAmountKnob_.reset();
        }
    }

    void FxChainStrip::syncTransformerRowsFromParams()
    {
        if (!showsMasterCompressorControls())
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
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        const auto& prefix = selectedSlot().paramPrefix;
        const bool syncOn = type == 3 ? readIntParam(apvts_, prefix + "TapeDelaySync") != 0
                                      : readIntParam(apvts_, prefix + "QuasarDelaySync") != 0;
        const int div = type == 3 ? readIntParam(apvts_, prefix + "TapeDelaySyncDivision")
                                  : readIntParam(apvts_, prefix + "QuasarDelaySyncDivision");
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
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        const auto suffix = type == 3 ? juce::String("TapeDelaySync") : juce::String("QuasarDelaySync");
        setIntParam(apvts_, selectedSlot().paramPrefix + suffix, enabled ? 1 : 0);
        refreshDelaySyncUi();
    }

    void FxChainStrip::setDelaySyncDivision(int divisionIndex)
    {
        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        const auto suffix =
            type == 3 ? juce::String("TapeDelaySyncDivision") : juce::String("QuasarDelaySyncDivision");
        setIntParam(apvts_, selectedSlot().paramPrefix + suffix, divisionIndex);
    }

    void FxChainStrip::setTransformerCore(int coreOrdinal)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "CompTransformerCore", coreOrdinal);
    }

    void FxChainStrip::setTransformerBrand(int brandOrdinal)
    {
        setIntParam(apvts_, selectedSlot().paramPrefix + "CompTransformerBrand", brandOrdinal);
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
            panel_.addAndMakeVisible(*mixKnob_);
        }

        rebuildParamKnobs();
        refreshSelectorStates();
        refreshTransformerUi();
        refreshDelaySyncUi();
        syncTypeRowFromParams();
        syncTransformerRowsFromParams();
        syncDelaySyncRowsFromParams();

        const int type = readEffectType(apvts_, selectedSlot().paramPrefix);
        slotTitleLabel_.setText(selectedSlot().shortLabel + " · " + fxPlaySpecForType(type).name,
                                juce::dontSendNotification);
        enableButton_->setToggleState(type != 0, juce::dontSendNotification);
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

        auto topRow = main.removeFromTop(36);
        const int enableSize = 32;
        enableButton_->setBounds(topRow.removeFromLeft(enableSize + 6).withSizeKeepingCentre(enableSize, enableSize));
        swapLeft_.setBounds(topRow.removeFromLeft(64));
        topRow.removeFromLeft(4);
        swapRight_.setBounds(topRow.removeFromLeft(64));

        const bool showTrans = showsMasterCompressorControls();
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

} // namespace pw8::plugin::ui
