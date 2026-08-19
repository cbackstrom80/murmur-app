#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace pw8::plugin::ui
{
    /// Small, unobtrusive "a newer version exists" pill — invisible unless
    /// a real update-check (net::UpdateChecker) confirms one. Clicking
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
        void resized() override;

        /// Kicks off the real network check. Safe to call once, shortly
        /// after the editor opens -- becomes visible only if an update is
        /// actually found.
        void checkNow();

    private:
        juce::HyperlinkButton link_;
        juce::String latestVersion_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UpdateBadge)
    };

} // namespace pw8::plugin::ui
