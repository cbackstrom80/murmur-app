#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// Round on/off control with accent glow ring — shared by FX slot enable, slot
// selection, and filter bypass for a consistent PLAY-mode toggle language.
namespace pw8::plugin::ui
{
    class GlowRingButton : public juce::Button
    {
    public:
        explicit GlowRingButton(const juce::String& name = {});

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

        void setAccentColour(juce::Colour colour);
        /// Brighter outer ring when this chip is the active FX slot selector.
        void setSelectionHighlight(bool highlighted);

    private:
        juce::Colour accent_ = juce::Colours::white;
        bool selectionHighlight_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlowRingButton)
    };

} // namespace pw8::plugin::ui
