#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "components/AmpEnvelopePanel.h"
#include "components/ArpLauncherChip.h"
#include "components/ArpPanelOverlay.h"
#include "components/CompactModeEditor.h"
#include "components/ContextStrip.h"
#include "components/FilterLfoPanel.h"
#include "components/FxChainStrip.h"
#include "components/ModAssignmentController.h"
#include "components/ModLauncherPanel.h"
#include "components/ModRoutingOverlay.h"
#include "components/NodeSelectorRow.h"
#include "components/OperatorEditorPanel.h"
#include "components/PatchBrowserBar.h"
#include "components/PatchFocusPanel.h"
#include "components/PresetBrowserOverlay.h"
#include "components/SectionPanel.h"
#include "content/FavoritesStore.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    class PlayModeEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer
    {
    public:
        explicit PlayModeEditor(PatchworkEightProcessor& processor);
        ~PlayModeEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        enum class ViewMode
        {
            Basic,
            Advanced,
            Compact,
        };

        enum class Page
        {
            Osc = 0,
            Filter,
            Env,
            Mod,
            Fx,
        };

        void setViewMode(ViewMode mode);
        void applyWindowConstraints();
        void showPage(Page page);
        void updateScopeUi();
        void refreshFilterPanelScope();
        void updateModAssignmentBanner();
        void openModRoutingOverlay();
        void closeModRoutingOverlay();
        void openArpPanel();
        void closeArpPanel();

        ModAssignmentController modAssignmentController_;
        ObsidianLookAndFeel lookAndFeel_;
        juce::TooltipWindow tooltipWindow_{this};

        PatchBrowserBar patchBrowserBar_;
        ArpLauncherChip arpLauncherChip_;
        NodeSelectorRow nodeSelectorRow_;
        ContextStrip contextStrip_;
        PatchFocusPanel patchFocusPanel_;
        CompactModeEditor compactEditor_;
        juce::TextButton basicViewButton_{"Basic"};
        juce::TextButton advancedViewButton_{"Advanced"};
        juce::TextButton compactViewButton_{"Compact"};
        std::array<juce::TextButton, 5> tabButtons_{
            juce::TextButton{"OSC"},
            juce::TextButton{"FILTER"},
            juce::TextButton{"ENV"},
            juce::TextButton{"MOD"},
            juce::TextButton{"FX"},
        };
        ViewMode viewMode_ = ViewMode::Basic;
        Page currentPage_ = Page::Filter;
        juce::Component oscPage_;
        juce::Component filterPage_;
        juce::Component envPage_;
        juce::Component modPage_;
        juce::Component fxPage_;

        juce::Label modAssignmentBanner_;
        SectionPanel oscPanel_{"Operator"};
        OperatorEditorPanel operatorEditorPanel_;
        FilterLfoPanel filterLfoPanel_;
        AmpEnvelopePanel ampEnvelopePanel_;
        ModLauncherPanel modLauncherPanel_;
        FxChainStrip fxChainStrip_;
        content::FavoritesStore favoritesStore_;
        PresetBrowserOverlay presetBrowserOverlay_;
        ModRoutingOverlay modRoutingOverlay_;
        ArpPanelOverlay arpPanelOverlay_;
        juce::ComponentBoundsConstrainer aspectConstrainer_;
        juce::ComponentBoundsConstrainer compactConstrainer_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayModeEditor)
    };

} // namespace pw8::plugin::ui
