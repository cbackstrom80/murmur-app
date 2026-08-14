#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Unified 8-node operator strip — shared across PLAY Advanced, DESIGN wt panel, etc.
    class EngineNodeStrip : public juce::Component
    {
    public:
        std::function<void(int nodeIndex)> onNodeSelected;
        std::function<void()> onGlobalSelected;

        explicit EngineNodeStrip(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

        void setSelectedNode(int nodeIndex);
        void setGlobalScope(bool global);
        [[nodiscard]] bool isGlobalScope() const noexcept { return globalScope_; }
        [[nodiscard]] int getSelectedNode() const noexcept { return selectedNode_; }

        /// Hide GLOBAL pill (DESIGN wt panel uses node-only selection).
        void setGlobalPillVisible(bool visible);

    private:
        [[nodiscard]] juce::Rectangle<int> pillBounds(int nodeIndex) const;
        [[nodiscard]] juce::Rectangle<int> globalPillBounds() const;

        PatchworkEightProcessor& processor_;
        int selectedNode_ = 0;
        bool globalScope_ = false;
        bool globalPillVisible_ = true;
    };

    /// Backward-compatible alias — prefer EngineNodeStrip in new code.
    using NodeSelectorRow = EngineNodeStrip;

} // namespace pw8::plugin::ui
