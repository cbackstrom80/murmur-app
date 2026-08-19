#include "QuasarSegmentedMeter.h"

#include "../../theme/ObsidianDraw.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    QuasarSegmentedMeter::QuasarSegmentedMeter(PatchworkEightProcessor& processor) : processor_(processor) {}

    void QuasarSegmentedMeter::tick()
    {
        const float synthPeak = processor_.getSynthBusPeakLinear();
        inVu_.processFrame(synthPeak * 0.707f, synthPeak);

        const float masterPeak = processor_.getMasterOutPeakLinear();
        outVu_.processFrame(masterPeak * 0.707f, masterPeak);

        repaint();
    }

    void QuasarSegmentedMeter::paintColumn(juce::Graphics& g, juce::Rectangle<float> bounds, const char* label,
                                           const scope::VuBallistics& vu) const
    {
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText(label, bounds.removeFromTop(12.0f), juce::Justification::centred, true);

        auto track = bounds.reduced(2.0f, 4.0f);
        draw::fillRecessedRoundedRect(g, track, 4.0f);

        const float litNorm = juce::jmax(vu.rmsNorm(), vu.peakHoldNorm() * 0.85f);
        const int litSegments = juce::jlimit(0, kNumSegments, static_cast<int>(std::ceil(litNorm * kNumSegments)));

        const float gap = 1.0f;
        const float segH = (track.getHeight() - gap * static_cast<float>(kNumSegments - 1))
                           / static_cast<float>(kNumSegments);

        for (int i = 0; i < kNumSegments; ++i)
        {
            const int segmentIndex = kNumSegments - 1 - i;
            const bool lit = segmentIndex < litSegments;
            const float y = track.getY() + static_cast<float>(i) * (segH + gap);
            auto seg = juce::Rectangle<float>(track.getX() + 3.0f, y, track.getWidth() - 6.0f, segH);
            const float t = static_cast<float>(segmentIndex) / static_cast<float>(kNumSegments - 1);
            const juce::Colour colour =
                lit ? juce::Colour(0xff00c8ff).interpolatedWith(juce::Colour(0xffe040fb), t).withAlpha(0.85f)
                    : palette::kPanelRaised.withAlpha(0.65f);
            g.setColour(colour);
            g.fillRoundedRectangle(seg, 1.5f);
        }
    }

    void QuasarSegmentedMeter::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);

        auto columns = bounds.reduced(8.0f, 6.0f);
        const float colW = (columns.getWidth() - 12.0f) * 0.5f;
        paintColumn(g, columns.removeFromLeft(colW), "IN", inVu_);
        columns.removeFromLeft(12.0f);
        paintColumn(g, columns, "OUT", outVu_);
    }

} // namespace pw8::plugin::ui
