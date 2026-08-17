#include "DesignFxStubKnob.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    DesignFxStubKnob::DesignFxStubKnob(juce::String label) : label_(std::move(label)) {}

    void DesignFxStubKnob::setNormalizedValue(float value)
    {
        value_ = juce::jlimit(0.0f, 1.0f, value);
        repaint();
    }

    void DesignFxStubKnob::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds();
        const int dialSize = juce::jmin(bounds.getWidth(), bounds.getHeight() - 18);
        auto dial = bounds.removeFromTop(dialSize).withSizeKeepingCentre(dialSize, dialSize);

        g.setColour(palette::kPanel);
        g.fillEllipse(dial.toFloat());
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawEllipse(dial.toFloat(), 1.0f);

        const float angle = juce::MathConstants<float>::pi * 1.25f + value_ * juce::MathConstants<float>::pi * 1.5f;
        const auto centre = dial.getCentre().toFloat();
        const float radius = static_cast<float>(dialSize) * 0.38f;
        juce::Point<float> tip(centre.x + std::cos(angle) * radius, centre.y + std::sin(angle) * radius);
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.drawLine(centre.x, centre.y, tip.x, tip.y, 1.6f);

        g.setColour(palette::kTextDim.withAlpha(0.55f));
        g.setFont(fonts::label(7.0f));
        g.drawText(label_, bounds.removeFromBottom(18), juce::Justification::centred);
    }

    void DesignFxStubKnob::mouseDown(const juce::MouseEvent& event)
    {
        dragStart_ = event.getPosition();
        dragStartValue_ = value_;
    }

    void DesignFxStubKnob::mouseDrag(const juce::MouseEvent& event)
    {
        setValueFromDrag(event);
    }

    void DesignFxStubKnob::mouseUp(const juce::MouseEvent& event)
    {
        juce::ignoreUnused(event);
    }

    void DesignFxStubKnob::setValueFromDrag(const juce::MouseEvent& event)
    {
        const float delta = static_cast<float>(dragStart_.y - event.getPosition().y) * 0.008f;
        const float next = juce::jlimit(0.0f, 1.0f, dragStartValue_ + delta);
        if (next == value_)
            return;

        value_ = next;
        repaint();
        if (onValueChanged)
            onValueChanged(value_);
    }

} // namespace pw8::plugin::ui
