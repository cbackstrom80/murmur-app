#include "GlobalPanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
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

        [[nodiscard]] int readIntParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }
    } // namespace

    GlobalPanel::GlobalPanel(PatchworkEightProcessor& processor)
        : processor_(processor), apvts_(processor.apvts), chainFlow_(apvts_)
    {
        addAndMakeVisible(panel_);

        for (auto* btn : {&chainTab_, &quasarTab_, &outputTab_})
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
        quasarTab_.onClick = [this] { setSubTab(SubTab::Quasar); };
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
            juce::StringArray{"SATUR", "CHORUS", "TAPE", "NODE", "FSHF", "FRACT", "REVERB", "EQ", "COMP", "LIMIT", "QUASAR"});
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
                setEffectTypeRaw(apvts_, prefix, 11);
            else
                setEffectTypeRaw(apvts_, prefix, 0);
            refreshFromParams();
        };
        panel_.addChildComponent(*chainEnable_);

        quasarHelp_.setText(
            "Headphone-first binaural spatial mixer. CNTR keeps bass mono-stable; QSR1/QSR2 orbit outside the head. "
            "Freeze delay at 100% feedback.",
            juce::dontSendNotification);
        quasarHelp_.setJustificationType(juce::Justification::centredLeft);
        quasarHelp_.setFont(fonts::value(10.0f));
        quasarHelp_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addAndMakeVisible(quasarHelp_);

        quasarScopeLabel_.setText("Spherical scope — Phase 4", juce::dontSendNotification);
        quasarScopeLabel_.setJustificationType(juce::Justification::centred);
        quasarScopeLabel_.setFont(fonts::label(11.0f));
        quasarScopeLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        quasarScopeLabel_.setColour(juce::Label::backgroundColourId, palette::kPanelRaised);
        panel_.addAndMakeVisible(quasarScopeLabel_);

        quasarSlotLabel_.setJustificationType(juce::Justification::centredLeft);
        quasarSlotLabel_.setFont(fonts::label(11.0f));
        quasarSlotLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        panel_.addAndMakeVisible(quasarSlotLabel_);

        quasarDelaySyncRow_ = std::make_unique<MetadataFacetRow>("DLY SYNC");
        quasarDelaySyncRow_->setValues(juce::StringArray{"FREE", "TEMPO"});
        quasarDelaySyncRow_->onChange = [this]() {
            if (quasarPrefix_.isEmpty())
                return;
            const bool sync = quasarDelaySyncRow_->getSelectedValue() == "TEMPO";
            if (auto* param = apvts_.getParameter(quasarPrefix_ + "QuasarDelaySync"))
                param->setValueNotifyingHost(param->convertTo0to1(sync ? 1.0f : 0.0f));
            if (quasarDelayDivisionRow_ != nullptr)
                quasarDelayDivisionRow_->setVisible(sync);
            resized();
        };
        panel_.addAndMakeVisible(*quasarDelaySyncRow_);

        quasarDelayDivisionRow_ = std::make_unique<MetadataFacetRow>("DLY DIV");
        quasarDelayDivisionRow_->setValues(
            juce::StringArray{"1/1", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/4T", "1/8.", "1/8T"});
        quasarDelayDivisionRow_->onChange = [this]() {
            if (quasarPrefix_.isEmpty())
                return;
            const auto& v = quasarDelayDivisionRow_->getSelectedValue();
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
            if (auto* param = apvts_.getParameter(quasarPrefix_ + "QuasarDelaySyncDivision"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(idx)));
        };
        panel_.addAndMakeVisible(*quasarDelayDivisionRow_);

        outputHelp_.setText("Master output level before DAW. Limiter ceiling reads from the active LIMITER master slot.",
                            juce::dontSendNotification);
        outputHelp_.setJustificationType(juce::Justification::centredLeft);
        outputHelp_.setFont(fonts::value(10.0f));
        outputHelp_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addChildComponent(outputHelp_);

        masterGainKnob_ =
            std::make_unique<GlowKnob>(apvts_, kMasterGainId, "Master Vol", nullptr, palette::kAccentWarm);
        panel_.addChildComponent(*masterGainKnob_);

        limiterCeilKnob_ = std::make_unique<GlowKnob>(apvts_, masterFxParamId(3, "LimiterCeilingDb"), "Limiter Ceil");
        panel_.addChildComponent(*limiterCeilKnob_);

        std::array<juce::String, 7> prefixes{};
        for (std::size_t i = 0; i < 3; ++i)
            prefixes[i] = insertFxParamId(i, "");
        for (std::size_t i = 0; i < 4; ++i)
            prefixes[i + 3] = masterPrefix(i);
        chainFlow_.setSlotPrefixes(prefixes);
        chainFlow_.setSelectedSlot(selectedMasterSlot_ + 3);

        selectMasterSlot(2);
        setSubTab(SubTab::Quasar);
        startTimerHz(8);
    }

    GlobalPanel::~GlobalPanel() { stopTimer(); }

    int GlobalPanel::readEffectType(const juce::String& prefix) const
    {
        return readEffectTypeRaw(apvts_, prefix);
    }

    int GlobalPanel::findQuasarMasterSlot() const
    {
        for (std::size_t i = 0; i < 4; ++i)
        {
            if (readEffectType(masterPrefix(i)) == 11)
                return static_cast<int>(i);
        }
        return -1;
    }

    void GlobalPanel::selectMasterSlot(std::size_t localIndex)
    {
        selectedMasterSlot_ = juce::jlimit<std::size_t>(0, 3, localIndex);
        chainFlow_.setSelectedSlot(selectedMasterSlot_ + 3);

        const auto prefix = masterPrefix(selectedMasterSlot_);
        chainMixKnob_.reset();
        chainMixKnob_ = std::make_unique<GlowKnob>(apvts_, prefix + "Mix", "Mix");
        panel_.addChildComponent(*chainMixKnob_);

        chainParamKnobs_.clear();
        const int type = readEffectType(prefix);
        const auto& spec = fxPlaySpecForType(type);
        for (const auto& def : spec.params)
        {
            if (def.fieldSuffix == nullptr || def.fieldSuffix[0] == '\0')
                continue;
            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + def.fieldSuffix, def.label);
            panel_.addChildComponent(*knob);
            chainParamKnobs_.push_back(std::move(knob));
        }

        refreshFromParams();
        resized();
    }

    void GlobalPanel::rebuildQuasarKnobs(const juce::String& prefix)
    {
        quasarKnobs_.clear();
        if (prefix.isEmpty())
            return;

        static constexpr std::array<std::pair<const char*, const char*>, 22> defs = {{
            {"Mix", "Mix"},
            {"CntrLevel", "CNTR"},
            {"Qsr1Level", "QSR1"},
            {"Qsr2Level", "QSR2"},
            {"Qsr1Distance", "Q1 Dist"},
            {"Qsr1Angle", "Q1 Angle"},
            {"Qsr1Height", "Q1 Height"},
            {"Qsr2Distance", "Q2 Dist"},
            {"Qsr2Angle", "Q2 Angle"},
            {"Qsr2Height", "Q2 Height"},
            {"Qsr1RoomAmount", "Q1 Room"},
            {"Qsr1RoomSize", "Q1 Size"},
            {"Qsr1RoomDamping", "Q1 Damp"},
            {"Qsr2RoomAmount", "Q2 Room"},
            {"Qsr2RoomSize", "Q2 Size"},
            {"Qsr2RoomDamping", "Q2 Damp"},
            {"QuasarDelayTimeMs", "Dly Time"},
            {"QuasarDelayFeedback", "Dly Fdbk"},
            {"QuasarDelayVolume", "Dly Vol"},
            {"QuasarOutputMode", "Output"},
            {"QuasarCrossfeed", "X-Feed"},
            {"InputSplitHpfHz", "Split HPF"},
        }};

        for (const auto& [suffix, label] : defs)
        {
            auto knob = std::make_unique<GlowKnob>(apvts_, prefix + suffix, label);
            panel_.addAndMakeVisible(*knob);
            quasarKnobs_.push_back(std::move(knob));
        }
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

        const int quasarSlot = findQuasarMasterSlot();
        const auto qPrefix = quasarSlot >= 0 ? masterPrefix(static_cast<std::size_t>(quasarSlot)) : juce::String{};
        if (quasarSlot >= 0 && qPrefix != quasarPrefix_)
        {
            quasarPrefix_ = qPrefix;
            rebuildQuasarKnobs(qPrefix);
        }

        if (quasarSlot >= 0)
        {
            quasarSlotLabel_.setText("Editing master M" + juce::String(quasarSlot + 1) + " QUASAR",
                                     juce::dontSendNotification);
            const bool syncOn =
                readIntParam(apvts_, qPrefix + "QuasarDelaySync") != 0;
            const int div = readIntParam(apvts_, qPrefix + "QuasarDelaySyncDivision");
            if (quasarDelaySyncRow_ != nullptr)
                quasarDelaySyncRow_->setSelectedValue(syncOn ? "TEMPO" : "FREE");
            if (quasarDelayDivisionRow_ != nullptr)
            {
                static constexpr const char* kLabels[] = {"1/1", "1/2", "1/4", "1/8", "1/16", "1/4.", "1/4T", "1/8.", "1/8T"};
                quasarDelayDivisionRow_->setSelectedValue(kLabels[juce::jlimit(0, 8, div)]);
                quasarDelayDivisionRow_->setVisible(subTab_ == SubTab::Quasar && syncOn);
            }
            for (auto& knob : quasarKnobs_)
                knob->setVisible(subTab_ == SubTab::Quasar);
        }
        else
        {
            quasarPrefix_.clear();
            quasarKnobs_.clear();
            quasarSlotLabel_.setText("No QUASAR slot — set a master slot TYPE to QUASAR in CHAIN.",
                                     juce::dontSendNotification);
        }
    }

    void GlobalPanel::setSubTab(SubTab tab)
    {
        subTab_ = tab;
        chainTab_.setToggleState(tab == SubTab::Chain, juce::dontSendNotification);
        quasarTab_.setToggleState(tab == SubTab::Quasar, juce::dontSendNotification);
        outputTab_.setToggleState(tab == SubTab::Output, juce::dontSendNotification);

        const bool chain = tab == SubTab::Chain;
        const bool quasar = tab == SubTab::Quasar;
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

        quasarHelp_.setVisible(quasar);
        quasarScopeLabel_.setVisible(quasar);
        quasarSlotLabel_.setVisible(quasar);
        if (quasarDelaySyncRow_ != nullptr)
            quasarDelaySyncRow_->setVisible(quasar && !quasarPrefix_.isEmpty());
        if (quasarDelayDivisionRow_ != nullptr)
            quasarDelayDivisionRow_->setVisible(quasar && !quasarPrefix_.isEmpty() &&
                                                 quasarDelaySyncRow_ != nullptr &&
                                                 quasarDelaySyncRow_->getSelectedValue() == "TEMPO");
        for (auto& knob : quasarKnobs_)
            knob->setVisible(quasar);

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
        const int tabW = tabRow.getWidth() / 3;
        chainTab_.setBounds(tabRow.removeFromLeft(tabW).reduced(2));
        quasarTab_.setBounds(tabRow.removeFromLeft(tabW).reduced(2));
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

        if (subTab_ == SubTab::Quasar)
        {
            quasarHelp_.setBounds(content.removeFromTop(36));
            content.removeFromTop(4);
            quasarScopeLabel_.setBounds(content.removeFromTop(120));
            content.removeFromTop(4);
            quasarSlotLabel_.setBounds(content.removeFromTop(18));
            content.removeFromTop(6);

            if (quasarDelaySyncRow_ != nullptr && quasarDelaySyncRow_->isVisible())
            {
                quasarDelaySyncRow_->setBounds(content.removeFromTop(34));
                content.removeFromTop(4);
            }
            if (quasarDelayDivisionRow_ != nullptr && quasarDelayDivisionRow_->isVisible())
            {
                quasarDelayDivisionRow_->setBounds(content.removeFromTop(34));
                content.removeFromTop(6);
            }

            if (!quasarKnobs_.empty())
            {
                const int cols = 6;
                const int rows = static_cast<int>((quasarKnobs_.size() + cols - 1) / cols);
                const int rowH = juce::jmin(88, content.getHeight() / juce::jmax(1, rows));
                std::size_t idx = 0;
                for (int r = 0; r < rows && content.getHeight() > 0; ++r)
                {
                    auto row = content.removeFromTop(rowH);
                    const int kw = row.getWidth() / cols;
                    for (int c = 0; c < cols && idx < quasarKnobs_.size(); ++c, ++idx)
                        quasarKnobs_[idx]->setBounds(row.removeFromLeft(kw).reduced(3));
                }
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
