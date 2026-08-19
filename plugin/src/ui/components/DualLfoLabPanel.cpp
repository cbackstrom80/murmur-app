#include "DualLfoLabPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ArpUiFormatters.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::String lfoWaveformToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "SINE";
                case 1: return "TRIANGLE";
                case 2: return "SAW";
                case 3: return "SQUARE";
                case 4: return "S & H";
                case 5: return "S.RANDOM";
                default: return juce::String(v);
            }
        }

        juce::String lfoModeToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "FREE";
                case 1: return "RETRIGGER";
                case 2: return "ONE SHOT";
                case 3: return "TEMPO SYNC";
                default: return juce::String(v);
            }
        }

        void stylePairTabButton(juce::TextButton& btn, const juce::String& text)
        {
            btn.setButtonText(text);
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
        }

        void highlightPairTabButton(juce::TextButton& btn, bool active)
        {
            btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.12f) : palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, active ? palette::kAccent : palette::kTextDim);
        }

        void styleSyncChipButton(juce::TextButton& btn, const juce::String& text)
        {
            btn.setButtonText(text);
            btn.setClickingTogglesState(false);
            btn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
        }

        void highlightSyncChipButton(juce::TextButton& btn, bool active, juce::Colour accent)
        {
            btn.setColour(juce::TextButton::buttonColourId, active ? accent.withAlpha(0.08f) : juce::Colours::transparentBlack);
            btn.setColour(juce::TextButton::textColourOffId, active ? accent : palette::kTextDim);
        }

        [[nodiscard]] int syncChipToDivisionIndex(int chipIndex) noexcept
        {
            static constexpr int kMap[] = {2, 3, 4, 5, 6, 7, 8};
            return kMap[juce::jlimit(0, 6, chipIndex)];
        }

        [[nodiscard]] int divisionIndexToSyncChip(int divisionIndex) noexcept
        {
            static constexpr int kMap[] = {-1, -1, 0, 1, 2, 3, 4, 5, 6, -1};
            const int idx = juce::jlimit(0, 9, divisionIndex);
            return kMap[idx];
        }
    } // namespace

    DualLfoLabPanel::LfoColumn::LfoColumn(MurmurProcessor& processor, std::size_t lfoIndex, const char* title,
                                          juce::Colour accent)
        : panel(title, accent), wireframe(processor.apvts, lfoIndex), lfoIndex(lfoIndex)
    {
        wireframe.attachVisualizerBus(processor.getVisualizerBus());

        syncHeaderLabel.setFont(fonts::label(8.0f));
        syncHeaderLabel.setColour(juce::Label::textColourId, palette::kTextDim);
        syncHeaderLabel.setJustificationType(juce::Justification::centredLeft);
        syncHeaderLabel.setText("SYNC DIVISION (TEMPO SYNC MODE)", juce::dontSendNotification);

        static constexpr const char* kSyncLabels[] = {"1 bar", "1/2", "1/4", "1/8", "1/16", "1/4T", "1/8T"};
        for (std::size_t i = 0; i < syncButtons.size(); ++i)
        {
            styleSyncChipButton(syncButtons[i], kSyncLabels[i]);
            syncButtons[i].onClick = [this, i, &processor] {
                if (auto* param = processor.apvts.getParameter(lfoParamId(this->lfoIndex, "SyncDivisionIndex")))
                    param->setValueNotifyingHost(
                        param->convertTo0to1(static_cast<float>(syncChipToDivisionIndex(static_cast<int>(i)))));
            };
        }

        routingLegend.setFont(fonts::label(7.0f));
        routingLegend.setColour(juce::Label::textColourId, palette::kTextDim);
        routingLegend.setJustificationType(juce::Justification::centredRight);
        routingLegend.setText("■ LAYER   ◎ VOICE", juce::dontSendNotification);

        rebind(processor, lfoIndex, title, accent);
        panel.addAndMakeVisible(wireframe);
        panel.addAndMakeVisible(syncHeaderLabel);
        for (auto& btn : syncButtons)
            panel.addAndMakeVisible(btn);
        panel.addAndMakeVisible(routingLabel);
        panel.addAndMakeVisible(routingLegend);
    }

    void DualLfoLabPanel::LfoColumn::rebind(MurmurProcessor& processor, std::size_t newIndex, const char* title,
                                            juce::Colour accent)
    {
        lfoIndex = newIndex;
        panel.setTitle(title);
        wireframe.setLfoIndex(newIndex);

        if (waveform != nullptr)
        {
            panel.removeChildComponent(waveform.get());
            panel.removeChildComponent(mode.get());
            panel.removeChildComponent(motion.get());
        }

        waveform = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(lfoIndex, "Waveform"), "WAVE", lfoWaveformToText);
        mode = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(lfoIndex, "Mode"), "MODE", lfoModeToText);
        motion = std::make_unique<ConcentricGlowKnob>(processor.apvts, lfoParamId(lfoIndex, "PhaseOffset"),
                                                      lfoParamId(lfoIndex, "RateHz"), "PHASE", "RATE");
        waveform->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        mode->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        motion->applyFigmaContext(figma::KnobContext::LfoMotionDual);
        motion->setInnerAccentColour(accent);

        routingLabel.setText(lfoIndex == 0   ? "ROUTED DESTINATIONS: Filter Cutoff, WT Position"
                             : lfoIndex == 1 ? "ROUTED DESTINATIONS: Pitch Fine, Amp Env Decay"
                                             : "ROUTED DESTINATIONS: Mod Matrix",
                             juce::dontSendNotification);

        panel.addAndMakeVisible(*waveform);
        panel.addAndMakeVisible(*mode);
        panel.addAndMakeVisible(*motion);
    }

    void DualLfoLabPanel::LfoColumn::layoutColumn()
    {
        auto content = panel.getContentBounds().reduced(6);
        routingLegend.setBounds(content.removeFromBottom(14));
        routingLabel.setBounds(content.removeFromBottom(14));
        content.removeFromBottom(6);

        auto syncRow = content.removeFromBottom(layout::kDesignDualLfoSyncRowHeight);
        syncHeaderLabel.setBounds(syncRow.removeFromTop(12));
        syncRow.removeFromTop(2);
        const int chipW = (syncRow.getWidth() - 6 * 1) / 7;
        for (auto& btn : syncButtons)
        {
            btn.setBounds(syncRow.removeFromLeft(chipW));
            syncRow.removeFromLeft(1);
        }

        content.removeFromBottom(8);
        wireframe.setBounds(content.removeFromTop(layout::kDesignDualLfoScopeHeight).reduced(2));
        content.removeFromTop(8);
        const int knobW = content.getWidth() / 4;
        const int knobH = layout::kDesignDualLfoKnobsRowHeight;
        waveform->setBounds(content.removeFromLeft(knobW).removeFromTop(knobH).reduced(4));
        mode->setBounds(content.removeFromLeft(knobW).removeFromTop(knobH).reduced(4));
        motion->setBounds(content.removeFromLeft(knobW * 2).removeFromTop(knobH).reduced(4));
    }

    DualLfoLabPanel::DualLfoLabPanel(MurmurProcessor& processor)
        : processor_(processor),
          lfo1_(processor, 0, "LFO 1", palette::kAccent),
          lfo2_(processor, 1, "LFO 2", palette::kMurmurViolet),
          lfo3_(processor, 4, "LFO 5", palette::kAccent),
          lfo4_(processor, 5, "LFO 6", palette::kMurmurViolet)
    {
        addAndMakeVisible(backButton_);
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };

        badgeLabel_.setText("UX-09", juce::dontSendNotification);
        badgeLabel_.setFont(fonts::label(9.0f));
        badgeLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        badgeLabel_.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(badgeLabel_);

        titleLabel_.setText("DUAL LFO LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(12.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("MURMUR 8-ENGINE LFO MODULATOR", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(7.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        addAndMakeVisible(subtitleLabel_);

        footerHint_.setText("Scope (Voice vs Layer) is set per route in the Mod Matrix, not here.",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::label(10.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        openModMatrixButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openModMatrixButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        openModMatrixButton_.onClick = [this] {
            if (onOpenModMatrix)
                onOpenModMatrix();
        };
        addAndMakeVisible(openModMatrixButton_);

        static constexpr const char* kPairTabLabels[] = {"LFO 1·2", "LFO 3·4", "LFO 5·8"};
        for (std::size_t i = 0; i < pairTabButtons_.size(); ++i)
        {
            stylePairTabButton(pairTabButtons_[i], kPairTabLabels[i]);
            pairTabButtons_[i].onClick = [this, i] { bindPairTab(static_cast<int>(i)); };
            addAndMakeVisible(pairTabButtons_[i]);
        }

        addAndMakeVisible(lfo1_.panel);
        addAndMakeVisible(lfo2_.panel);
        addAndMakeVisible(lfo3_.panel);
        addAndMakeVisible(lfo4_.panel);

        static constexpr const char* kFooterChipLabels[] = {"LF01", "LF02", "LF03", "LF04",
                                                            "LF05", "LF06", "LF07", "LF08"};
        for (std::size_t i = 0; i < footerChipButtons_.size(); ++i)
        {
            footerChipButtons_[i].setButtonText(kFooterChipLabels[i]);
            footerChipButtons_[i].setClickingTogglesState(false);
            footerChipButtons_[i].onClick = [this, i] { selectFooterLfo(i); };
            addAndMakeVisible(footerChipButtons_[i]);
        }

        bindPairTab(0);
    }

    void DualLfoLabPanel::selectFooterLfo(std::size_t lfoIndex)
    {
        const auto clamped = juce::jlimit<std::size_t>(0, 7, lfoIndex);
        bindPairTab(static_cast<int>(clamped / 2));
        activeFooterLfo_ = clamped;
        highlightFooterChips();
    }

    void DualLfoLabPanel::layoutColumns()
    {
        if (activePairTab_ == 2)
        {
            lfo1_.rebind(processor_, 4, "LFO 5", palette::kAccent);
            lfo2_.rebind(processor_, 5, "LFO 6", palette::kMurmurViolet);
            lfo3_.rebind(processor_, 6, "LFO 7", palette::kAccent.withAlpha(0.9f));
            lfo4_.rebind(processor_, 7, "LFO 8", palette::kMurmurViolet.withAlpha(0.9f));
            lfo1_.panel.setVisible(true);
            lfo2_.panel.setVisible(true);
            lfo3_.panel.setVisible(true);
            lfo4_.panel.setVisible(true);
            lfo1_.layoutColumn();
            lfo2_.layoutColumn();
            lfo3_.layoutColumn();
            lfo4_.layoutColumn();
            highlightSyncButtons(lfo1_);
            highlightSyncButtons(lfo2_);
            highlightSyncButtons(lfo3_);
            highlightSyncButtons(lfo4_);
            return;
        }

        lfo3_.panel.setVisible(false);
        lfo4_.panel.setVisible(false);
        lfo1_.layoutColumn();
        lfo2_.layoutColumn();
        highlightSyncButtons(lfo1_);
        highlightSyncButtons(lfo2_);
    }

    void DualLfoLabPanel::bindPairTab(int tabIndex)
    {
        activePairTab_ = juce::jlimit(0, 2, tabIndex);
        static constexpr struct
        {
            std::size_t left;
            std::size_t right;
            const char* leftTitle;
            const char* rightTitle;
        } kPairs[] = {{0, 1, "LFO 1", "LFO 2"}, {2, 3, "LFO 3", "LFO 4"}, {4, 5, "LFO 5", "LFO 6"}};

        if (activePairTab_ != 2)
        {
            const auto& pair = kPairs[static_cast<std::size_t>(activePairTab_)];
            lfo1_.rebind(processor_, pair.left, pair.leftTitle, palette::kAccent);
            lfo2_.rebind(processor_, pair.right, pair.rightTitle, palette::kMurmurViolet);
        }

        activeFooterLfo_ = static_cast<std::size_t>(activePairTab_) * 2;
        highlightPairTabs();
        highlightFooterChips();
        layoutColumns();
        resized();
    }

    namespace
    {
        const juce::Colour kFooterChipColours[] = {palette::kAccent,       palette::kMurmurViolet,
                                                   palette::kAccent,       palette::kMurmurViolet,
                                                   palette::kAccent,       palette::kMurmurViolet,
                                                   palette::kAccent,       palette::kMurmurViolet};
    } // namespace

    void DualLfoLabPanel::highlightFooterChips()
    {
        for (std::size_t i = 0; i < footerChipButtons_.size(); ++i)
        {
            const bool active = i == activeFooterLfo_;
            auto& btn = footerChipButtons_[i];
            btn.setColour(juce::TextButton::buttonColourId,
                          active ? kFooterChipColours[i].withAlpha(0.12f) : palette::kBackgroundBottom);
            btn.setColour(juce::TextButton::textColourOffId, active ? palette::kTextPrimary : palette::kTextDim);
        }
    }

    void DualLfoLabPanel::highlightPairTabs()
    {
        for (std::size_t i = 0; i < pairTabButtons_.size(); ++i)
            highlightPairTabButton(pairTabButtons_[i], static_cast<int>(i) == activePairTab_);
    }

    void DualLfoLabPanel::highlightSyncButtons(LfoColumn& column)
    {
        if (auto* raw = processor_.apvts.getRawParameterValue(lfoParamId(column.lfoIndex, "SyncDivisionIndex")))
        {
            const int activeChip = divisionIndexToSyncChip(static_cast<int>(raw->load() + 0.5f));
            const juce::Colour accent = column.lfoIndex % 2 == 0 ? palette::kAccent : palette::kMurmurViolet;
            for (std::size_t i = 0; i < column.syncButtons.size(); ++i)
                highlightSyncChipButton(column.syncButtons[i], static_cast<int>(i) == activeChip, accent);
        }
    }

    DualLfoLabPanel::~DualLfoLabPanel() = default;

    void DualLfoLabPanel::showOverlay() { setVisible(true); }

    void DualLfoLabPanel::dismiss() { setVisible(false); }

    void DualLfoLabPanel::setEmbeddedInDesignMode(bool embedded)
    {
        if (embeddedInDesignMode_ == embedded)
            return;

        embeddedInDesignMode_ = embedded;
        backButton_.setButtonText(embedded ? "← DESIGN" : "← PLAY BOARD");
        badgeLabel_.setVisible(!embedded);
        titleLabel_.setVisible(!embedded);
        subtitleLabel_.setVisible(!embedded);
        for (auto& tab : pairTabButtons_)
            tab.setVisible(!embedded);
        resized();
    }

    void DualLfoLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop);

        if (!embeddedInDesignMode_)
        {
            auto badge = badgeLabel_.getBounds().toFloat().expanded(4.0f, 2.0f);
            g.setColour(palette::kAccent.withAlpha(0.13f));
            g.fillRoundedRectangle(badge, 4.0f);
            g.setColour(palette::kAccent);
            g.drawRoundedRectangle(badge, 4.0f, 1.0f);
        }

        if (!footerBounds_.isEmpty())
        {
            g.setColour(palette::kPanelRaised.withAlpha(0.55f));
            g.fillRoundedRectangle(footerBounds_.toFloat(), 8.0f);
            g.setColour(palette::kBorder.withAlpha(0.45f));
            g.drawRoundedRectangle(footerBounds_.toFloat().reduced(0.5f), 8.0f, 1.0f);

            auto chips = footerBounds_.reduced(16, 0).removeFromLeft(320);
            g.setFont(fonts::label(7.0f));
            g.setColour(palette::kTextDim);
            g.drawText("MOD SOURCE MATRIX QUICK-LINKS:", chips.removeFromTop(12), juce::Justification::centredLeft);
        }
    }

    void DualLfoLabPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignDualLfoPageOuterMargin);

        const int headerHeight =
            embeddedInDesignMode_ ? layout::kDesignLabPanelHeaderHeight : layout::kDesignDualLfoLabHeaderHeight;
        auto header = bounds.removeFromTop(headerHeight);
        backButton_.setBounds(header.removeFromLeft(120));
        if (!embeddedInDesignMode_)
        {
            header.removeFromLeft(8);
            badgeLabel_.setBounds(header.removeFromLeft(36).withSizeKeepingCentre(36, 18));
            header.removeFromLeft(8);
            titleLabel_.setBounds(header.removeFromLeft(100));
            header.removeFromLeft(8);
            subtitleLabel_.setBounds(header.removeFromLeft(180));
            header.removeFromLeft(12);
            auto tabs = header.removeFromRight(220);
            const int tabW = (tabs.getWidth() - 8) / 3;
            for (auto& tab : pairTabButtons_)
            {
                tab.setBounds(tabs.removeFromLeft(tabW).reduced(1));
                tabs.removeFromLeft(4);
            }
        }

        bounds.removeFromTop(layout::kDesignDualLfoPageSectionGap);
        footerBounds_ = bounds.removeFromBottom(layout::kDesignDualLfoBottomBarHeight);
        bounds.removeFromBottom(layout::kDesignDualLfoPageSectionGap);

        auto footer = footerBounds_.reduced(16, 8);
        openModMatrixButton_.setBounds(footer.removeFromRight(160).reduced(2));
        footer.removeFromRight(12);
        footerHint_.setBounds(footer.withSizeKeepingCentre(footer.getWidth() - 300, 20));

        auto chipsRow = footerBounds_.reduced(16, 0).removeFromLeft(320).removeFromBottom(18);
        chipsRow.removeFromTop(14);
        const int chipW = (chipsRow.getWidth() - 7 * 4) / 8;
        for (auto& chip : footerChipButtons_)
        {
            chip.setBounds(chipsRow.removeFromLeft(chipW).withHeight(14));
            chipsRow.removeFromLeft(4);
        }

        const int columnCount = activePairTab_ == 2 ? layout::kDesignDualLfoQuadColumnCount : 2;

        if (activePairTab_ == 2)
        {
            const int totalGap = (columnCount - 1) * layout::kDesignDualLfoColumnGap;
            const int colW = (bounds.getWidth() - totalGap) / columnCount;
            lfo1_.panel.setBounds(bounds.removeFromLeft(colW));
            bounds.removeFromLeft(layout::kDesignDualLfoColumnGap);
            lfo2_.panel.setBounds(bounds.removeFromLeft(colW));
            bounds.removeFromLeft(layout::kDesignDualLfoColumnGap);
            lfo3_.panel.setBounds(bounds.removeFromLeft(colW));
            bounds.removeFromLeft(layout::kDesignDualLfoColumnGap);
            lfo4_.panel.setBounds(bounds);
        }
        else
        {
            const int colW = layout::kDesignDualLfoColumnWidth;
            const int totalW = colW * 2 + layout::kDesignDualLfoColumnGap;
            bounds.removeFromLeft(juce::jmax(0, (bounds.getWidth() - totalW) / 2));
            lfo1_.panel.setBounds(bounds.removeFromLeft(colW));
            bounds.removeFromLeft(layout::kDesignDualLfoColumnGap);
            lfo2_.panel.setBounds(bounds.removeFromLeft(colW));
        }

        layoutColumns();
        highlightFooterChips();
    }

    bool DualLfoLabPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

} // namespace pw8::plugin::ui
