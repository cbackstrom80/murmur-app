#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "AlgorithmGraphView.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Fullscreen dimmed overlay with the circular algorithm graph (PLAY signature moment).
    class TopologyGraphOverlay : public juce::Component
    {
    public:
        explicit TopologyGraphOverlay(PatchworkEightProcessor& processor);

        void showOverlay(int selectedNode);
        void dismiss();

        std::function<void(int nodeIndex)> onNodeSelected;
        std::function<void()> onDismissed;

        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

        PatchworkEightProcessor& processor_;
        juce::Label badgeLabel_;
        juce::Label titleLabel_;
        SectionPanel panel_{"LIVE TOPOLOGY"};
        std::unique_ptr<AlgorithmGraphView> graphView_;
        juce::TextButton closeButton_{"← PLAY BOARD"};
    };

} // namespace pw8::plugin::ui
