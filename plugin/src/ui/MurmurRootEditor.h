#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DesignModeEditor.h"
#include "PlayModeEditor.h"
#include "SharedEditorChrome.h"
#include "components/MurmurChromeBar.h"
#include "components/PatchBrowserBar.h"
#include "components/PresetBrowserOverlay.h"
#include "content/FavoritesStore.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// Root plugin editor — unified chrome + PLAY / DESIGN child editors.
    class MurmurRootEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit MurmurRootEditor(PatchworkEightProcessor& processor);
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
        [[nodiscard]] int outerMarginForCurrentView() const;

        ObsidianLookAndFeel lookAndFeel_;
        juce::TooltipWindow tooltipWindow_{this};

        content::FavoritesStore favoritesStore_;
        PatchBrowserBar patchBrowserBar_;
        MurmurChromeBar murmurChromeBar_;
        PresetBrowserOverlay presetBrowserOverlay_;
        SharedEditorChrome chrome_{patchBrowserBar_, favoritesStore_, presetBrowserOverlay_};

        PlayModeEditor playModeEditor_;
        DesignModeEditor designModeEditor_;
        layout::EditorMode editorMode_ = layout::EditorMode::Play;
        layout::DesignSubPage designSubPage_ = layout::DesignSubPage::Engine;
        layout::PlayViewMode lastNonCompactPlayView_ = layout::PlayViewMode::Basic;

        juce::ComponentBoundsConstrainer aspectConstrainer_;
        juce::ComponentBoundsConstrainer compactConstrainer_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MurmurRootEditor)
    };

} // namespace pw8::plugin::ui
