#include "OscilloscopeView.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    OscilloscopeView::OscilloscopeView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(30);
    }

    OscilloscopeView::~OscilloscopeView()
    {
        stopTimer();
    }

    void OscilloscopeView::timerCallback()
    {
        const int pulled = processor_.readScopeSamples(capture_.data(), kCaptureSize);
        if (pulled > 8)
        {
            captureCount_ = pulled;
            hasData_ = true;
            repaint();
        }
    }

    void OscilloscopeView::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(2.0f);
        draw::fillRecessedRoundedRect(g, bounds, 6.0f);

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(fonts::kCaptionSize));
        g.drawText("SCOPE", bounds.removeFromTop(14.0f).reduced(6.0f, 0.0f), juce::Justification::centredLeft);

        auto plot = bounds.reduced(6.0f, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(plot.getCentreY()), plot.getX(), plot.getRight());

        if (!hasData_ || captureCount_ < 8)
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(fonts::kCaptionSize));
            g.drawText("Play notes to see waveform", plot, juce::Justification::centred);
            return;
        }

        juce::Path wave;
        const float midY = plot.getCentreY();
        const float halfH = plot.getHeight() * 0.42f;
        const float xStep = plot.getWidth() / static_cast<float>(captureCount_ - 1);

        for (int i = 0; i < captureCount_; ++i)
        {
            const float x = plot.getX() + static_cast<float>(i) * xStep;
            const float y = midY - juce::jlimit(-1.0f, 1.0f, capture_[static_cast<std::size_t>(i)]) * halfH;
            if (i == 0)
                wave.startNewSubPath(x, y);
            else
                wave.lineTo(x, y);
        }

        draw::strokeGlowPath(g, wave, 0.55f, 1.8f, true);
        g.setColour(palette::kAccent.withAlpha(0.9f));
        g.strokePath(wave, juce::PathStrokeType(1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

} // namespace pw8::plugin::ui
