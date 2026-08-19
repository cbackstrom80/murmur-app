#include "DashboardStrip.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    DashboardStrip::DashboardStrip(MurmurProcessor& processor, ModAssignmentController& modAssignmentController)
        : processor_(processor), fxChainStrip_(processor), filterLfoPanel_(processor, modAssignmentController)
    {
        fxChainStrip_.setPlayDashboardMode(true);
        filterLfoPanel_.setScope(FilterPanelScope::Global, 0);
        filterLfoPanel_.setDashboardMode(true);

        fxChainStrip_.onVocoderLabRequested = [this](std::size_t slotIndex) {
            if (onVocoderLabRequested)
                onVocoderLabRequested(slotIndex);
        };
        filterLfoPanel_.onLfoLabRequested = [this] {
            if (onLfoLabRequested)
                onLfoLabRequested();
        };

        addAndMakeVisible(fxChainStrip_);
        addAndMakeVisible(filterLfoPanel_);
        startTimerHz(10);
    }

    void DashboardStrip::timerCallback()
    {
        const int pulled =
            processor_.readScopeSamples(scopeScratch_.data(), static_cast<int>(scopeScratch_.size()));
        if (pulled > 0)
        {
            const auto [rms, peak] = scope::measureMonoBlock(scopeScratch_.data(), pulled);
            fxOutVu_.processFrame(rms, peak);
        }
        else
        {
            const float masterPeak = processor_.getMasterOutPeakLinear();
            fxOutVu_.processFrame(masterPeak * 0.707f, masterPeak);
        }

        const float synthPeak = processor_.getSynthBusPeakLinear();
        filterBusVu_.processFrame(synthPeak * 0.707f, synthPeak);

        if (!fxOutMeterBounds_.isEmpty())
            repaint(fxOutMeterBounds_);
        if (!filterOutMeterBounds_.isEmpty())
            repaint(filterOutMeterBounds_);
    }

    void DashboardStrip::paintLiveMeter(juce::Graphics& g, juce::Rectangle<int> bounds, const char* label,
                                         const scope::VuBallistics& vu) const
    {
        if (bounds.isEmpty())
            return;

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(6.5f));
        g.drawText(label, bounds.removeFromLeft(18), juce::Justification::centredLeft);

        auto track = bounds.toFloat().reduced(0.0f, 1.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(track, 2.0f);
        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.drawRoundedRectangle(track.reduced(0.5f), 2.0f, 0.8f);

        auto fill = track.reduced(1.0f);
        fill.setWidth(fill.getWidth() * vu.rmsNorm());
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(fill, 1.5f);

        const float peakX = track.getX() + track.getWidth() * vu.peakHoldNorm();
        g.setColour(palette::kAccentWarm.withAlpha(0.9f));
        g.fillRect(peakX - 0.5f, track.getY() + 1.0f, 1.0f, track.getHeight() - 2.0f);
    }

    void DashboardStrip::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);

        paintLiveMeter(g, fxOutMeterBounds_, "FX", fxOutVu_);
        paintLiveMeter(g, filterOutMeterBounds_, "FLT", filterBusVu_);
    }

    void DashboardStrip::updateMeterLayout()
    {
        fxOutMeterBounds_ = {};
        filterOutMeterBounds_ = {};

        auto fxBounds = fxChainStrip_.getBounds();
        if (!fxBounds.isEmpty())
        {
            fxOutMeterBounds_ = fxBounds.removeFromTop(12).removeFromRight(78).reduced(2, 0);
        }

        auto filterBounds = filterLfoPanel_.getBounds();
        if (!filterBounds.isEmpty())
        {
            filterOutMeterBounds_ = filterBounds.removeFromTop(12).removeFromRight(78).reduced(6, 0);
        }
    }

    void DashboardStrip::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDashboardInnerPadding);
        filterLfoPanel_.setBounds(bounds.removeFromRight(layout::kDashboardFilterPanelWidth));
        bounds.removeFromRight(layout::kDashboardFxFilterGap);
        fxChainStrip_.setBounds(bounds);
        updateMeterLayout();
    }

} // namespace pw8::plugin::ui
