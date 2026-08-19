#include "MasterOutputDeck.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        static constexpr const char* kModeLabels[] = {"ENV", "VACT", "FOLL", "COMP"};
    }

    MasterOutputDeck::MasterOutputDeck(MurmurProcessor& processor) : processor_(processor)
    {
        masterKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "MASTER", nullptr, palette::kAccent);
        masterKnob_->setHeaderCompactMode(true);
        masterKnob_->applyFigmaContext(figma::KnobContext::PanelGridMedium);
        addAndMakeVisible(*masterKnob_);

        enableButton_.setClickingTogglesState(true);
        enableButton_.setRadioGroupId(0);
        addAndMakeVisible(enableButton_);
        enabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            processor_.apvts, kMasterDynamicsEnabledId, enableButton_);

        for (std::size_t i = 0; i < modePills_.size(); ++i)
        {
            modePills_[i].setButtonText(kModeLabels[i]);
            modePills_[i].onClick = [this, i] { setDynamicsMode(static_cast<int>(i)); };
            addAndMakeVisible(modePills_[i]);
        }

        refreshModePills();
        startTimerHz(30);
    }

    MasterOutputDeck::~MasterOutputDeck() { stopTimer(); }

    void MasterOutputDeck::setDynamicsMode(int modeIndex)
    {
        if (auto* param = processor_.apvts.getParameter(kMasterDynamicsModeId))
            param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(juce::jlimit(0, 3, modeIndex))));
        refreshModePills();
    }

    void MasterOutputDeck::styleModePill(juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId, active ? palette::kAccent.withAlpha(0.22f) : juce::Colour(0xff12141a));
        btn.setColour(juce::TextButton::buttonOnColourId, palette::kAccent.withAlpha(0.35f));
        btn.setColour(juce::TextButton::textColourOffId, active ? palette::kAccent : palette::kTextDim);
        btn.setColour(juce::TextButton::textColourOnId, palette::kAccentWarm);
    }

    void MasterOutputDeck::refreshModePills()
    {
        int mode = 0;
        if (auto* raw = processor_.apvts.getRawParameterValue(kMasterDynamicsModeId))
            mode = static_cast<int>(raw->load());
        for (std::size_t i = 0; i < modePills_.size(); ++i)
            styleModePill(modePills_[i], static_cast<int>(i) == mode);
    }

    void MasterOutputDeck::timerCallback()
    {
        refreshModePills();

        const int pulled =
            processor_.readScopeSamples(scopeScratch_.data(), static_cast<int>(scopeScratch_.size()));
        if (pulled > 0)
        {
            const auto [rms, peak] = scope::measureMonoBlock(scopeScratch_.data(), pulled);
            leftVu_.processFrame(rms, peak);
            rightVu_.processFrame(rms * 0.92f, peak * 0.95f);
        }
        else
        {
            const float masterPeak = processor_.getMasterOutPeakLinear();
            leftVu_.processFrame(masterPeak * 0.707f, masterPeak);
            rightVu_.processFrame(masterPeak * 0.65f, masterPeak * 0.95f);
        }

        grMeterDb_ = processor_.getMasterDynamicsGainReductionDb();
        sidechainEnvelope_ = processor_.getMasterDynamicsSidechainEnvelope();
        if (processor_.getSidechainActive())
            sidechainEnvelope_ = std::max(sidechainEnvelope_, processor_.getSidechainLevel());

        sidechainHistory_[sidechainHistoryWrite_ % sidechainHistory_.size()] = sidechainEnvelope_;
        ++sidechainHistoryWrite_;

        repaint(leftMeterBounds_);
        repaint(rightMeterBounds_);
        repaint(grMeterBounds_);
        repaint(sidechainVizBounds_);
    }

    void MasterOutputDeck::paintGainReductionMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                     float grDb) const
    {
        if (bounds.isEmpty())
            return;

        g.setFont(fonts::label(layout::kIpadPlayCaptionSize));
        g.setColour(palette::kTextDim);
        g.drawText("GR", bounds.removeFromTop(10.0f), juce::Justification::centred, true);

        auto track = bounds.reduced(4.0f, 2.0f);
        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.75f));
        g.drawRoundedRectangle(track.reduced(0.5f), 4.0f, 1.0f);

        const float norm = juce::jlimit(0.0f, 1.0f, std::abs(grDb) / 24.0f);
        auto fill = track.reduced(2.0f);
        const float fillHeight = fill.getHeight() * norm;
        auto fillRect = fill.withTop(fill.getBottom() - fillHeight);
        g.setColour(palette::kAccentWarm.withAlpha(0.85f));
        g.fillRoundedRectangle(fillRect, 2.0f);
    }

    void MasterOutputDeck::paintSidechainViz(juce::Graphics& g, juce::Rectangle<float> bounds) const
    {
        if (bounds.isEmpty())
            return;

        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.6f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);

        auto plot = bounds.reduced(4.0f, 3.0f);
        juce::Path path;
        const float w = plot.getWidth();
        const float h = plot.getHeight();
        for (std::size_t i = 0; i < sidechainHistory_.size(); ++i)
        {
            const std::size_t idx = (sidechainHistoryWrite_ + i) % sidechainHistory_.size();
            const float x = plot.getX() + (static_cast<float>(i) / static_cast<float>(sidechainHistory_.size() - 1)) * w;
            const float y = plot.getBottom() - sidechainHistory_[idx] * h;
            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }
        g.setColour(processor_.getSidechainActive() ? palette::kFigmaTeal.withAlpha(0.85f)
                                                    : palette::kAccent.withAlpha(0.55f));
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }

    void MasterOutputDeck::paintVerticalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                              const scope::VuBallistics& vu, const char* label) const
    {
        if (bounds.isEmpty())
            return;

        g.setFont(fonts::label(layout::kIpadPlayCaptionSize));
        g.setColour(palette::kTextDim);
        g.drawText(label, bounds.removeFromTop(10.0f), juce::Justification::centred, true);

        auto track = bounds.reduced(4.0f, 2.0f);
        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.75f));
        g.drawRoundedRectangle(track.reduced(0.5f), 4.0f, 1.0f);

        auto fill = track.reduced(2.0f);
        const float fillHeight = fill.getHeight() * vu.rmsNorm();
        auto fillRect = fill.withTop(fill.getBottom() - fillHeight);
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.fillRoundedRectangle(fillRect, 2.0f);

        const float peakY = fill.getBottom() - fill.getHeight() * vu.peakHoldNorm();
        g.setColour(palette::kAccentWarm.withAlpha(0.9f));
        g.fillRect(fill.getX() + 1.0f, peakY - 1.0f, fill.getWidth() - 2.0f, 2.0f);
    }

    void MasterOutputDeck::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(juce::Colour(0xff050608));
        g.fillRoundedRectangle(bounds, static_cast<float>(layout::kDesktopPlayModeOscilloscopeCornerRadius));
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.reduced(0.75f), static_cast<float>(layout::kDesktopPlayModeOscilloscopeCornerRadius),
                               1.5f);

        g.setFont(fonts::label(layout::kIpadPlayLabelSize));
        g.setColour(palette::kTextSecondary);
        g.drawText("MASTER OUTPUT", bounds.getX() + 14.0f, bounds.getY() + 8.0f, 120.0f, 12.0f,
                   juce::Justification::centredLeft, true);

        paintVerticalMeter(g, leftMeterBounds_.toFloat(), leftVu_, "L");
        paintVerticalMeter(g, rightMeterBounds_.toFloat(), rightVu_, "R");
        paintGainReductionMeter(g, grMeterBounds_.toFloat(), grMeterDb_);
        paintSidechainViz(g, sidechainVizBounds_.toFloat());
    }

    void MasterOutputDeck::resized()
    {
        auto bounds = getLocalBounds().reduced(12, 10);
        auto header = bounds.removeFromTop(18);
        enableButton_.setBounds(header.removeFromRight(72).reduced(0, 1));

        modePillRowBounds_ = bounds.removeFromTop(22);
        bounds.removeFromTop(4);
        sidechainVizBounds_ = bounds.removeFromBottom(28);
        bounds.removeFromBottom(6);

        const int meterWidth = 24;
        leftMeterBounds_ = bounds.removeFromLeft(meterWidth);
        grMeterBounds_ = bounds.removeFromRight(meterWidth);
        rightMeterBounds_ = bounds.removeFromRight(meterWidth);
        bounds.removeFromLeft(4);
        bounds.removeFromRight(4);

        const int pillW = (modePillRowBounds_.getWidth() - 6) / 4;
        for (std::size_t i = 0; i < modePills_.size(); ++i)
        {
            auto pill = modePillRowBounds_.removeFromLeft(pillW).reduced(1, 0);
            modePills_[i].setBounds(pill);
            modePillRowBounds_.removeFromLeft(2);
        }

        const int knobSize = juce::jmin(layout::kIpadPlayMasterKnobSize, bounds.getWidth(), bounds.getHeight());
        masterKnob_->setMaxDialDiameter(knobSize);
        masterKnob_->setBounds(bounds.withSizeKeepingCentre(knobSize, knobSize));
    }

} // namespace pw8::plugin::ui
