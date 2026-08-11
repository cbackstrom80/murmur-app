#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

// The operator detail view for whichever algorithm-graph node is currently
// selected (AlgorithmGraphView::onNodeSelected) -- UI GATE 3, built directly off
// the "Why cant i click on other oscillators" feedback on the HTML mockup this
// was prototyped in: clicking a node in the graph should show what that
// operator's actually doing, not just its 4-letter engine tag.
//
// One panel, re-bound to a different node's parameters on selection (rebuild(),
// not 8 pre-built hidden panels) -- there's only ever one node selected at a
// time, so keeping one set of live GlowKnob attachments and swapping which
// parameter IDs they point at is simpler than juggling 8 parallel panels' worth
// of APVTS attachments.
//
// The engine row is hand-painted pills, not 8 juce::Button children -- matching
// the same manual hit-test style AlgorithmGraphView already uses, and letting an
// unimplemented engine (algorithm::isEngineImplemented()) render as visibly
// disabled (dim, no hover, a tooltip explaining why) rather than a control that
// looks live but silently does nothing if clicked -- the same "no engine
// pretends to work when it doesn't" honesty AlgorithmGraphView's dashed ring
// already commits to for these same 6 engines.
//
// Deliberately does NOT expose a Pan knob: the .pw8 schema has a per-operator
// `pan` field, but it isn't in the automatable parameter surface yet
// (state/PluginState.h's field list), so a knob for it here would silently do
// nothing -- the same reasoning PluginState.h's own "deliberately not exposed"
// list already applies elsewhere.
namespace pw8::plugin::ui
{
    class OperatorEditorPanel : public juce::Component
    {
    public:
        explicit OperatorEditorPanel(PatchworkEightProcessor& processor);

        void resized() override;
        /// Drawn via paintOverChildren(), not paint(): `panel_` is a full-bounds
        /// child (it owns the card background/border/title), so anything painted
        /// in this component's own paint() would be immediately painted over by
        /// panel_ -- the pill row and "not implemented" note need to land after
        /// every child has already painted.
        void paintOverChildren(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

        /// Re-binds every control to node `nodeIndex`'s parameters and updates
        /// the panel's header. Safe to call repeatedly (e.g. once per
        /// AlgorithmGraphView::onNodeSelected firing).
        void showNode(int nodeIndex);

    private:
        [[nodiscard]] juce::Rectangle<int> pillRowBounds() const;
        [[nodiscard]] juce::Rectangle<int> pillBounds(int engineIndex) const;

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Operator"};
        int selectedNode_ = 0;

        std::unique_ptr<GlowKnob> waveformKnob_;
        std::unique_ptr<GlowKnob> levelKnob_;
        std::unique_ptr<GlowKnob> ratioKnob_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OperatorEditorPanel)
    };

} // namespace pw8::plugin::ui
