#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "UpdateBadge.h"

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
    ///
    /// Real per-product branding seam (Branding, below) — the no-arg
    /// constructor still produces MURMUR's exact real visuals, unchanged;
    /// Undertow's own binary passes its own Branding instead.
    class SplashOverlay : public juce::Component, private juce::Timer
    {
    public:
        /// Real per-product branding seam so Undertow's own separate binary
        /// (docs/UNDERTOW.md, real approved `undertow-vst-splash` Figma
        /// frame) shows its own splash instead of MURMUR's -- an image +
        /// colour + 2 strings, not a forked class. `backgroundImage`
        /// invalid/default means "no photo asset for this product" -- paint()
        /// falls back to a real procedural radial-glow-and-rings background
        /// tinted with `accentColour` instead (matches this codebase's
        /// existing all-vector convention; no sprite pipeline needed for
        /// one screen).
        struct Branding
        {
            juce::Image markIcon;
            juce::Image backgroundImage;
            juce::Colour accentColour;
            juce::String tagline;
            juce::String footerLeft;

            /// Real MURMUR defaults -- byte-identical visuals to before this
            /// Branding seam existed.
            static Branding makeMurmurDefault();
        };

        SplashOverlay();
        explicit SplashOverlay(Branding branding);
        ~SplashOverlay() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        /// Called once the display+fade sequence finishes. The parent editor
        /// should remove/hide this component from that callback.
        std::function<void()> onDismissed;

    private:
        void timerCallback() override;

        Branding branding_;
        UpdateBadge updateBadge_; // matches the Figma frame's own update-notification-banner node

        double elapsedMs_ = 0.0;
        static constexpr double kDisplayMs = 1500.0;
        static constexpr double kFadeMs = 400.0;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashOverlay)
    };

} // namespace pw8::plugin::ui
