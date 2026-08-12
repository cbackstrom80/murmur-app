#pragma once

#include <array>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "ModSourceChip.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "wireframe/ModRoutingWireframeView.h"

namespace pw8::plugin::ui
{
    class ModSourceStrip : public juce::Component, private juce::Timer
    {
    public:
        explicit ModSourceStrip(PatchworkEightProcessor& processor);
        ~ModSourceStrip() override;

        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        void timerCallback() override;

        struct ConnectionRowLayout
        {
            modulation::ModRoute route;
            juce::Rectangle<int> textArea;
            juce::Rectangle<int> removeButton;
        };
        [[nodiscard]] std::vector<ConnectionRowLayout> layoutConnectionRows() const;
        [[nodiscard]] juce::Rectangle<int> connectionsAreaBounds() const;

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Mod Matrix"};
        wireframe::ModRoutingWireframeView routingWireframe_;
        juce::Label helpLabel_;
        std::array<std::unique_ptr<ModSourceChip>, 3> chips_;
        bool hasEverHadModRoute_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModSourceStrip)
    };

} // namespace pw8::plugin::ui
