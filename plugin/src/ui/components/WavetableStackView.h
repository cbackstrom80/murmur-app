#pragma once

#include <memory>

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
// Wired into OperatorEditorPanel: it swaps this view in for the Wave/Ratio
// knobs when the selected node's engine is Wavetable. See
// docs/VISUALIZATION_UI_GATE5.md "Integration" for the rationale.
//
// Also owns the only UI anywhere that can actually assign a wavetable to an
// operator: a small "Load..." button, same FileChooser-based pattern as
// PatchBrowserBar's patch loader. Deliberately scoped to picking an EXISTING
// pw8-wavetable-builder JSON table (*.json), not importing a raw .wav live --
// oscillator::loadWavetableFromFile() only ever parses the JSON table format
// (see engine/src/oscillator/WavetableTableLoader.cpp), and the FFT/mip-
// generation the builder does for a raw source .wav lives only in that
// separate offline tool (tools/wavetable_builder), not as a reusable library
// call the plugin could invoke live. Wiring the builder itself into the
// message thread is a real follow-up, not blind-implemented here.
namespace pw8::plugin::ui
{
    class WavetableStackView : public juce::Component, private juce::Timer
    {
    public:
        explicit WavetableStackView(PatchworkEightProcessor& processor);
        ~WavetableStackView() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        /// Which algorithm-graph node's wavetable to display -- mirrors
        /// OperatorEditorPanel::showNode() so a caller can wire
        /// AlgorithmGraphView::onNodeSelected straight to both.
        void showNode(int nodeIndex);

    private:
        void timerCallback() override;
        void loadWavetableFromFile();

        PatchworkEightProcessor& processor_;
        int selectedNode_ = 0;
        juce::TextButton loadButton_{"Load..."};
        std::unique_ptr<juce::FileChooser> fileChooser_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableStackView)
    };

} // namespace pw8::plugin::ui
