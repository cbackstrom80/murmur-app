#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "../theme/FigmaKnobTokens.h"
#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma UX-09 triple-ring control: three APVTS parameters on concentric glow arcs.
    class TripleGlowKnob : public juce::Component, public juce::DragAndDropTarget, private juce::Timer
    {
    public:
        TripleGlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& outerParamId,
                       const juce::String& middleParamId, const juce::String& innerParamId,
                       const juce::String& outerLabel, const juce::String& middleLabel, const juce::String& innerLabel,
                       std::function<juce::String(float)> outerValueToText = nullptr,
                       std::function<juce::String(float)> middleValueToText = nullptr,
                       std::function<juce::String(float)> innerValueToText = nullptr);

        ~TripleGlowKnob() override;

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;

        void enableOuterModulationTarget(PatchworkEightProcessor& processor, modulation::ModDestination destination,
                                         std::uint8_t targetIndex = 0);
        void enableMiddleModulationTarget(PatchworkEightProcessor& processor, modulation::ModDestination destination,
                                          std::uint8_t targetIndex = 0);
        void enableInnerModulationTarget(PatchworkEightProcessor& processor, modulation::ModDestination destination,
                                         std::uint8_t targetIndex = 0);
        void setModAssignmentController(ModAssignmentController* controller);

        void setExpandedReadout(bool expanded);
        void setMaxDialDiameter(int diameter);
        void applyFigmaContext(figma::KnobContext context);

    private:
        struct ModRingState
        {
            modulation::ModDestination destination = modulation::ModDestination::None;
            std::uint8_t targetIndex = 0;
            modulation::ModSource source = modulation::ModSource::None;
            juce::Colour colour = juce::Colours::transparentBlack;
            float amountNormalized = 0.0f;
            bool dragHover = false;
            bool showDepthPopover = false;
            juce::Rectangle<int> depthPopoverArea_;
            bool depthDragActive = false;
            float depthDragStartAmount = 0.0f;
            float depthDragStartX = 0.0f;
            float liveModNormalized = -1.0f;
        };

        [[nodiscard]] float readProportional(int channel) const;
        [[nodiscard]] juce::String formatValue(int channel) const;
        [[nodiscard]] juce::Rectangle<float> dialBounds() const;
        [[nodiscard]] ModRingState& modStateForChannel(int channel);
        [[nodiscard]] ModRingState& modStateAtPoint(juce::Point<int> localPos);

        void adjustFocusedChannel(float deltaY);
        void refreshModRingState(ModRingState& state, const juce::String& paramId);
        void paintModRing(juce::Graphics& g, const ModRingState& state, float orbitRadius, float orbitScale) const;
        void handleModMouseDown(const juce::MouseEvent& event, ModRingState& state);
        void handleModMouseDrag(const juce::MouseEvent& event, ModRingState& state);
        void timerCallback() override;

        bool isInterestedInDragSource(const SourceDetails& details) override;
        void itemDragEnter(const SourceDetails& details) override;
        void itemDragExit(const SourceDetails& details) override;
        void itemDropped(const SourceDetails& details) override;

        juce::AudioProcessorValueTreeState& apvts_;
        juce::String outerParamId_;
        juce::String middleParamId_;
        juce::String innerParamId_;
        juce::String outerLabel_;
        juce::String middleLabel_;
        juce::String innerLabel_;
        std::function<juce::String(float)> outerValueToText_;
        std::function<juce::String(float)> middleValueToText_;
        std::function<juce::String(float)> innerValueToText_;

        ModAssignmentController* modAssignment_ = nullptr;
        PatchworkEightProcessor* modProcessor_ = nullptr;
        ModRingState outerMod_;
        ModRingState middleMod_;
        ModRingState innerMod_;

        int focusedChannel_ = 0;
        bool expandedReadout_ = false;
        bool valueDragActive_ = false;
        int maxDialDiameter_ = 120;
        float dragStartValue_ = 0.0f;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TripleGlowKnob)
    };

} // namespace pw8::plugin::ui
