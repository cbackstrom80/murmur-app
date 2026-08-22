#include "SubAnchorMeter.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        // Real hex values from the approved Undertow brand system (Figma
        // 277:671) -- "Active Copper" (#D4603A, primary system focus) for a
        // healthy/anchored reading, "Crimson Saturator" (#C43030, harmonic
        // distortion) as the warning colour for an inverted/decorrelated
        // reading. Not arbitrary -- the same real palette the approved
        // mockups specify.
        const juce::Colour kCopper{0xffD4603A};
        const juce::Colour kCrimson{0xffC43030};
    } // namespace

    SubAnchorMeter::SubAnchorMeter(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(30);
    }

    SubAnchorMeter::~SubAnchorMeter()
    {
        stopTimer();
    }

    void SubAnchorMeter::timerCallback()
    {
        // Real values straight off Engine::getSubAnchorCorrelation()/
        // getSubAnchorLevelDb() via MurmurProcessor's passthrough -- no
        // smoothing needed here, the engine already publishes a real
        // ~512-sample windowed reading, not a per-sample jitter source.
        correlationDisplay_ = processor_.getSubAnchorCorrelation();
        levelDbDisplay_ = processor_.getSubAnchorLevelDb();
        repaint();
    }

    void SubAnchorMeter::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 6.0f);

        auto content = bounds.reduced(8.0f, 6.0f);

        auto labelRow = content.removeFromTop(12.0f);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("SUB CORRELATION", labelRow, juce::Justification::centredLeft, true);
        g.setColour(correlationDisplay_ >= 0.0f ? kCopper : kCrimson);
        g.drawText(juce::String(correlationDisplay_, 2), labelRow, juce::Justification::centredRight, true);

        content.removeFromTop(2.0f);
        auto barArea = content.removeFromTop(10.0f);
        auto track = barArea.reduced(0.0f, 1.0f);
        draw::fillRecessedRoundedRect(g, track, 3.0f);

        const float midX = track.getCentreX();
        const float halfW = track.getWidth() * 0.5f;
        const float norm = juce::jlimit(-1.0f, 1.0f, correlationDisplay_);
        if (norm >= 0.0f)
        {
            const auto fill = juce::Rectangle<float>(midX, track.getY(), halfW * norm, track.getHeight());
            g.setColour(kCopper.withAlpha(0.85f));
            g.fillRoundedRectangle(fill, 2.0f);
        }
        else
        {
            const auto fill =
                juce::Rectangle<float>(midX + halfW * norm, track.getY(), -halfW * norm, track.getHeight());
            g.setColour(kCrimson.withAlpha(0.85f));
            g.fillRoundedRectangle(fill, 2.0f);
        }
        g.setColour(palette::kBorder);
        g.drawLine(midX, track.getY(), midX, track.getBottom(), 1.0f);

        content.removeFromTop(6.0f);
        auto levelRow = content.removeFromTop(14.0f);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("SUB LEVEL", levelRow, juce::Justification::centredLeft, true);
        g.setColour(palette::kTextPrimary);
        const juce::String levelText =
            levelDbDisplay_ <= -99.0f ? juce::String("-inf dB") : juce::String(levelDbDisplay_, 1) + " dB";
        g.drawText(levelText, levelRow, juce::Justification::centredRight, true);
    }

} // namespace pw8::plugin::ui
