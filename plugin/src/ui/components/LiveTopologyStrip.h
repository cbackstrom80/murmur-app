#pragma once

#include <array>
#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    /// Compact 8-node topology strip for PLAY Basic/Advanced — tap to expand full graph.
    class LiveTopologyStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit LiveTopologyStrip(PatchworkEightProcessor& processor);
        ~LiveTopologyStrip() override;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

        void setSelectedNode(int nodeIndex);
        [[nodiscard]] int getSelectedNode() const noexcept { return selectedNode_; }

        /// Pulse edges connected to this operator (MVP performance feedback).
        void setActiveOperator(int nodeIndex);

        /// Brief performance pulse — e.g. KOINS knob or MW/EXP moved.
        void triggerPerformancePulse();

        std::function<void(int nodeIndex)> onNodeSelected;
        std::function<void()> onExpandRequested;

    private:
        void timerCallback() override;

        [[nodiscard]] juce::Point<float> nodePosition(std::size_t index, juce::Rectangle<float> area) const;
        [[nodiscard]] bool edgeTouchesNode(const algorithm::AlgorithmEdge& edge, int nodeIndex,
                                           const algorithm::AlgorithmGraphDefinition& algo) const;

        PatchworkEightProcessor& processor_;
        float pulsePhase_ = 0.0f;
        float performancePulse_ = 0.0f;
        int selectedNode_ = 0;
        int activeOperator_ = 0;
    };

} // namespace pw8::plugin::ui
