#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// Shared procedural wireframe frame — corner ticks, title dot, thin border (~1.4px),
    /// obsidian fill. Wraps filter/LFO scope, wt mesh, FX detail sections (UI Week 4).
    class WireframePanel : public juce::Component
    {
    public:
        explicit WireframePanel(const juce::String& title = {},
                                juce::Colour accentColour = juce::Colours::transparentBlack);

        void paint(juce::Graphics& g) override;
        void resized() override;

        void setTitle(const juce::String& title);
        [[nodiscard]] juce::Rectangle<int> getContentBounds() const;

        /// Static helper for painting the same frame into an arbitrary bounds (overlays).
        static void paintFrame(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& title,
                               juce::Colour accentColour);

    private:
        juce::String title_;
        juce::Colour accentColour_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WireframePanel)
    };

} // namespace pw8::plugin::ui
