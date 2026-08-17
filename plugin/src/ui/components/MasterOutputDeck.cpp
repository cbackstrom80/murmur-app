#include "MasterOutputDeck.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    MasterOutputDeck::MasterOutputDeck(PatchworkEightProcessor& processor) : processor_(processor)
    {
        masterKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "MASTER", nullptr, palette::kAccent);
        masterKnob_->setHeaderCompactMode(true);
        addAndMakeVisible(*masterKnob_);
        startTimerHz(30);
    }

    MasterOutputDeck::~MasterOutputDeck() { stopTimer(); }

    void MasterOutputDeck::timerCallback()
    {
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

        repaint(leftMeterBounds_);
        repaint(rightMeterBounds_);
    }

    void MasterOutputDeck::paintVerticalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                              const scope::VuBallistics& vu, const char* label) const
    {
        if (bounds.isEmpty())
            return;

        g.setFont(fonts::label(7.0f));
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

        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);
        g.drawText("MASTER OUTPUT", bounds.getX() + 14.0f, bounds.getY() + 12.0f, 120.0f, 12.0f,
                   juce::Justification::centredLeft, true);

        paintVerticalMeter(g, leftMeterBounds_.toFloat(), leftVu_, "L");
        paintVerticalMeter(g, rightMeterBounds_.toFloat(), rightVu_, "R");
    }

    void MasterOutputDeck::resized()
    {
        auto bounds = getLocalBounds().reduced(12, 10);
        bounds.removeFromTop(18);

        const int meterWidth = 28;
        leftMeterBounds_ = bounds.removeFromLeft(meterWidth);
        rightMeterBounds_ = bounds.removeFromRight(meterWidth);
        bounds.removeFromLeft(6);
        bounds.removeFromRight(6);

        const int knobSize = juce::jmin(layout::kIpadPlayMasterKnobSize, bounds.getWidth(), bounds.getHeight());
        masterKnob_->setMaxDialDiameter(knobSize);
        masterKnob_->setBounds(bounds.withSizeKeepingCentre(knobSize, knobSize));
    }

} // namespace pw8::plugin::ui
