#include "GlobalPanel.h"

#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "FxEffectPlayParams.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] int readEffectTypeRaw(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        void setEffectTypeRaw(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, int typeOrdinal)
        {
            if (auto* param = apvts.getParameter(prefix + "Type"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(typeOrdinal)));
        }

        [[nodiscard]] juce::String masterPrefix(std::size_t localIndex)
        {
            return masterFxParamId(localIndex, "");
        }
    } // namespace

    GlobalPanel::GlobalPanel(MurmurProcessor& processor)
        : processor_(processor), apvts_(processor.apvts), chainFlow_(apvts_)
    {
        addAndMakeVisible(panel_);

        for (auto* btn : {&chainTab_, &outputTab_})
        {
            btn->setClickingTogglesState(true);
            btn->setRadioGroupId(9100);
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::buttonOnColourId, palette::kAccent.withAlpha(0.28f));
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn->setColour(juce::TextButton::textColourOnId, palette::kTextPrimary);
            panel_.addAndMakeVisible(*btn);
        }
        chainTab_.onClick = [this] { setSubTab(SubTab::Chain); };
        outputTab_.onClick = [this] { setSubTab(SubTab::Output); };

        chainHelp_.setText(
            "Master bus M1–M4 after voice sum. Reorder in FX tab inserts; here focus on global chain slots.",
            juce::dontSendNotification);
        chainHelp_.setJustificationType(juce::Justification::centredLeft);
        chainHelp_.setFont(fonts::value(10.0f));
        chainHelp_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addChildComponent(chainHelp_);

        panel_.addChildComponent(chainFlow_);
        chainFlow_.onSlotSelected = [this](std::size_t index) {
            if (index >= 3)
                selectMasterSlot(index - 3);
        };

        for (std::size_t i = 0; i < masterSelectors_.size(); ++i)
        {
            const auto label = "M" + juce::String(static_cast<int>(i + 1));
            masterSelectors_[i] = std::make_unique<GlowRingButton>(label);
            masterSelectors_[i]->onClick = [this, i]() { selectMasterSlot(i); };
            panel_.addChildComponent(*masterSelectors_[i]);

            masterLabels_[i].setText(label, juce::dontSendNotification);
            masterLabels_[i].setJustificationType(juce::Justification::centred);
            masterLabels_[i].setFont(fonts::label(8.5f));
            masterLabels_[i].setColour(juce::Label::textColourId, palette::kTextDim);
            panel_.addChildComponent(masterLabels_[i]);
        }

        chainTypeRow_ = std::make_unique<MetadataFacetRow>("TYPE");
        chainTypeRow_->setValues(
            juce::StringArray{"SATUR", "CHORUS", "TAPE", "NODE", "FSHF", "FRACT", "REVERB", "EQ", "COMP", "LIMIT", "VOCODER"});
        chainTypeRow_->onChange = [this]() {
            const auto chip = chainTypeRow_->getSelectedValue();
            if (chip.isEmpty())
                return;
            setEffectTypeRaw(apvts_, masterPrefix(selectedMasterSlot_), fxTypeOrdinalFromChipLabel(chip));
            refreshFromParams();
        };
        panel_.addChildComponent(*chainTypeRow_);

        chainSlotTitle_.setJustificationType(juce::Justification::centredLeft);
        chainSlotTitle_.setFont(fonts::label(12.0f));
        chainSlotTitle_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        panel_.addChildComponent(chainSlotTitle_);

        chainEnable_ = std::make_unique<GlowRingButton>("ON");
        chainEnable_->onClick = [this]() {
            const auto prefix = masterPrefix(selectedMasterSlot_);
            const int current = readEffectTypeRaw(apvts_, prefix);
            if (current == 0)
                setEffectTypeRaw(apvts_, prefix, 7);
            else
                setEffectTypeRaw(apvts_, prefix, 0);
            refreshFromParams();
        };
        panel_.addChildComponent(*chainEnable_);

        outputHelp_.setText("Master output level before DAW. Limiter ceiling reads from the active LIMITER master slot.",
                            juce::dontSendNotification);
        outputHelp_.setJustificationType(juce::Justification::centredLeft);
        outputHelp_.setFont(fonts::value(10.0f));
        outputHelp_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addChildComponent(outputHelp_);

        masterGainKnob_ =
            std::make_unique<GlowKnob>(apvts_, kMasterGainId, "Master Vol", nullptr, palette::kAccentWarm);
        masterGainKnob_->applyFigmaContext(figma::KnobContext::ChromeMaster);
        panel_.addChildComponent(*masterGainKnob_);

        limiterCeilKnob_ = std::make_unique<GlowKnob>(apvts_, masterFxParamId(3, "LimiterCeilingDb"), "Limiter Ceil");
        limiterCeilKnob_->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        panel_.addChildComponent(*limiterCeilKnob_);

        std::array<juce::String, 7> prefixes{};
        for (std::size_t i = 0; i < 3; ++i)
            prefixes[i] = insertFxParamId(i, "");
        for (std::size_t i = 0; i < 4; ++i)
            prefixes[i + 3] = masterPrefix(i);
        chainFlow_.setSlotPrefixes(prefixes);
        chainFlow_.setSelectedSlot(selectedMasterSlot_ + 3);

        selectMasterSlot(2);
        setSubTab(SubTab::Chain);
        startTimerHz(8);
    }

    GlobalPanel::~GlobalPanel() { stopTimer(); }

    int GlobalPanel::readEffectType(const juce::String& prefix) const
    {
        return readEffectTypeRaw(apvts_, prefix);
    }

    void GlobalPanel::selectMasterSlot(std::size_t localIndex)
    {
        selectedMasterSlot_ = juce::jlimit<std::size_t>(0, 3, localIndex);
        chainFlow_.setSelectedSlot(selectedMasterSlot_ + 3);

        const auto prefix = masterPrefix(selectedMasterSlot_);
        chainMixKnob_.reset();
        chainMixKnob_ = std::make_unique<GlowKnob>(apvts_, prefix + "Mix", "Mix");
        chainMixKnob_->applyFigmaContext(figma::KnobContext::DesignFxDetail);
        panel_.addChildComponent(*chainMixKnob_);

        chainParamKnobs_.clear();
        const int type = readEffectType(prefix);
        const auto& spec = fxPlaySpecForType(type);
        for (const auto& def : spec.params)
        {
            if (def.fieldSuffix == nullptr || def.fieldSuffix[0] == '\0')
                continue;
            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + def.fieldSuffix, def.label);
            knob->applyFigmaContext(figma::KnobContext::DesignFxDetail);
            panel_.addChildComponent(*knob);
            chainParamKnobs_.push_back(std::move(knob));
        }

        refreshFromParams();
        resized();
    }

    void GlobalPanel::refreshFromParams()
    {
        const auto prefix = masterPrefix(selectedMasterSlot_);
        const int type = readEffectType(prefix);
        chainSlotTitle_.setText("M" + juce::String(static_cast<int>(selectedMasterSlot_ + 1)) + " · " +
                                    fxPlaySpecForType(type).name,
                                juce::dontSendNotification);
        chainEnable_->setToggleState(type != 0, juce::dontSendNotification);
        if (chainTypeRow_ != nullptr)
            chainTypeRow_->setSelectedValue(type == 0 ? juce::String() : fxChipLabelForType(type));

        for (std::size_t i = 0; i < masterSelectors_.size(); ++i)
        {
            masterSelectors_[i]->setSelectionHighlight(i == selectedMasterSlot_);
            masterLabels_[i].setColour(juce::Label::textColourId,
                                       i == selectedMasterSlot_ ? palette::kAccent : palette::kTextDim);
        }
    }

    void GlobalPanel::setSubTab(SubTab tab)
    {
        subTab_ = tab;
        chainTab_.setToggleState(tab == SubTab::Chain, juce::dontSendNotification);
        outputTab_.setToggleState(tab == SubTab::Output, juce::dontSendNotification);

        const bool chain = tab == SubTab::Chain;
        const bool output = tab == SubTab::Output;

        chainHelp_.setVisible(chain);
        chainFlow_.setVisible(chain);
        for (auto& sel : masterSelectors_)
            sel->setVisible(chain);
        for (auto& lbl : masterLabels_)
            lbl.setVisible(chain);
        if (chainTypeRow_ != nullptr)
            chainTypeRow_->setVisible(chain);
        chainSlotTitle_.setVisible(chain);
        chainEnable_->setVisible(chain);
        if (chainMixKnob_ != nullptr)
            chainMixKnob_->setVisible(chain);
        for (auto& knob : chainParamKnobs_)
            knob->setVisible(chain);

        outputHelp_.setVisible(output);
        if (masterGainKnob_ != nullptr)
            masterGainKnob_->setVisible(output);
        if (limiterCeilKnob_ != nullptr)
            limiterCeilKnob_->setVisible(output);

        resized();
    }

    void GlobalPanel::timerCallback() { refreshFromParams(); }

    void GlobalPanel::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds();

        auto tabRow = content.removeFromTop(28);
        const int tabW = tabRow.getWidth() / 2;
        chainTab_.setBounds(tabRow.removeFromLeft(tabW).reduced(2));
        outputTab_.setBounds(tabRow.reduced(2));
        content.removeFromTop(6);

        if (subTab_ == SubTab::Chain)
        {
            chainHelp_.setBounds(content.removeFromTop(28));
            content.removeFromTop(4);
            chainFlow_.setBounds(content.removeFromTop(72));
            content.removeFromTop(6);

            auto selRow = content.removeFromTop(48);
            const int chipW = selRow.getWidth() / 4;
            for (std::size_t i = 0; i < 4; ++i)
            {
                auto chip = selRow.removeFromLeft(chipW).reduced(2);
                const int ring = juce::jmin(chip.getWidth(), 26);
                masterSelectors_[i]->setBounds(chip.removeFromTop(ring).withSizeKeepingCentre(ring, ring));
                masterLabels_[i].setBounds(chip.removeFromTop(14));
            }

            content.removeFromTop(6);
            chainSlotTitle_.setBounds(content.removeFromTop(20));
            content.removeFromTop(4);
            if (chainTypeRow_ != nullptr)
                chainTypeRow_->setBounds(content.removeFromTop(34));
            content.removeFromTop(4);

            auto topRow = content.removeFromTop(36);
            chainEnable_->setBounds(topRow.removeFromLeft(38).withSizeKeepingCentre(32, 32));

            auto knobRow = content.removeFromTop(96);
            const int knobCount = static_cast<int>(chainParamKnobs_.size()) + (chainMixKnob_ != nullptr ? 1 : 0);
            if (knobCount > 0)
            {
                const int kw = knobRow.getWidth() / knobCount;
                for (auto& knob : chainParamKnobs_)
                    knob->setBounds(knobRow.removeFromLeft(kw).reduced(3));
                if (chainMixKnob_ != nullptr)
                    chainMixKnob_->setBounds(knobRow.removeFromLeft(kw).reduced(3));
            }
            return;
        }

        if (subTab_ == SubTab::Output)
        {
            outputHelp_.setBounds(content.removeFromTop(36));
            content.removeFromTop(8);
            auto knobRow = content.removeFromTop(96);
            if (masterGainKnob_ != nullptr)
                masterGainKnob_->setBounds(knobRow.removeFromLeft(96).reduced(4));
            if (limiterCeilKnob_ != nullptr)
                limiterCeilKnob_->setBounds(knobRow.removeFromLeft(96).reduced(4));
        }
    }

} // namespace pw8::plugin::ui
