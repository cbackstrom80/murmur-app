#include "CompactModeEditor.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"

namespace pw8::plugin::ui
{
    CompactModeEditor::CompactModeEditor(MurmurProcessor& processor)
        : processor_(processor),
          scopeView_(processor),
          focusPanel_(processor)
    {
        scopeView_.setCompactLayout(true);
        addAndMakeVisible(scopeView_);

        focusPanel_.setCompactLayout(true);
        addAndMakeVisible(focusPanel_);

        masterKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "MASTER", [](float value) {
            return juce::String(value, 1) + " dB";
        }, palette::kAccentWarm);
        masterKnob_->applyFigmaContext(figma::KnobContext::CompactMaster);
        addAndMakeVisible(*masterKnob_);

        startTimerHz(15);
        timerCallback();
    }

    CompactModeEditor::~CompactModeEditor()
    {
        stopTimer();
    }

    void CompactModeEditor::paintCardBackground(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        g.setColour(palette::kPanelRaised.withAlpha(0.92f));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
    }

    void CompactModeEditor::paintHorizontalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                 const scope::VuBallistics& vu) const
    {
        g.setColour(juce::Colour(0xff0d0f14));
        g.fillRoundedRectangle(bounds, 3.0f);

        const float fillNorm = juce::jlimit(0.0f, 1.0f, vu.rmsNorm());
        const float peakNorm = juce::jlimit(fillNorm, 1.0f, vu.peakHoldNorm());
        const float greenWidth = bounds.getWidth() * fillNorm;
        const float amberWidth = bounds.getWidth() * (peakNorm - fillNorm);

        if (greenWidth > 0.5f)
        {
            g.setColour(palette::kAccent.withAlpha(0.92f));
            g.fillRoundedRectangle(bounds.withWidth(greenWidth), 3.0f);
        }

        if (amberWidth > 0.5f)
        {
            g.setColour(palette::kAccentWarm.withAlpha(0.95f));
            g.fillRoundedRectangle(bounds.withX(bounds.getX() + greenWidth).withWidth(amberWidth), 3.0f);
        }
    }

    void CompactModeEditor::paint(juce::Graphics& g)
    {
        if (!scopePanelBounds_.isEmpty())
            paintCardBackground(g, scopePanelBounds_.toFloat());
        if (!megaKnobDeckBounds_.isEmpty())
            paintCardBackground(g, megaKnobDeckBounds_.toFloat());
    }

    void CompactModeEditor::paintOverChildren(juce::Graphics& g)
    {
        if (!megaKnobDeckBounds_.isEmpty())
        {
            auto header = megaKnobDeckBounds_.reduced(layout::kCompactMacroPanelPadding)
                             .removeFromTop(layout::kCompactMacroHeaderHeight);
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextSecondary);
            g.drawText("PERFORMANCE KOINS", header, juce::Justification::centredLeft, true);
        }

        if (!meterStatsBounds_.isEmpty())
        {
            auto stats = meterStatsBounds_;
            auto meterGroup = stats.removeFromTop(16);
            const int meterGap = layout::kCompactOutputMeterGap;
            auto leftMeter = meterGroup.removeFromTop(layout::kCompactOutputMeterHeight);
            meterGroup.removeFromTop(meterGap);
            auto rightMeter = meterGroup.removeFromTop(layout::kCompactOutputMeterHeight);
            paintHorizontalMeter(g, leftMeter.toFloat(), leftVu_);
            paintHorizontalMeter(g, rightMeter.toFloat(), rightVu_);

            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextSecondary);
            auto statsText = stats;
            g.drawText("CPU: " + formatCpuPercent(metrics_.cpuPercent), statsText.removeFromTop(10),
                       juce::Justification::centredLeft, true);
            g.drawText("VOICES: " + formatVoiceCount(metrics_.activeVoices, metrics_.maxVoices), statsText,
                       juce::Justification::centredLeft, true);
        }

        if (!footerBounds_.isEmpty())
        {
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kTextSecondary);
            g.drawText(juce::String("MURMUR OBSIDIAN ") + juce::String(juce::CharPointer_UTF8("\xc2\xa9")) + " 2026",
                       footerBounds_, juce::Justification::centred, true);
        }
    }

    void CompactModeEditor::updateMissionCard()
    {
        const auto& meta = processor_.getCurrentPatch().metadata;
        const auto name = juce::String(meta.name);
        juce::ignoreUnused(name);

        juce::String category;
        if (const auto presetPath = processor_.getCurrentPresetPath(); presetPath.isNotEmpty())
            category = juce::File(presetPath).getParentDirectory().getFileName();
        if (category.isEmpty())
            category = "Factory";

        const auto hint = performanceHintForPatch(processor_.getCurrentPatch(), &processor_.apvts);
        juce::ignoreUnused(hint);

        if (category != lastCategory_ || hint != lastHint_)
        {
            lastCategory_ = category;
            lastHint_ = hint;
            repaint();
        }
    }

    void CompactModeEditor::timerCallback()
    {
        updateMissionCard();
        metrics_ = readPerformanceMetrics(processor_);

        const float masterPeak = processor_.getMasterOutPeakLinear();
        leftVu_.processFrame(masterPeak * 0.86f, masterPeak);
        rightVu_.processFrame(masterPeak * 0.78f, masterPeak * 0.94f);

        repaint(meterStatsBounds_);
        repaint(footerBounds_);
    }

    void CompactModeEditor::resized()
    {
        auto bounds = getLocalBounds();

        footerBounds_ = bounds.removeFromBottom(layout::kCompactFooterSystemHeight);
        bounds.removeFromBottom(layout::kCompactBlockGap);

        megaKnobDeckBounds_ = bounds.removeFromBottom(layout::kCompactMegaKnobDeckHeight);
        bounds.removeFromBottom(layout::kCompactBlockGap);

        scopePanelBounds_ = bounds;
        scopeView_.setBounds(scopePanelBounds_);

        const int deckPad = layout::kCompactMacroPanelPadding;
        const int headerSkip = layout::kCompactMacroHeaderHeight + layout::kCompactMacroHeaderGap;
        auto deck = megaKnobDeckBounds_.reduced(deckPad).withTrimmedTop(headerSkip);

        const int colGap = layout::kCompactMacroKnobGap;
        const int colW = (deck.getWidth() - 2 * colGap) / 3;

        masterKnobBounds_ = deck.removeFromLeft(colW);
        deck.removeFromLeft(colGap);
        koinKnobBounds_ = deck;

        const int dialPad = 18;
        masterKnob_->setBounds(masterKnobBounds_.withSizeKeepingCentre(layout::kCompactMegaKnobSize + dialPad,
                                                                       layout::kCompactMegaKnobSize + dialPad));
        focusPanel_.setBounds(koinKnobBounds_);

        meterStatsBounds_ = scopePanelBounds_.removeFromBottom(52).reduced(12, 6);
    }

} // namespace pw8::plugin::ui
