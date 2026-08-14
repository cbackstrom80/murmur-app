#pragma once

#include "ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// Per-slider LookAndFeel for functional concentric dual knobs (Curtis reference pattern).
    /// Stack an Outer + Inner juce::Slider; each slider owns its own DualKnobLookAndFeel instance.
    class DualKnobLookAndFeel : public ObsidianLookAndFeel
    {
    public:
        enum class KnobType
        {
            Inner,
            Outer,
        };

        explicit DualKnobLookAndFeel(KnobType type) : knobType_(type) {}

        void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional,
                              float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override;

    private:
        KnobType knobType_;
    };

} // namespace pw8::plugin::ui
