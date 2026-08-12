#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/AmpEnvelopePanel.h"
#include "components/FilterLfoPanel.h"
#include "components/FxChainStrip.h"
#include "components/MacroStrip.h"
#include "components/ModSourceStrip.h"
#include "components/NodeSelectorRow.h"
#include "components/OperatorEditorPanel.h"
#include "components/PatchBrowserBar.h"
#include "components/PresetBrowserOverlay.h"
#include "components/SectionPanel.h"
#include "content/FavoritesStore.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

// PLAY mode editor with a paged layout (docs/UI_PAGED_LAYOUT.md): persistent
// patch bar + compact node selector, then tabbed Basic/OSC/Filter/Mod/FX pages.
namespace pw8::plugin::ui
{
    class PlayModeEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer
    {
    public:
        explicit PlayModeEditor(PatchworkEightProcessor& processor);
        ~PlayModeEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        enum class Page
        {
            Basic = 0,
            Osc,
            Filter,
            Env,
            Mod,
            Fx,
        };

        void showPage(Page page);

        PatchworkEightProcessor& processor_;
        ObsidianLookAndFeel lookAndFeel_;
        juce::TooltipWindow tooltipWindow_{this};

        PatchBrowserBar patchBrowserBar_;
        NodeSelectorRow nodeSelectorRow_;
        std::array<juce::TextButton, 6> tabButtons_{
            juce::TextButton{"BASIC"},
            juce::TextButton{"OSC"},
            juce::TextButton{"FILTER"},
            juce::TextButton{"ENV"},
            juce::TextButton{"MOD"},
            juce::TextButton{"FX"},
        };
        Page currentPage_ = Page::Basic;

        juce::Component basicPage_;
        juce::Component oscPage_;
        juce::Component filterPage_;
        juce::Component envPage_;
        juce::Component modPage_;
        juce::Component fxPage_;

        SectionPanel macroPanel_{"Performance Macros"};
        MacroStrip macroStrip_;
        SectionPanel oscPanel_{"Operator"};
        OperatorEditorPanel operatorEditorPanel_;
        FilterLfoPanel filterLfoPanel_;
        AmpEnvelopePanel ampEnvelopePanel_;
        ModSourceStrip modSourceStrip_;
        FxChainStrip fxChainStrip_;
        content::FavoritesStore favoritesStore_;
        PresetBrowserOverlay presetBrowserOverlay_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayModeEditor)
    };

} // namespace pw8::plugin::ui
