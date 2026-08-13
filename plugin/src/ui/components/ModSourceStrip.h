#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "FilterLfoPanel.h"
#include "ModAssignmentController.h"
#include "ModSourcePalette.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/ModRoutingWireframeView.h"

namespace pw8::plugin::ui
{
    class ModSourceStrip : public juce::Component, private juce::Timer
    {
    public:
        ModSourceStrip(PatchworkEightProcessor& processor, ModAssignmentController& assignmentController);

        ~ModSourceStrip() override;

        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

        void setRoutingContext(FilterPanelScope scope, int engineIndex);

        /// Repaint mod source chips after armed-source changes.
        void repaintModAssignmentState();

    private:
        void timerCallback() override;
        void assignToCutoff();
        void assignToResonance();
        void refreshDestinationButtonLabels();
        void beginAmountDrag(const modulation::ModRoute& route, float startX);
        void continueAmountDrag(float currentX);
        void endAmountDrag();

        struct AmountDragState
        {
            modulation::ModRoute route;
            float startAmount = 0.0f;
            float startX = 0.0f;
        };

        std::optional<AmountDragState> amountDrag_;

        struct ConnectionRowLayout
        {
            modulation::ModRoute route;
            juce::Rectangle<int> textArea;
            juce::Rectangle<int> amountArea;
            juce::Rectangle<int> removeButton;
        };
        [[nodiscard]] std::vector<ConnectionRowLayout> layoutConnectionRows() const;
        [[nodiscard]] juce::Rectangle<int> connectionsAreaBounds() const;

        PatchworkEightProcessor& processor_;
        ModAssignmentController& assignmentController_;
        SectionPanel panel_{"Route Editor"};
        wireframe::ModRoutingWireframeView routingWireframe_;
        juce::Label helpLabel_;
        ModSourcePalette sourcePalette_;
        juce::Label destinationLabel_;
        juce::TextButton cutoffButton_{"Cutoff"};
        juce::TextButton resonanceButton_{"Resonance"};
        FilterPanelScope routingScope_ = FilterPanelScope::Global;
        int routingEngineIndex_ = 0;
        bool hasEverHadModRoute_ = false;
        juce::Rectangle<int> rightColumnBounds_;
        juce::Rectangle<int> connectionsArea_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceStrip)
    };

} // namespace pw8::plugin::ui
