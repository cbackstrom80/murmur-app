#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModSourceChip.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

// The drag-to-modulate source palette (docs/UI.md): a small row of colored,
// draggable chips. Deliberately just the sources meaningful against today's two
// real PLAY-mode destinations (Filter Cutoff/Resonance, see FilterLfoPanel) --
// LFO1, the amp envelope, and Velocity -- not the full 29-source mod matrix,
// which stays PLANNED alongside a real matrix UI.
//
// Also owns the connections list (UI GATE 3, following the HTML mockup this was
// validated in): a plain-text readout of every currently-active mod route --
// "LFO 1 -> FILTER CUTOFF", one per line, each with a small remove button --
// living below the chip row in the same card. GlowKnob's colored ring already
// shows *that* a knob is modulated at a glance; this list is the same
// information in words, and (unlike the ring, which only covers the two
// drag-enabled destinations) reads every route in the patch honestly, including
// ones a hand-authored .pw8 preset created that this UI has no drag gesture
// for yet (e.g. an OperatorLevel route).
namespace pw8::plugin::ui
{
    class ModSourceStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit ModSourceStrip(PatchworkEightProcessor& processor);
        ~ModSourceStrip() override;

        void resized() override;
        /// Drawn after children paint, same reasoning as OperatorEditorPanel:
        /// `panel_` is a full-bounds child that owns the card background, so
        /// anything this component paints itself would otherwise be painted over.
        void paintOverChildren(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        void timerCallback() override;

        struct ConnectionRowLayout
        {
            modulation::ModRoute route;
            juce::Rectangle<int> textArea;
            juce::Rectangle<int> removeButton;
        };
        [[nodiscard]] std::vector<ConnectionRowLayout> layoutConnectionRows() const;
        [[nodiscard]] juce::Rectangle<int> connectionsAreaBounds() const;

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Mod Sources -- Drag Onto A Ringed Knob"};
        std::array<std::unique_ptr<ModSourceChip>, 3> chips_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceStrip)
    };

} // namespace pw8::plugin::ui
