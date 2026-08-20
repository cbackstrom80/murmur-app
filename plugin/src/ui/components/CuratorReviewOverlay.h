#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/LicenseStore.h"
#include "net/MurmurApiClient.h"

namespace pw8::plugin::ui
{
    /// Real curator review panel — lists community/factory patches from
    /// murmur-web's /api/v1/curator-queue and lets a flagged curator
    /// (LicenseStore::info().isDevCurator) vote each one up or down. A
    /// down-vote permanently (soft-)removes the patch from the community
    /// repo server-side; every vote persists as real training signal for
    /// future model-optimization work (see murmur-web's PatchCuratorVote).
    ///
    /// Only reachable when the activated license is curator-flagged (see
    /// MurmurChromeBar's menu wiring). Honest about a real limitation:
    /// metadata only (name/category/tags/source) -- no audio preview,
    /// since audio-render-on-demand isn't wired to the browser yet.
    class CuratorReviewOverlay : public juce::Component
    {
    public:
        CuratorReviewOverlay();
        ~CuratorReviewOverlay() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void visibilityChanged() override;

        std::function<void()> onDismissed;
        std::function<juce::String()> getLicenseKey; // supplied by the owner (MurmurRootEditor)

        void voteOnRow(int row, bool voteUp);

    private:
        class Model;

        void refresh();

        juce::Label titleLabel_;
        juce::Label statusLabel_;
        juce::TextButton closeButton_{"CLOSE"};
        juce::ListBox listBox_;
        std::unique_ptr<Model> model_;
        juce::Array<net::CuratorQueuePatch> patches_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CuratorReviewOverlay)
    };

} // namespace pw8::plugin::ui
