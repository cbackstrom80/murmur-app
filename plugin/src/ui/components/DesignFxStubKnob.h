#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// Non-APVTS rotary for Figma design-FX placeholder knobs (TONE, COLOR, MOOD INTENSITY, etc.).
    class DesignFxStubKnob : public juce::Component
    {
    public:
        explicit DesignFxStubKnob(juce::String label);

        void setNormalizedValue(float value);
        [[nodiscard]] float normalizedValue() const { return value_; }

        std::function<void(float)> onValueChanged;

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

    private:
        void setValueFromDrag(const juce::MouseEvent& event);

        juce::String label_;
        float value_ = 0.5f;
        float dragStartValue_ = 0.5f;
        juce::Point<int> dragStart_;
    };

} // namespace pw8::plugin::ui
