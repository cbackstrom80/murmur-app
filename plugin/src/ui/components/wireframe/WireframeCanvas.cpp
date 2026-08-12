#include "WireframeCanvas.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

namespace pw8::plugin::ui::wireframe
{
    WireframeCanvas::WireframeCanvas()
    {
        captionLabel_.setJustificationType(juce::Justification::centred);
        captionLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        captionLabel_.setFont(fonts::label(11.0f));
        addAndMakeVisible(captionLabel_);
    }

    WireframeCanvas::~WireframeCanvas() = default;

    void WireframeCanvas::resized()
    {
        auto bounds = getLocalBounds().reduced(kMargin);
        captionLabel_.setBounds(bounds.removeFromBottom(kCaptionHeight));
    }

    void WireframeCanvas::setCaption(const juce::String& text)
    {
        captionLabel_.setText(text, juce::dontSendNotification);
    }

    juce::Rectangle<float> WireframeCanvas::meshBounds() const
    {
        auto bounds = getLocalBounds().toFloat().reduced(static_cast<float>(kMargin));
        bounds.removeFromBottom(static_cast<float>(kCaptionHeight + kMargin));
        return bounds;
    }

    void WireframeCanvas::paintEmptyMessage(juce::Graphics& g, juce::Rectangle<float> bounds,
                                             const juce::String& message) const
    {
        g.setColour(palette::kTextDim);
        g.setFont(fonts::value(11.0f));
        g.drawText(message, bounds, juce::Justification::centred);
    }

    void WireframeCanvas::paintSubCaption(juce::Graphics& g, juce::Rectangle<float>& meshArea) const
    {
        if (subCaption_.isEmpty())
            return;
        auto area = meshArea.removeFromBottom(static_cast<float>(kSubCaptionHeight));
        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::value(10.0f));
        g.drawText(subCaption_, area, juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui::wireframe
