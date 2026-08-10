#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

// PLAY mode's centerpiece and the component that actually differentiates this
// synth visually: a read-only rendering of Layer A's live 8-node algorithm graph
// -- the same structure AlgorithmGraphCompiler compiles and `pw8-graph inspect`
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
// docs/ROADMAP.md). This view is read-only, driven by a Timer polling the
// processor's current patch (cheap: 8 nodes/16 edges, no allocation) since
// there's no push-based "patch changed" callback yet.
//
// The animated pulse traveling along each edge is a *structural* signal-flow
// indicator (which edges exist and what type they are), not literal audio-level
// metering -- exposing real per-edge signal energy from the audio thread would
// need a dedicated lock-free reporting path that doesn't exist yet, so this is
// an honest, explicitly-scoped v1 rather than a fake meter.
namespace pw8::plugin::ui
{
    class AlgorithmGraphView : public juce::Component, private juce::Timer
    {
    public:
        explicit AlgorithmGraphView(PatchworkEightProcessor& processor);
        ~AlgorithmGraphView() override;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        [[nodiscard]] juce::Point<float> nodePosition(std::size_t index, std::size_t nodeCount,
                                                        juce::Rectangle<float> area) const;

        PatchworkEightProcessor& processor_;
        float pulsePhase_ = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmGraphView)
    };

} // namespace pw8::plugin::ui
