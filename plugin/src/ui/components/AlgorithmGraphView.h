#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

// PLAY mode's centerpiece and the component that actually differentiates this
// synth visually: a rendering of Layer A's live 8-node algorithm graph -- the
// same structure AlgorithmGraphCompiler compiles and `pw8-graph inspect`
// already prints in text form (docs/PLUGIN_ARCHITECTURE.md "Signature UI: Graph").
//
// Deliberately NOT a draggable modular patcher: the master spec is explicit that
// this skin must never look like visible patch-cable spaghetti. The fix applied
// here is architectural, not just cosmetic -- the 8 nodes sit at fixed positions
// on a circle (topology is a schema thing edited in DESIGN mode/`pw8-graph`, not
// a PLAY-mode drag target), so what's on screen is always a clean, readable
// diagram of *this patch's specific graph*, not a general-purpose wire canvas.
//
// Editing the graph (moving nodes, adding/removing edges) is explicitly out of
// scope for PLAY mode and this pass -- DESIGN mode's job, PLANNED (see
// docs/ROADMAP.md). *Selecting* a node is in scope, though (UI GATE 3, following
// direct user feedback on the HTML mockup this was prototyped in -- "why can't I
// click on other oscillators"): clicking a node fires `onNodeSelected` so
// PlayModeEditor can show that operator's controls in OperatorEditorPanel. This
// view stays read-only about the graph's *shape* -- it never adds/removes/moves
// anything -- only which node is "focused" is interactive state.
//
// Two more mockup-validated additions live here (same "II dont understand the
// graph" feedback pass): a plain-language caption translating the current edge
// list into a sentence, and a legend mapping each edge color actually present
// in this patch to its name -- both drawn in a reserved strip below the circle,
// not a separate component, since they read straight off the same polled patch
// state the circle itself already has.
//
// Driven by a Timer polling the processor's current patch (cheap: 8 nodes/16
// edges, no allocation) since there's no push-based "patch changed" callback yet.
//
// The animated pulse traveling along each edge is a *structural* signal-flow
// indicator (which edges exist and what type they are), not literal audio-level
// metering -- exposing real per-edge signal energy from the audio thread would
// need a dedicated lock-free reporting path that doesn't exist yet, so this is
// an honest, explicitly-scoped v1 rather than a fake meter.
namespace pw8::plugin::ui
{
    /// A short (<=4-char) label for a graph node's engine -- e.g. "CLSC",
    /// "WT". Shared between the graph circle's own node labels and
    /// OperatorEditorPanel's engine pills so both always agree.
    [[nodiscard]] const char* engineShortName(algorithm::EngineType engine) noexcept;

    class AlgorithmGraphView : public juce::Component, private juce::Timer
    {
    public:
        explicit AlgorithmGraphView(PatchworkEightProcessor& processor);
        ~AlgorithmGraphView() override;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

        /// Fired with a node index [0, 8) whenever the user clicks a node.
        std::function<void(int)> onNodeSelected;

        [[nodiscard]] int getSelectedNode() const noexcept { return selectedNode_; }

    private:
        void timerCallback() override;

        [[nodiscard]] juce::Point<float> nodePosition(std::size_t index, std::size_t nodeCount,
                                                        juce::Rectangle<float> area) const;
        /// The circle-layout area edges/nodes are drawn into -- everything above
        /// the reserved caption/legend strip at the bottom of this component.
        [[nodiscard]] juce::Rectangle<float> circleArea() const;
        [[nodiscard]] juce::String buildCaption() const;

        PatchworkEightProcessor& processor_;
        float pulsePhase_ = 0.0f;
        int selectedNode_ = 0; // Matches OperatorEditorPanel's own initial showNode(0).

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmGraphView)
    };

} // namespace pw8::plugin::ui
