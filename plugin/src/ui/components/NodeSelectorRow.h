#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    class NodeSelectorRow : public juce::Component
    {
    public:
        std::function<void(int nodeIndex)> onNodeSelected;
        std::function<void()> onGlobalSelected;

        explicit NodeSelectorRow(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        void setSelectedNode(int nodeIndex);
        void setGlobalScope(bool global);
        [[nodiscard]] bool isGlobalScope() const noexcept { return globalScope_; }
        [[nodiscard]] int getSelectedNode() const noexcept { return selectedNode_; }

    private:
        [[nodiscard]] juce::Rectangle<int> pillBounds(int nodeIndex) const;
        [[nodiscard]] juce::Rectangle<int> globalPillBounds() const;

        PatchworkEightProcessor& processor_;
        int selectedNode_ = 0;
        bool globalScope_ = false;
    };

} // namespace pw8::plugin::ui
