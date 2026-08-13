#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "DesignModeEditor.h"
#include "PlayModeEditor.h"
#include "SharedEditorChrome.h"
#include "components/PatchBrowserBar.h"
#include "components/PresetBrowserOverlay.h"
#include "content/FavoritesStore.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// Root plugin editor — PLAY | DESIGN mode toggle, shared patch browser, child editors.
    class MurmurRootEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit MurmurRootEditor(PatchworkEightProcessor& processor);
        ~MurmurRootEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        enum class AppMode
        {
            Play,
            Design,
        };

        void setAppMode(AppMode mode);
        void wireSharedChrome();
        void applyWindowConstraints();
        void syncChromeVisibility();

        ObsidianLookAndFeel lookAndFeel_;
        juce::TooltipWindow tooltipWindow_{this};

        content::FavoritesStore favoritesStore_;
        PatchBrowserBar patchBrowserBar_;
        PresetBrowserOverlay presetBrowserOverlay_;
        SharedEditorChrome chrome_{patchBrowserBar_, favoritesStore_, presetBrowserOverlay_};

        juce::TextButton playModeButton_{"PLAY"};
        juce::TextButton designModeButton_{"DESIGN"};
        PlayModeEditor playModeEditor_;
        DesignModeEditor designModeEditor_;

        AppMode appMode_ = AppMode::Play;
        juce::ComponentBoundsConstrainer aspectConstrainer_;
        juce::ComponentBoundsConstrainer compactConstrainer_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MurmurRootEditor)
    };

} // namespace pw8::plugin::ui
