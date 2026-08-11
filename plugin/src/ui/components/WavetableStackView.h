#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

// A pseudo-3D "deck of cards" view of the currently-selected node's loaded
// wavetable: each frame painted as a waveform ribbon, offset and skewed so the
// stack reads as receding into the screen, decreasing in opacity with depth.
// Deliberately NOT real 3D -- no juce::OpenGLContext, no shaders, no mesh/camera:
// every frame is a flat juce::Path drawn with an AffineTransform shear+translate,
// which is enough to read as dimensional at this scale and costs nothing beyond
// what every other OBSIDIAN component already pays for (plain juce::Graphics
// painting). See docs/VISUALIZATION_UI_GATE5.md for why this stays 2D-tricks
// rather than a real 3D engine, and for the two views (spectrum, oscilloscope)
// that DO need new plumbing (a realtime audio-thread tap) this view doesn't.
//
// Needs no audio-thread tap at all: PatchworkEightProcessor::getActiveWavetableTable()
// reads table data that's already sitting in memory once loadPatch() has run (see
// that accessor's own doc comment for why it's safe to read from the UI/message
// thread through the same atomic Engine pointer processBlock() uses). This is
// exactly category 3 ("static, no audio tap needed") from
// docs/PLUGIN_ARCHITECTURE.md's "Visualization" section.
//
// NOT wired into PlayModeEditor yet -- see docs/VISUALIZATION_UI_GATE5.md
// "Integration" for the suggested spot and why it's left as a follow-up rather
// than force-fit into the current layout budget sight-unseen (no compiler
// available to verify a layout change against the real window).
namespace pw8::plugin::ui
{
    class WavetableStackView : public juce::Component, private juce::Timer
    {
    public:
        explicit WavetableStackView(PatchworkEightProcessor& processor);
        ~WavetableStackView() override;

        void paint(juce::Graphics& g) override;

        /// Which algorithm-graph node's wavetable to display -- mirrors
        /// OperatorEditorPanel::showNode() so a caller can wire
        /// AlgorithmGraphView::onNodeSelected straight to both.
        void showNode(int nodeIndex);

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        int selectedNode_ = 0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableStackView)
    };

} // namespace pw8::plugin::ui
