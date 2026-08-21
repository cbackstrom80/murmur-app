#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DesignModeEditor.h"
#include "PlayModeEditor.h"
#include "SharedEditorChrome.h"
#include "components/CuratorReviewOverlay.h"
#include "components/KeyActivationOverlay.h"
#include "components/MurmurChromeBar.h"
#include "components/PatchBrowserBar.h"
#include "components/PresetBrowserOverlay.h"
#include "components/SplashOverlay.h"
#include "content/FavoritesStore.h"
#include "content/LicenseStore.h"
#include "content/PresetRatingsStore.h"
#include "net/MurmurApiClient.h"
#include "processor/MurmurProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// Root plugin editor — unified chrome + PLAY / DESIGN child editors.
    class MurmurRootEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit MurmurRootEditor(MurmurProcessor& processor);
        ~MurmurRootEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void wireSharedChrome();
        void applyWindowConstraints();
        void syncChromeState();
        void setEditorMode(layout::EditorMode mode);
        void setDesignSubPage(layout::DesignSubPage page);
        /// Downloads each patch's real .murmur file, routed by patch.isOwn
        /// into the local "User" preset bank (the caller's own generations)
        /// or "Community" (everyone else's) -- see PresetBrowserOverlay's
        /// real path-substring bank classification. Shared by the
        /// activation-time sync and the manual refresh below.
        void downloadAndIndexLibraryPatches(const juce::Array<net::LibraryPatch>& patches);
        /// Manual re-sync against the currently-activated license, without
        /// re-entering the key -- LicenseStore already persists it locally
        /// for exactly this. No-ops (silently) if no license is activated.
        void refreshLibraryFromServer();
        [[nodiscard]] int outerMarginForCurrentView() const;
        [[nodiscard]] juce::BorderSize<int> contentInsetsForCurrentView() const;

        ObsidianLookAndFeel lookAndFeel_;
        juce::TooltipWindow tooltipWindow_{this};

        content::FavoritesStore favoritesStore_;
        content::PresetRatingsStore ratingsStore_;
        PatchBrowserBar patchBrowserBar_;
        MurmurChromeBar murmurChromeBar_;
        PresetBrowserOverlay presetBrowserOverlay_;
        SharedEditorChrome chrome_{patchBrowserBar_, favoritesStore_, presetBrowserOverlay_};

        PlayModeEditor playModeEditor_;
        DesignModeEditor designModeEditor_;
        SplashOverlay splashOverlay_;
        KeyActivationOverlay keyActivationOverlay_;
        CuratorReviewOverlay curatorReviewOverlay_;
        juce::TextButton curatorEntryButton_{"CURATOR"}; // corner button, visible only
                                                           // when the activated license
                                                           // is curator-flagged
        juce::TextButton refreshLibraryButton_{"SYNC LIBRARY"}; // corner button, visible
                                                                  // only once a license is
                                                                  // activated -- manual
                                                                  // re-sync, see
                                                                  // refreshLibraryFromServer()
        layout::EditorMode editorMode_ = layout::EditorMode::Play;
        layout::DesignSubPage designSubPage_ = layout::DesignSubPage::Engine;
        layout::PlayViewMode lastNonCompactPlayView_ = layout::PlayViewMode::Desktop;

        juce::ComponentBoundsConstrainer aspectConstrainer_;
        juce::ComponentBoundsConstrainer compactConstrainer_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MurmurRootEditor)
    };

} // namespace pw8::plugin::ui
