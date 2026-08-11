#include "MacroStrip.h"

#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    MacroStrip::MacroStrip(PatchworkEightProcessor& processor) : processor_(processor)
    {
        addAndMakeVisible(panel_);
        for (std::size_t i = 0; i < knobs_.size(); ++i)
        {
            // The warm half of the duotone: macros are the one surface a player's
            // hands are actually on, not a structural/signal reading (see
            // ObsidianPalette.h). Starts on the generic name; timerCallback()
            // switches it to the patch-authored one, if any, once a patch loads.
            knobs_[i] = std::make_unique<GlowKnob>(processor_.apvts, kMacroParameterIds[i], kMacroParameterNames[i],
                                                     nullptr, palette::kAccentWarm);
            panel_.addAndMakeVisible(*knobs_[i]);
        }

        startTimerHz(2); // Macro names only change on patch load -- an infrequent poll is plenty.
    }

    MacroStrip::~MacroStrip()
    {
        stopTimer();
    }

    void MacroStrip::timerCallback()
    {
        const auto& macros = processor_.getCurrentPatch().macros;
        for (std::size_t i = 0; i < knobs_.size() && i < macros.size(); ++i)
        {
            // Empty patch-authored name (an init patch, or a hand-authored .pw8
            // that never set one) falls back to the generic "Macro N" rather than
            // showing a blank label.
            const auto desiredName =
                macros[i].name.empty() ? juce::String(kMacroParameterNames[i]) : juce::String(macros[i].name);
            // Unconditional: GlowKnob::setDisplayName() -> Label::setText() already
            // no-ops (no repaint, no side effects) when the text hasn't changed
            // (juce::Label::lastTextValue check), so a second "did this change"
            // guard here would only duplicate that with a second source of truth
            // to keep in sync.
            knobs_[i]->setDisplayName(desiredName);
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
