#include "SectionPanel.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kShadowMargin = 5; // Reserved border so the shadow has room to bleed within our own bounds.
        constexpr int kTitleHeight = 24;
        constexpr int kPadding = 8;
    } // namespace

    SectionPanel::SectionPanel(const juce::String& title, juce::Colour accentColour, bool preserveTitleCase)
        : title_(preserveTitleCase ? title : title.toUpperCase()),
          accentColour_(accentColour.isTransparent() ? palette::kAccent : accentColour)
    {
    }

    namespace
    {
        juce::Colour sectionTitleColour(juce::Colour accentColour)
        {
            if (accentColour == palette::kAccentWarm)
                return palette::kAccentWarm.brighter(0.22f);
            return palette::kTextPrimary;
        }
    } // namespace

    void SectionPanel::setShowChrome(bool show)
    {
        showChrome_ = show;
        repaint();
    }

    void SectionPanel::paint(juce::Graphics& g)
    {
        if (!showChrome_)
            return;

        auto cardBounds = getLocalBounds().reduced(kShadowMargin).toFloat();

        // Soft drop shadow: three decreasing-alpha passes offset downward, wider
        // each time -- cheap to paint, reads as a real soft shadow rather than a
        // single hard-edged offset rectangle. This, not just the border, is what
        // makes the card feel like it's sitting proud of the background.
        for (int i = 3; i >= 1; --i)
        {
            const float spread = static_cast<float>(i) * 1.6f;
            g.setColour(palette::kShadow.withAlpha(0.09f * static_cast<float>(i)));
            g.fillRoundedRectangle(cardBounds.translated(0.0f, spread * 0.6f).expanded(spread * 0.3f), 7.0f);
        }

        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(cardBounds, 6.0f);

        // Faint top highlight -- a single-pixel-ish lighter line along the card's
        // top edge, the other half of the "milled panel" read alongside the shadow.
        {
            juce::Path topEdge;
            topEdge.addRoundedRectangle(cardBounds, 6.0f);
            g.saveState();
            g.reduceClipRegion(cardBounds.toNearestInt().withHeight(3));
            g.setColour(palette::kTopHighlight);
            g.strokePath(topEdge, juce::PathStrokeType(1.5f));
            g.restoreState();
        }

        g.setColour(palette::kBorder);
        g.drawRoundedRectangle(cardBounds.reduced(0.5f), 6.0f, 1.0f);

        if (title_.isNotEmpty())
        {
            constexpr float kDotDiameter = 5.0f;
            auto titleRow = cardBounds.removeFromTop(static_cast<float>(kTitleHeight)).reduced(kPadding, 0.0f);

            const auto dotBounds = titleRow.removeFromLeft(kDotDiameter).withSizeKeepingCentre(kDotDiameter, kDotDiameter);
            g.setColour(accentColour_);
            g.fillEllipse(dotBounds);

            titleRow.removeFromLeft(6.0f);
            g.setColour(sectionTitleColour(accentColour_));
            g.setFont(fonts::title(fonts::kSectionTitleSize));
            g.drawText(title_, titleRow, juce::Justification::centredLeft);

            // A hairline divider under the header row, the "card header" treatment
            // that separates title from content the way a tab strip would, without
            // adding actual tab navigation (PLAY mode has exactly one screen).
            // `cardBounds` was already shrunk by the removeFromTop() above, so its
            // current Y is exactly where the title row ends.
            g.setColour(palette::kBorder);
            const float dividerY = cardBounds.getY();
            g.drawLine(cardBounds.getX() + kPadding, dividerY, cardBounds.getRight() - kPadding, dividerY, 1.0f);
        }
    }

    void SectionPanel::resized() {}

    void SectionPanel::setTitle(const juce::String& title)
    {
        title_ = title;
        repaint();
    }

    juce::Rectangle<int> SectionPanel::getContentBounds() const
    {
        if (!showChrome_)
            return getLocalBounds();

        auto bounds = getLocalBounds().reduced(kShadowMargin);
        if (title_.isNotEmpty())
            bounds.removeFromTop(kTitleHeight);
        return bounds.reduced(kPadding);
    }

} // namespace pw8::plugin::ui
