#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Dual-parameter Murmur dial: inner ring = param A (e.g. filter cutoff), outer orbit = param B
    /// (e.g. resonance). Both remain separate APVTS parameters for host automation.
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

    private:
        class FormattedSlider : public juce::Slider
        {
        public:
            std::function<juce::String(float)> valueToText;
            juce::String getTextFromValue(double value) override;
        };

        enum class ActiveRing
        {
            Inner,
            Outer,
        };

        void configureSlider(FormattedSlider& slider, const juce::String& ringRole);
        void timerCallback() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;
        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

        bool isInterestedInDragSource(const SourceDetails& details) override;
        void itemDragEnter(const SourceDetails& details) override;
        void itemDragExit(const SourceDetails& details) override;
        void itemDropped(const SourceDetails& details) override;

        [[nodiscard]] ActiveRing ringAtPoint(juce::Point<float> localPos) const;
        [[nodiscard]] juce::Slider& activeSlider() noexcept;
        void forwardMouseToActiveSlider(const juce::MouseEvent& event, bool isDown);

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

        void refreshModRingState(ModRingState& state);
        void paintModRing(juce::Graphics& g, const ModRingState& state, float orbitRadius, bool outerOrbit) const;
        void handleModMouseDown(const juce::MouseEvent& event, ModRingState& state);
        void handleModMouseDrag(const juce::MouseEvent& event, ModRingState& state);

        FormattedSlider innerSlider_;
        FormattedSlider outerSlider_;
        juce::Label innerLabel_;
        juce::Label outerLabel_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> innerAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outerAttachment_;

        ModAssignmentController* modAssignment_ = nullptr;
        PatchworkEightProcessor* modProcessor_ = nullptr;
        ModRingState innerMod_;
        ModRingState outerMod_;
        ActiveRing activeRing_ = ActiveRing::Inner;
        int maxDialDiameter_ = 88;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConcentricGlowKnob)
    };

} // namespace pw8::plugin::ui
