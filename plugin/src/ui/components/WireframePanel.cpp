#include "WireframePanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kFrameInset = 4;
        constexpr int kTitleHeight = 20;
        constexpr int kPadding = 6;
        constexpr float kStrokeWidth = 1.4f;
        constexpr float kCornerTick = 9.0f;

        void paintCornerTicks(juce::Graphics& g, juce::Rectangle<float> frame, juce::Colour tickColour)
        {
            const juce::Point<float> corners[] = {
                frame.getTopLeft(),
                frame.getTopRight(),
                frame.getBottomLeft(),
                frame.getBottomRight(),
            };
            g.setColour(tickColour);
            for (const auto& c : corners)
            {
                if (c == frame.getTopLeft())
                {
                    g.drawLine(c.x, c.y, c.x + kCornerTick, c.y, kStrokeWidth);
                    g.drawLine(c.x, c.y, c.x, c.y + kCornerTick, kStrokeWidth);
                }
                else if (c == frame.getTopRight())
                {
                    g.drawLine(c.x, c.y, c.x - kCornerTick, c.y, kStrokeWidth);
                    g.drawLine(c.x, c.y, c.x, c.y + kCornerTick, kStrokeWidth);
                }
                else if (c == frame.getBottomLeft())
                {
                    g.drawLine(c.x, c.y, c.x + kCornerTick, c.y, kStrokeWidth);
                    g.drawLine(c.x, c.y, c.x, c.y - kCornerTick, kStrokeWidth);
                }
                else
                {
                    g.drawLine(c.x, c.y, c.x - kCornerTick, c.y, kStrokeWidth);
                    g.drawLine(c.x, c.y, c.x, c.y - kCornerTick, kStrokeWidth);
                }
            }
        }
    } // namespace

    WireframePanel::WireframePanel(const juce::String& title, juce::Colour accentColour)
        : title_(title), accentColour_(accentColour.isTransparent() ? palette::kAccent : accentColour)
    {
        setInterceptsMouseClicks(false, true);
    }

    void WireframePanel::setTitle(const juce::String& title)
    {
        title_ = title;
        repaint();
    }

    void WireframePanel::paintFrame(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title,
                                    juce::Colour accentColour)
    {
        const auto accent = accentColour.isTransparent() ? palette::kAccent : accentColour;
        const auto frame = bounds.reduced(0.5f);

        g.setColour(palette::kPanel.withAlpha(0.92f));
        g.fillRoundedRectangle(frame, 4.0f);

        g.setColour(accent.withAlpha(0.08f));
        g.drawRoundedRectangle(frame.expanded(0.5f), 4.5f, 0.8f);

        g.setColour(palette::kBorderBright.withAlpha(0.75f));
        g.drawRoundedRectangle(frame, 4.0f, kStrokeWidth);

        paintCornerTicks(g, frame, accent.withAlpha(0.55f));

        if (title.isNotEmpty())
        {
            auto titleRow = frame.withTrimmedBottom(frame.getHeight() - static_cast<float>(kTitleHeight))
                                .reduced(static_cast<float>(kPadding), 0.0f);
            constexpr float kDotDiameter = 4.0f;
            const auto dotBounds = titleRow.removeFromLeft(kDotDiameter).withSizeKeepingCentre(kDotDiameter, kDotDiameter);
            g.setColour(accent);
            g.fillEllipse(dotBounds);
            titleRow.removeFromLeft(5.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::label(9.0f));
            g.drawText(title.toUpperCase(), titleRow, juce::Justification::centredLeft);
        }
    }

    void WireframePanel::paint(juce::Graphics& g)
    {
        paintFrame(g, getLocalBounds().toFloat(), title_, accentColour_);
    }

    void WireframePanel::resized() {}

    juce::Rectangle<int> WireframePanel::getContentBounds() const
    {
        auto bounds = getLocalBounds().reduced(kFrameInset);
        if (title_.isNotEmpty())
            bounds.removeFromTop(kTitleHeight);
        return bounds.reduced(kPadding);
    }

} // namespace pw8::plugin::ui
