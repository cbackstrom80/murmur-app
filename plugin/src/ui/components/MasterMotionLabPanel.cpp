#include "MasterMotionLabPanel.h"

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

        void styleSectionLabel(juce::Label& label, const juce::String& text)
        {
            label.setText(text, juce::dontSendNotification);
            label.setFont(fonts::label(10.0f));
            label.setColour(juce::Label::textColourId, palette::kTextPrimary);
            label.setJustificationType(juce::Justification::centredLeft);
        }
    } // namespace

    MasterMotionLabPanel::LfoColumn::LfoColumn(MurmurProcessor& processor, std::size_t lfoIndex,
                                                 const char* title, juce::Colour accentColour)
        : panel(title, accentColour), wireframe(processor.apvts, lfoIndex), accent(accentColour), lfoIndex(lfoIndex)
    {
        wireframe.attachVisualizerBus(processor.getVisualizerBus());

        syncHeaderLabel.setFont(fonts::label(7.0f));
        syncHeaderLabel.setColour(juce::Label::textColourId, palette::kTextDim);
        syncHeaderLabel.setJustificationType(juce::Justification::centredLeft);
        syncHeaderLabel.setText("SYNC DIVISION (TEMPO SYNC)", juce::dontSendNotification);

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

        waveform = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(lfoIndex, "Waveform"), "WAVE", lfoWaveformToText);
        mode = std::make_unique<GlowKnob>(processor.apvts, lfoParamId(lfoIndex, "Mode"), "MODE", lfoModeToText);
        motion = std::make_unique<ConcentricGlowKnob>(processor.apvts, lfoParamId(lfoIndex, "PhaseOffset"),
                                                      lfoParamId(lfoIndex, "RateHz"), "PHASE", "RATE");
        waveform->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        mode->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        motion->applyFigmaContext(figma::KnobContext::LfoMotionDual);
        motion->setInnerAccentColour(accentColour);

        routingLabel.setFont(fonts::label(7.0f));
        routingLabel.setColour(juce::Label::textColourId, palette::kTextDim);
        routingLabel.setJustificationType(juce::Justification::centredLeft);
        routingLabel.setText(lfoIndex == 0   ? "ROUTED: Filter Cutoff, WT Position"
                             : lfoIndex == 1 ? "ROUTED: Pitch Fine, Amp Decay"
                             : lfoIndex == 2 ? "ROUTED: Mod Matrix"
                                             : "ROUTED: Mod Matrix",
                             juce::dontSendNotification);

        panel.addAndMakeVisible(wireframe);
        panel.addAndMakeVisible(syncHeaderLabel);
        for (auto& btn : syncButtons)
            panel.addAndMakeVisible(btn);
        panel.addAndMakeVisible(*waveform);
        panel.addAndMakeVisible(*mode);
        panel.addAndMakeVisible(*motion);
        panel.addAndMakeVisible(routingLabel);
        panel.addAndMakeVisible(routingLegend);
    }

    void MasterMotionLabPanel::LfoColumn::layoutColumn()
    {
        auto content = panel.getContentBounds().reduced(4);
        routingLegend.setBounds(content.removeFromBottom(10));
        routingLabel.setBounds(content.removeFromBottom(12));
        content.removeFromBottom(2);

        auto syncRow = content.removeFromBottom(layout::kMasterMotionLabLfoSyncRowHeight);
        syncHeaderLabel.setBounds(syncRow.removeFromTop(10));
        syncRow.removeFromTop(1);
        const int chipW = juce::jmax(1, (syncRow.getWidth() - 6) / 7);
        for (auto& btn : syncButtons)
        {
            btn.setBounds(syncRow.removeFromLeft(chipW));
            syncRow.removeFromLeft(1);
        }

        content.removeFromBottom(4);
        auto knobRow = content.removeFromBottom(layout::kMasterMotionLabLfoKnobsRowHeight);
        const int knobW = juce::jmax(1, knobRow.getWidth() / 4);
        waveform->setBounds(knobRow.removeFromLeft(knobW).reduced(2));
        mode->setBounds(knobRow.removeFromLeft(knobW).reduced(2));
        motion->setBounds(knobRow.reduced(2));

        content.removeFromBottom(4);
        wireframe.setBounds(content.withHeight(juce::jmax(layout::kMasterMotionLabLfoScopeMinHeight, content.getHeight()))
                               .reduced(1));
    }

    MasterMotionLabPanel::MasterMotionLabPanel(MurmurProcessor& processor)
        : processor_(processor),
          morphTimeline_(processor),
          envelopePanel_(processor),
          lfo1_(processor, 0, "LFO 1", palette::kAccent),
          lfo2_(processor, 1, "LFO 2", palette::kMurmurViolet),
          lfo3_(processor, 2, "LFO 3", palette::kAccentWarm),
          lfo4_(processor, 3, "LFO 4", juce::Colour(0xff9f80ff))
    {
        backButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        backButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        closeButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(closeButton_);

        titleLabel_.setText("MASTER MOTION LAB", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(12.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("MASTER ENVELOPE · MORPH KOIN · LFO 1–4 · LAYER SCOPE", juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(7.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kAccent);
        addAndMakeVisible(subtitleLabel_);

        styleSectionLabel(envelopeSectionLabel_, "MASTER ENVELOPE");
        styleSectionLabel(morphSectionLabel_, "MORPH KOIN");
        styleSectionLabel(lfoSectionLabel_, "MASTER LFOs");
        addAndMakeVisible(envelopeSectionLabel_);
        addAndMakeVisible(morphSectionLabel_);
        addAndMakeVisible(lfoSectionLabel_);

        morphTimeline_.setShowMorphKnob(true);
        morphTimeline_.setCompactHubMode(true);
        envelopePanel_.setCompactSectionMode(true);

        openModMatrixButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openModMatrixButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        openModMatrixButton_.onClick = [this] {
            if (onOpenModMatrix)
                onOpenModMatrix();
        };
        addAndMakeVisible(openModMatrixButton_);

        openMorphEditorButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openMorphEditorButton_.setColour(juce::TextButton::textColourOffId, palette::kFigmaTeal);
        openMorphEditorButton_.onClick = [this] {
            if (onOpenMorphEditor)
                onOpenMorphEditor();
        };
        addAndMakeVisible(openMorphEditorButton_);

        openSegmentsButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openSegmentsButton_.setColour(juce::TextButton::textColourOffId, palette::kTextDim);
        openSegmentsButton_.onClick = [this] {
            if (onOpenEnvelopeSegments)
                onOpenEnvelopeSegments();
        };
        addAndMakeVisible(openSegmentsButton_);

        footerHint_.setText("Scope: LAYER (shared across voices) · LFO 5–8 in DESIGN → Dual LFO",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::label(9.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        addAndMakeVisible(morphTimeline_);
        addAndMakeVisible(envelopePanel_);
        addAndMakeVisible(lfo1_.panel);
        addAndMakeVisible(lfo2_.panel);
        addAndMakeVisible(lfo3_.panel);
        addAndMakeVisible(lfo4_.panel);

        startTimerHz(8);
    }

    MasterMotionLabPanel::~MasterMotionLabPanel() = default;

    void MasterMotionLabPanel::showOverlay() { setVisible(true); }

    void MasterMotionLabPanel::dismiss() { setVisible(false); }

    void MasterMotionLabPanel::timerCallback()
    {
        highlightSyncButtons(lfo1_);
        highlightSyncButtons(lfo2_);
        highlightSyncButtons(lfo3_);
        highlightSyncButtons(lfo4_);
    }

    void MasterMotionLabPanel::highlightSyncButtons(LfoColumn& column)
    {
        if (auto* raw = processor_.apvts.getRawParameterValue(lfoParamId(column.lfoIndex, "SyncDivisionIndex")))
        {
            const int activeChip = divisionIndexToSyncChip(static_cast<int>(raw->load() + 0.5f));
            for (std::size_t i = 0; i < column.syncButtons.size(); ++i)
                highlightSyncChipButton(column.syncButtons[i], static_cast<int>(i) == activeChip, column.accent);
        }
    }

    bool MasterMotionLabPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

    void MasterMotionLabPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundBottom);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
    }

    void MasterMotionLabPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(12);

        auto header = bounds.removeFromTop(28);
        backButton_.setBounds(header.removeFromLeft(110));
        header.removeFromLeft(12);
        titleLabel_.setBounds(header.removeFromLeft(200));
        subtitleLabel_.setBounds(header.withTrimmedRight(28));
        closeButton_.setBounds(header.removeFromRight(24).reduced(0, 2));

        bounds.removeFromTop(layout::kMasterMotionLabSectionGap);
        envelopeSectionLabel_.setBounds(bounds.removeFromTop(14));
        bounds.removeFromTop(2);
        envelopePanel_.setBounds(bounds.removeFromTop(layout::kMasterMotionLabEnvelopeHeroHeight));
        bounds.removeFromTop(layout::kMasterMotionLabSectionGap);

        morphSectionLabel_.setBounds(bounds.removeFromTop(14));
        bounds.removeFromTop(2);
        morphTimeline_.setBounds(bounds.removeFromTop(layout::kMasterMotionLabMorphHeight));
        bounds.removeFromTop(layout::kMasterMotionLabSectionGap);

        lfoSectionLabel_.setBounds(bounds.removeFromTop(14));
        bounds.removeFromTop(2);

        auto footer = bounds.removeFromBottom(layout::kMasterMotionLabFooterHeight);
        bounds.removeFromBottom(layout::kMasterMotionLabSectionGap);

        auto lfoRow = bounds;
        const int colW = lfoRow.getWidth() / 4;
        lfo1_.panel.setBounds(lfoRow.removeFromLeft(colW).withTrimmedRight(3));
        lfo2_.panel.setBounds(lfoRow.removeFromLeft(colW).withTrimmedLeft(1).withTrimmedRight(2));
        lfo3_.panel.setBounds(lfoRow.removeFromLeft(colW).withTrimmedLeft(1).withTrimmedRight(2));
        lfo4_.panel.setBounds(lfoRow.withTrimmedLeft(1));

        lfo1_.layoutColumn();
        lfo2_.layoutColumn();
        lfo3_.layoutColumn();
        lfo4_.layoutColumn();

        openSegmentsButton_.setBounds(footer.removeFromLeft(96).reduced(0, 2));
        footer.removeFromLeft(6);
        openMorphEditorButton_.setBounds(footer.removeFromLeft(120).reduced(0, 2));
        footer.removeFromLeft(6);
        openModMatrixButton_.setBounds(footer.removeFromLeft(160).reduced(0, 2));
        footer.removeFromLeft(12);
        footerHint_.setBounds(footer);
    }

} // namespace pw8::plugin::ui
