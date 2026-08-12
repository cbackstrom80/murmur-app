#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "WireframeProjection.h"

namespace pw8::plugin::ui::wireframe
{
    /// Base for engine wireframe previews: caption label + mesh bounds helper.
    class WireframeCanvas : public juce::Component
    {
    public:
        WireframeCanvas();
        ~WireframeCanvas() override;

        void resized() override;

        void setCaption(const juce::String& text);
        void setSubCaption(const juce::String& text) { subCaption_ = text; }

        [[nodiscard]] juce::Rectangle<float> meshBounds() const;
        void paintEmptyMessage(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& message) const;

        /// Draws sub-caption into the bottom of `meshArea`, shrinking it in-place.
        void paintSubCaption(juce::Graphics& g, juce::Rectangle<float>& meshArea) const;

    protected:
        static constexpr int kCaptionHeight = 18;
        static constexpr int kSubCaptionHeight = 14;
        static constexpr int kMargin = 4;

        juce::Label captionLabel_;
        juce::String subCaption_;
    };

} // namespace pw8::plugin::ui::wireframe
