#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// "New update available" pill — matches the real Figma murmur-vst-splash
    /// frame's `update-notification-banner` node exactly (pill outline, dot,
    /// tracked label, chevron; see that frame for the reference). Invisible
    /// unless a real update-check (net::UpdateChecker) confirms one. Clicking
    /// opens the real GitHub release in the system browser; this never
    /// downloads or installs anything itself (see UpdateChecker's own doc
    /// comment for why: no Sparkle, no code-signing continuity to trust an
    /// in-place replace yet).
    class UpdateBadge : public juce::Component
    {
    public:
        UpdateBadge();
        ~UpdateBadge() override;

        void paint(juce::Graphics& g) override;
        void mouseUp(const juce::MouseEvent& event) override;

        /// Kicks off the real network check. Safe to call once, shortly
        /// after the editor opens -- becomes visible only if an update is
        /// actually found.
        void checkNow();

    private:
        juce::String labelText_;
        juce::URL releaseUrl_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdateBadge)
    };

} // namespace pw8::plugin::ui
