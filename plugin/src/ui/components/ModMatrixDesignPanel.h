#pragma once

#include <memory>
#include <optional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "ModSourcePalette.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// DESIGN Matrix tab — view/add/remove mod routes with amount editing (MVP).
    class ModMatrixDesignPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit ModMatrixDesignPanel(PatchworkEightProcessor& processor);

        void refreshFromPatch();
        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        struct ConnectionRowLayout
        {
            modulation::ModRoute route;
            juce::Rectangle<int> textArea;
            juce::Rectangle<int> amountArea;
            juce::Rectangle<int> removeButton;
        };

        struct DestChoice
        {
            modulation::ModDestination destination = modulation::ModDestination::None;
            bool usesTargetIndex = false;
            juce::String label;
        };

        void rebuildDestChoices();
        void addRouteFromSelection();
        [[nodiscard]] std::vector<ConnectionRowLayout> layoutConnectionRows() const;
        void beginAmountDrag(const modulation::ModRoute& route, float startX);
        void continueAmountDrag(float currentX);
        void endAmountDrag();
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        ModAssignmentController assignmentController_;
        ModSourcePalette sourcePalette_;
        juce::Label destHint_{"", "Destination:"};
        juce::ComboBox destCombo_;
        juce::Label targetHint_{"", "Op:"};
        juce::ComboBox targetCombo_;
        juce::TextButton addRouteButton_{"Add Route"};
        juce::Label routesHint_{"", "Routes (multiple per destination sum):"};
        juce::Rectangle<int> connectionsArea_;

        std::vector<DestChoice> destChoices_;

        struct AmountDragState
        {
            modulation::ModRoute route;
            float startAmount = 0.0f;
            float startX = 0.0f;
        };
        std::optional<AmountDragState> amountDrag_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixDesignPanel)
    };

} // namespace pw8::plugin::ui
