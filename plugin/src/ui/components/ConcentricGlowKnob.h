#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"
#include "../theme/DualKnobLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// Dual-parameter Murmur dial: outer ring = param A (e.g. filter cutoff, WT bend),
    /// inner cap = param B (e.g. resonance, WT asym). Both remain separate APVTS
    /// parameters for host automation. Two stacked Sliders with per-ring LookAndFeel
    /// (Curtis reference pattern) — NOT decorative deck depth (see UI_DIFFERENTIATION_BRIEF.md).
    class ConcentricGlowKnob : public juce::Component, public juce::DragAndDropTarget, private juce::Timer
    {
    public:
        ConcentricGlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& innerParamId,
                           const juce::String& outerParamId, const juce::String& innerLabel,
                           const juce::String& outerLabel,
                           std::function<juce::String(float)> innerValueToText = nullptr,
                           std::function<juce::String(float)> outerValueToText = nullptr);

        ~ConcentricGlowKnob() override;

        void resized() override;
        void paintOverChildren(juce::Graphics& g) override;

        void enableInnerModulationTarget(PatchworkEightProcessor& processor,
                                         modulation::ModDestination destination, std::uint8_t targetIndex = 0);
        void enableOuterModulationTarget(PatchworkEightProcessor& processor,
                                         modulation::ModDestination destination, std::uint8_t targetIndex = 0);

        void setModAssignmentController(ModAssignmentController* controller);
        void setMaxDialDiameter(int diameter);

        /// Category color for the inner cap (outer ring always uses structural kAccent arc).
        void setInnerAccentColour(juce::Colour colour);

    private:
        class FormattedSlider : public juce::Slider
        {
        public:
            std::function<juce::String(float)> valueToText;
            juce::String getTextFromValue(double value) override;
        };

        void configureSlider(FormattedSlider& slider);
        void timerCallback() override;
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
        };

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

        bool isInterestedInDragSource(const SourceDetails& details) override;
        void itemDragEnter(const SourceDetails& details) override;
        void itemDragExit(const SourceDetails& details) override;
        void itemDropped(const SourceDetails& details) override;

        [[nodiscard]] ModRingState& modStateForEvent(const juce::MouseEvent& event);
        [[nodiscard]] ModRingState& modStateAtPoint(juce::Point<int> localPos);
        [[nodiscard]] bool isOuterRingPoint(juce::Point<int> localPos) const;

        void refreshModRingState(ModRingState& state);
        void paintModRing(juce::Graphics& g, const ModRingState& state, float orbitRadius, bool outerOrbit) const;
        void handleModMouseDown(const juce::MouseEvent& event, ModRingState& state);
        void handleModMouseDrag(const juce::MouseEvent& event, ModRingState& state);
        void syncFocusProperties();
        void setFocusedChannel(int channel);

        int focusedChannel_ = 0;
        DualKnobLookAndFeel outerLookAndFeel_{DualKnobLookAndFeel::KnobType::Outer};
        DualKnobLookAndFeel innerLookAndFeel_{DualKnobLookAndFeel::KnobType::Inner};
        FormattedSlider outerSlider_;
        FormattedSlider innerSlider_;
        juce::Label innerLabel_;
        juce::Label outerLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> innerAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outerAttachment_;

        ModAssignmentController* modAssignment_ = nullptr;
        PatchworkEightProcessor* modProcessor_ = nullptr;
        ModRingState innerMod_;
        ModRingState outerMod_;
        int maxDialDiameter_ = 88;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConcentricGlowKnob)
    };

    /// Alias — concentric knobs are functional dual-parameter controls, not depth decoration.
    using ConcentricDualKnob = ConcentricGlowKnob;

} // namespace pw8::plugin::ui
