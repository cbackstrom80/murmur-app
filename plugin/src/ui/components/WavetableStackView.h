#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "content/WavetableIndex.h"

// A real perspective wireframe mesh of the currently-selected node's loaded
// wavetable -- classic oscilloscope/wavetable-editor look (phosphor-glow rows
// receding into depth, real occlusion where nearer rows cover farther ones,
// cross-lines tying adjacent rows into one continuous surface). Replaces the
// original flat "deck of cards" shear-based ribbons (UI GATE 5) with the same
// no-OpenGL, no-shader, no-mesh-API constraint: every row is a plain
// juce::Path, projected by hand (cheap pseudo-3D: shift + shrink per row,
// no real camera/projection matrix), and occlusion is the classic
// painter's-algorithm trick -- fill a background-colour silhouette under a
// farther row before a nearer row's line is drawn on top of it. Colour is
// OBSIDIAN's own palette::kAccent cyan, not the reference image's literal
// green phosphor, so this reads as part of the existing skin.
//
// Visual density vs. honesty: the table's real frame count (as few as 1, as
// many as several dozen in the factory library) rarely matches how many rows
// a continuous-looking mesh surface wants, so more rows than the table
// actually has are drawn, each extra row's samples linearly interpolated
// between its two real neighbouring frames. This isn't fabricated data --
// it's exactly what WavetableOscillator itself computes when WavetablePos
// scans between two frames, so the mesh changes exactly the way the audio
// does, not just decoratively.
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
// Owns every way to change which wavetable an operator points at:
// - "<"/">" -- cycle the factory wavetable library (WavetableIndex).
//   Custom JSON import exists but is hidden until explicitly needed.
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

        /// Loads the first indexed wavetable when this operator has none assigned.
        void ensureDefaultWavetableLoaded();

        /// When true, draws example grain windows over the wavetable mesh (Granular engine).
        void setGranularOverlay(bool enabled) { granularOverlay_ = enabled; repaint(); }

        /// Lab mode shows custom import alongside factory prev/next navigation.
        void setShowLoadButton(bool show);

    private:
        void timerCallback() override;
        void loadWavetableFromFile();

        /// Rescans the current wavetableId's parent directory for sibling
        /// *.json tables (for the "<"/">" arrows) and locates the current
        /// file's index. Always safe to call (empty wavetableId / no parent
        /// directory just clears the sibling list, disabling both arrows).
        void refreshSiblings();
        void goToSibling(int delta);
        [[nodiscard]] juce::String currentWavetablePath() const;

        PatchworkEightProcessor& processor_;
        content::WavetableIndex wavetableIndex_;
        int selectedNode_ = 0;
        juce::Label tableNameLabel_;
        juce::TextButton loadButton_{"Load..."};
        juce::TextButton prevButton_{"<"};
        juce::TextButton nextButton_{">"};
        std::unique_ptr<juce::FileChooser> fileChooser_;

        juce::String lastKnownWavetableId_;
        bool granularOverlay_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableStackView)
    };

} // namespace pw8::plugin::ui
