#include "GlowRingButton.h"

#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    GlowRingButton::GlowRingButton(const juce::String& name) : juce::Button(name)
    {
        setClickingTogglesState(true);
        accent_ = palette::kAccent;
    }

    void GlowRingButton::setAccentColour(juce::Colour colour) { accent_ = colour; }

    void GlowRingButton::setSelectionHighlight(bool highlighted)
    {
        if (selectionHighlight_ != highlighted)
        {
            selectionHighlight_ = highlighted;
            repaint();
        }
    }

    void GlowRingButton::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
    {
        auto bounds = getLocalBounds().toFloat().reduced(1.0f);
        const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight());
        const auto circle = bounds.withSizeKeepingCentre(diameter, diameter);
        const float radius = diameter * 0.5f;
        const bool on = getToggleState();

        if (on || selectionHighlight_)
        {
            const float glowAlpha = on ? 0.35f : 0.18f;
            g.setColour(accent_.withAlpha(glowAlpha));
            g.fillEllipse(circle.expanded(radius * 0.22f));
        }

        g.setColour(palette::kPanelRaised);
        g.fillEllipse(circle);

        const float ringWidth = selectionHighlight_ ? 2.2f : 1.6f;
        g.setColour(on ? accent_ : palette::kBorderBright);
        g.drawEllipse(circle.reduced(ringWidth * 0.5f), ringWidth);

        if (on)
        {
            g.setColour(accent_);
            g.fillEllipse(circle.reduced(radius * 0.38f));
        }

        if (shouldDrawButtonAsHighlighted || shouldDrawButtonAsDown)
        {
            g.setColour(accent_.withAlpha(shouldDrawButtonAsDown ? 0.45f : 0.25f));
            g.drawEllipse(circle.expanded(1.5f), 1.0f);
        }
    }

} // namespace pw8::plugin::ui
