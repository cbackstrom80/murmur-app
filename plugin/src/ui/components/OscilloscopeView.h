#pragma once

#include <array>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Time-domain oscilloscope fed by the master-bus audio tap (~30 Hz refresh).
    class OscilloscopeView : public juce::Component, private juce::Timer
    {
    public:
        explicit OscilloscopeView(PatchworkEightProcessor& processor);

        ~OscilloscopeView() override;

        void paint(juce::Graphics& g) override;

    private:
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        static constexpr int kCaptureSize = 512;
        std::array<float, static_cast<std::size_t>(kCaptureSize)> capture_{};
        int captureCount_ = 0;
        bool hasData_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilloscopeView)
    };

} // namespace pw8::plugin::ui
