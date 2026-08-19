#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// CPU / latency / grains telemetry strip (Figma `102:4` footer).
    class QuasarTelemetryBar : public juce::Component
    {
    public:
        explicit QuasarTelemetryBar(MurmurProcessor& processor);

        void tick();
        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        MurmurProcessor& processor_;
        juce::Label cpuLabel_;
        juce::Label latencyLabel_;
        juce::Label grainsLabel_;
    };

} // namespace pw8::plugin::ui
