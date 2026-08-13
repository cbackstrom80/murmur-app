#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    /// DESIGN mode edge-list editor for Layer A's algorithm graph.
    class AlgorithmGraphEditor : public juce::Component
    {
    public:
        explicit AlgorithmGraphEditor(PatchworkEightProcessor& processor);

        void resized() override;

        /// Reload working copy from the processor's current patch.
        void refreshFromPatch();

        ~AlgorithmGraphEditor() override;

        /// Fired after a successful Apply (graph committed + engine reloaded).
        std::function<void()> onGraphApplied;

    private:
        class EdgeRow;

        void ensureDefaultNodes();
        void rebuildEdgeRows();
        void recompilePreview();
        void addEdge();
        void applyEdits();
        void revertEdits();
        [[nodiscard]] bool workingCopyMatchesPatch() const;

        PatchworkEightProcessor& processor_;
        algorithm::AlgorithmGraphDefinition workingCopy_;
        algorithm::AlgorithmGraphDefinition committedCopy_;

        juce::Label nodesHeader_{"", "Nodes (toggle output)"};
        std::array<juce::ToggleButton, pw8::core::kNodesPerLayer> outputToggles_;

        juce::Label edgesHeader_{"", "Edges"};
        juce::TextButton addEdgeButton_{"+ Add Edge"};
        juce::Viewport edgeViewport_;
        juce::Component edgeListHost_;
        juce::OwnedArray<EdgeRow> edgeRows_;

        juce::Label compileStatus_{"", "Compile: —"};
        juce::TextButton applyButton_{"Apply to Patch"};
        juce::TextButton revertButton_{"Revert"};

        algorithm::CompileStatus lastStatus_ = algorithm::CompileStatus::Ok;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlgorithmGraphEditor)
    };

} // namespace pw8::plugin::ui
