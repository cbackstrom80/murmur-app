#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

// The recessed-panel container every PLAY-mode section sits inside -- a titled
// rectangle with a hairline border and a soft inner shadow, like a milled panel
// set into a chassis. Purely a paint()/layout wrapper: callers add their own
// content component as a child and position it inside getContentBounds().
namespace pw8::plugin::ui
{
    class SectionPanel : public juce::Component
    {
    public:
        explicit SectionPanel(const juce::String& title);

        void paint(juce::Graphics& g) override;
        void resized() override;

        /// The area inside the panel a caller should lay its own content into.
        [[nodiscard]] juce::Rectangle<int> getContentBounds() const;

    private:
        juce::String title_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionPanel)
    };

} // namespace pw8::plugin::ui
