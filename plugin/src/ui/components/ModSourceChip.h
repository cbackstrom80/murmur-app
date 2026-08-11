#pragma once

#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "pw8/modulation/ModMatrixTypes.hpp"

// A small, colored, draggable chip representing one modulation source -- the
// actual core interaction docs/UI.md's research pass identified as the thing
// that makes Serum/Vital feel alive: click-drag a source chip onto any knob
// that accepts modulation (see GlowKnob's drag-target half of this mechanism)
// to assign it, no menu, no matrix-first workflow. A dedicated PatchworkEight
// synth doesn't have a full mod matrix UI yet (docs/UI.md "What's PLANNED"),
// so this chip set is deliberately small -- exactly the sources meaningful
// against today's two real PLAY-mode destinations (Filter Cutoff/Resonance):
// LFO1, the amp envelope, and Velocity.
namespace pw8::plugin::ui
{
    /// The drag-source description ModSourceChip posts and GlowKnob parses --
    /// exposed as free functions (not a private implementation detail of either
    /// class) so both sides of the drag/drop pair always agree on the encoding.
    [[nodiscard]] juce::String modSourceDragDescription(modulation::ModSource source);

    /// Returns the source if `description` is a chip drag (matches the encoding
    /// above), or std::nullopt for anything else (a file drag, another plugin's
    /// drag source, ...) -- GlowKnob's isInterestedInDragSource uses this directly.
    [[nodiscard]] std::optional<modulation::ModSource> parseModSourceDragDescription(const juce::String& description);

    /// Human-readable labels for the connections list (ModSourceStrip) -- cover
    /// every ModSource/ModDestination value, not just the 3 sources/2
    /// destinations this pass offers as drag chips/drop targets, since a loaded
    /// .pw8 patch can reference any of them (e.g. gate4-massive-dark-metallic-
    /// bass.pw8's Lfo1 -> OperatorLevel route) and the list should describe the
    /// real patch honestly, not just what's reachable by dragging in this UI.
    /// `modSourceLabel(Env1)` deliberately returns "AMP ENV", matching
    /// ModSourceStrip's chip label for that same source, rather than the more
    /// literal "ENV 1" -- so a connection made by dragging that chip reads back
    /// identically to how it was created.
    [[nodiscard]] juce::String modSourceLabel(modulation::ModSource source);
    [[nodiscard]] juce::String modDestinationLabel(modulation::ModDestination destination, std::uint8_t targetIndex);

    class ModSourceChip : public juce::Component
    {
    public:
        ModSourceChip(modulation::ModSource source, const juce::String& label, juce::Colour colour);

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;

    private:
        modulation::ModSource source_;
        juce::String label_;
        juce::Colour colour_;
        bool dragStarted_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceChip)
    };

} // namespace pw8::plugin::ui
