#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "AlgorithmGraphView.h"
#include "SectionPanel.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Fullscreen dimmed overlay with the circular algorithm graph (PLAY signature moment).
    class TopologyGraphOverlay : public juce::Component
    {
    public:
        explicit TopologyGraphOverlay(MurmurProcessor& processor);

        void showOverlay(int selectedNode);
        void dismiss();

        std::function<void(int nodeIndex)> onNodeSelected;
        std::function<void()> onDismissed;

        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

        MurmurProcessor& processor_;
        juce::Label badgeLabel_;
        juce::Label titleLabel_;
        SectionPanel panel_{"LIVE TOPOLOGY"};
        std::unique_ptr<AlgorithmGraphView> graphView_;
        juce::TextButton closeButton_{"← PLAY BOARD"};
    };

} // namespace pw8::plugin::ui
