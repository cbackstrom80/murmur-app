#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The recessed-panel container every PLAY-mode section sits inside -- a titled
// card with a soft drop shadow, a hairline border, and a faint top highlight,
// like a milled panel set into a chassis. Purely a paint()/layout wrapper:
// callers add their own content component as a child and position it inside
// getContentBounds().
namespace pw8::plugin::ui
{
    class SectionPanel : public juce::Component
    {
    public:
        /// `accentColour`, if not transparent (the default cool cyan), tints this
        /// panel's header dot -- used by MacroStrip for the duotone's warm half.
        explicit SectionPanel(const juce::String& title, juce::Colour accentColour = juce::Colours::transparentBlack);

        void paint(juce::Graphics& g) override;
        void resized() override;

        /// The area inside the panel a caller should lay its own content into.
        [[nodiscard]] juce::Rectangle<int> getContentBounds() const;

    private:
        juce::String title_;
        juce::Colour accentColour_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionPanel)
    };

} // namespace pw8::plugin::ui
