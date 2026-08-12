#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

// Compact PLAY-mode node selector: eight engine-pill chips in one row (see
// docs/UI_PAGED_LAYOUT.md work-stream 1). Replaces the tall circular graph for
// node selection only -- topology display stays DESIGN-mode scope.
namespace pw8::plugin::ui
{
    class NodeSelectorRow : public juce::Component
    {
    public:
        std::function<void(int nodeIndex)> onNodeSelected;

        explicit NodeSelectorRow(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        void setSelectedNode(int nodeIndex);
        [[nodiscard]] int getSelectedNode() const noexcept { return selectedNode_; }

    private:
        [[nodiscard]] juce::Rectangle<int> pillBounds(int nodeIndex) const;

        PatchworkEightProcessor& processor_;
        int selectedNode_ = 0;
    };

} // namespace pw8::plugin::ui
