#include "MacroStrip.h"

#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    MacroStrip::MacroStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        addAndMakeVisible(panel_);
        for (std::size_t i = 0; i < knobs_.size(); ++i)
        {
            knobs_[i] = std::make_unique<GlowKnob>(apvts, kMacroParameterIds[i], kMacroParameterNames[i]);
            panel_.addAndMakeVisible(*knobs_[i]);
        }
    }

    void MacroStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto bounds = panel_.getContentBounds();
        const int knobWidth = bounds.getWidth() / static_cast<int>(knobs_.size());
        for (auto& knob : knobs_)
            knob->setBounds(bounds.removeFromLeft(knobWidth).reduced(4));
    }

} // namespace pw8::plugin::ui
