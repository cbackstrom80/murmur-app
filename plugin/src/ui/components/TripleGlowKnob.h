#pragma once

#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// Figma UX-09 triple-ring control: three APVTS parameters on concentric glow arcs.
    class TripleGlowKnob : public juce::Component
    {
    public:
        TripleGlowKnob(juce::AudioProcessorValueTreeState& apvts, const juce::String& outerParamId,
                       const juce::String& middleParamId, const juce::String& innerParamId,
                       const juce::String& outerLabel, const juce::String& middleLabel, const juce::String& innerLabel,
                       std::function<juce::String(float)> outerValueToText = nullptr,
                       std::function<juce::String(float)> middleValueToText = nullptr,
                       std::function<juce::String(float)> innerValueToText = nullptr);

        void paint(juce::Graphics& g) override;
        void resized() override;

        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;

        void setExpandedReadout(bool expanded);
        void setMaxDialDiameter(int diameter);

    private:
        [[nodiscard]] float readProportional(int channel) const;
        [[nodiscard]] juce::String formatValue(int channel) const;
        void adjustFocusedChannel(float deltaY);

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

        int focusedChannel_ = 0;
        bool expandedReadout_ = false;
        int maxDialDiameter_ = 120;
        float dragStartValue_ = 0.0f;
    };

} // namespace pw8::plugin::ui
