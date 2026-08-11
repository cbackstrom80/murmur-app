#pragma once

#include <array>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"
#include "WavetableStackView.h"
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
//
// UI GATE 5: when the selected node's engine is Wavetable, the Wave/Ratio knobs
// (meaningless for that engine -- classicWaveform/frequencyRatio aren't what a
// wavetable operator reads) are replaced by WavetableStackView plus a WT POS
// knob -- the latter previously had NO UI anywhere despite being a real
// automatable parameter (PluginState.h's "WavetablePos" field). Level stays for
// every engine, wavetable or not. A Timer (not just showNode()) drives this
// switch since the engine can change without a node reselection -- clicking a
// different engine pill on the SAME node, or a host loading a new patch/preset.
// UI GATE 5 (tooltips): implements juce::TooltipClient rather than making each
// pill its own real juce::Component -- ObsidianLookAndFeel already themed
// TooltipWindow's colours, but no TooltipWindow instance existed anywhere to
// use them (see PlayModeEditor.h), and no getTooltip() existed to feed one even
// once it did. getTooltip() below hit-tests the mouse position against the same
// pillBounds() mouseDown() already uses, so this is a few lines rather than
// restructuring 8 hand-painted pills into child components.
//
// Engine Type 3 (FM/PM), the first of the 6 previously-silent engines to ship:
// the FM pill unblocks automatically (algorithm::isEngineImplemented() is the
// single source of truth every gate here already reads, updated centrally in
// AlgorithmTypes.hpp -- nothing pill/tooltip/note-specific to change). Adds 4
// knobs for the self-contained internal modulator (Ratio/Index/Feedback/Wave),
// shown alongside the carrier's own Wave/Level/Ratio rather than replacing
// them -- an FM/PM node's carrier is a real, still-relevant Classic oscillator
// under the hood (see op::OperatorState::render()'s FmPm case).
//
// Engine 7 (NoiseChaos): when the selected node's engine is NoiseChaos, the
// Wave knob is replaced by Noise Variant / Noise Rate (Ratio stays -- see the
// ratioKnob_ comment further down) -- same visibility-swap pattern the
// Wavetable engine established above, a further branch alongside it and
// FM/PM's rather than a generalized N-engine knob system (a bespoke
// per-engine layout stays simpler than a generic one while the engine count
// is still small).
namespace pw8::plugin::ui
{
    class OperatorEditorPanel : public juce::Component, public juce::TooltipClient, private juce::Timer
    {
    public:
        explicit OperatorEditorPanel(PatchworkEightProcessor& processor);
        ~OperatorEditorPanel() override;

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

        /// juce::TooltipClient -- only non-empty over a currently-disabled
        /// (unimplemented) engine pill, explaining why it's dim/unclickable. Empty
        /// string over everything else, which JUCE's TooltipWindow treats as "no
        /// tooltip" rather than showing a blank bubble.
        [[nodiscard]] juce::String getTooltip() override;

    private:
        [[nodiscard]] juce::Rectangle<int> pillRowBounds() const;
        [[nodiscard]] juce::Rectangle<int> pillBounds(int engineIndex) const;
        /// Which engine pill (0..kNumEngines-1) contains `pos`, or -1 if none --
        /// shared by mouseDown() and getTooltip() so the hit-test region can't
        /// drift out of sync between click and hover behaviour.
        [[nodiscard]] int pillIndexAt(juce::Point<int> pos) const;
        /// Constructs all four knobs bound to selectedNode_'s parameters. Only
        /// needed when selectedNode_ itself changes -- every knob's parameter ID
        /// depends on the node, not the engine, so an engine-only change (see
        /// updateEngineVisibility()) never needs to touch these.
        void rebuildKnobsForNode();
        /// Applies the Wave/Ratio-vs-WavetableStackView+WTPos visibility split for
        /// the CURRENT engine value and calls resized() -- shared by showNode()
        /// (after rebuildKnobsForNode()) and timerCallback()/mouseDown() (engine
        /// changed without a node change). Deliberately never reconstructs a knob:
        /// doing so used to abort any in-progress mouse drag on that knob the
        /// moment a host automated the Engine parameter mid-gesture.
        void updateEngineVisibility();
        [[nodiscard]] int currentEngineOrdinal() const noexcept;

        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Operator"};
        int selectedNode_ = 0;
        int lastKnownEngine_ = -1; // -1 forces updateEngineDependentLayout() on first showNode().

        std::unique_ptr<GlowKnob> waveformKnob_;
        std::unique_ptr<GlowKnob> levelKnob_;
        std::unique_ptr<GlowKnob> ratioKnob_;
        std::unique_ptr<GlowKnob> wavetablePosKnob_; // Only constructed/visible for the Wavetable engine.
        WavetableStackView wavetableStackView_;      // Same visibility rule; never rebuilt per-node (holds no APVTS attachment).
        // Engine Type 3 (FM/PM) only -- same "always constructed, visibility
        // toggled" pattern as the Wavetable-only knobs above.
        std::unique_ptr<GlowKnob> fmModRatioKnob_;
        std::unique_ptr<GlowKnob> fmModIndexKnob_;
        std::unique_ptr<GlowKnob> fmModFeedbackKnob_;
        std::unique_ptr<GlowKnob> fmModWaveformKnob_;
        std::unique_ptr<GlowKnob> noiseVariantKnob_; // Only visible for the NoiseChaos engine.
        std::unique_ptr<GlowKnob> noiseRateKnob_;    // Only visible for the NoiseChaos engine.

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OperatorEditorPanel)
    };

} // namespace pw8::plugin::ui
