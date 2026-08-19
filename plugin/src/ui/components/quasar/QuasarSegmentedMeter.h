#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/MurmurProcessor.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Figma `102:4` — 12-segment IN/OUT LED columns (synth bus vs master out peaks).
    class QuasarSegmentedMeter : public juce::Component
    {
    public:
        static constexpr int kNumSegments = 12;

        explicit QuasarSegmentedMeter(MurmurProcessor& processor);

        void tick();
        void paint(juce::Graphics& g) override;

    private:
        void paintColumn(juce::Graphics& g, juce::Rectangle<float> bounds, const char* label,
                         const scope::VuBallistics& vu) const;

        MurmurProcessor& processor_;
        scope::VuBallistics inVu_;
        scope::VuBallistics outVu_;
    };

} // namespace pw8::plugin::ui
