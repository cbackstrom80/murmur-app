#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// Real branded splash overlay shown briefly when the editor first opens —
    /// the kelp-forest whale artwork from the Figma murmur-vst-splash frame,
    /// not the "Ring wreath (8 nodes)" placeholder docs/UI_DIFFERENTIATION_BRIEF.md
    /// listed as a future idea.
    ///
    /// Deliberately honest about what the progress bar means: it fills over a
    /// fixed, real duration (kDisplayMs) rather than claiming to track any real
    /// backend loading percentage -- the plugin's own startup work (preset
    /// index scan, DSP construction) isn't instrumented to report progress,
    /// so a fabricated "67%" would be a lie about precision that doesn't
    /// exist. The status label cycles through real phase names instead of a
    /// number.
    class SplashOverlay : public juce::Component, private juce::Timer
    {
    public:
        SplashOverlay();
        ~SplashOverlay() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        /// Called once the display+fade sequence finishes. The parent editor
        /// should remove/hide this component from that callback.
        std::function<void()> onDismissed;

    private:
        void timerCallback() override;

        juce::Image backgroundImage_;
        juce::Image markIcon_;

        double elapsedMs_ = 0.0;
        static constexpr double kDisplayMs = 1500.0;
        static constexpr double kFadeMs = 400.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashOverlay)
    };

} // namespace pw8::plugin::ui
