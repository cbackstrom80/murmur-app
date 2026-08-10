#include "MacroStrip.h"

#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    MacroStrip::MacroStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        addAndMakeVisible(panel_);
        for (std::size_t i = 0; i < knobs_.size(); ++i)
        {
            // The warm half of the duotone: macros are the one surface a player's
            // hands are actually on, not a structural/signal reading (see
            // ObsidianPalette.h).
            knobs_[i] = std::make_unique<GlowKnob>(apvts, kMacroParameterIds[i], kMacroParameterNames[i], nullptr,
                                                     palette::kAccentWarm);
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
