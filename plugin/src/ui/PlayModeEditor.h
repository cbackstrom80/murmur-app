#pragma once

#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "PlayModeLayout.h"
#include "SharedEditorChrome.h"
#include "components/AmpEnvelopePanel.h"
#include "components/BasicPerformanceSidebar.h"
#include "components/MasterEnvelopePanel.h"
#include "components/ArpPanelOverlay.h"
#include "components/CompactModeEditor.h"
#include "components/ContextStrip.h"
#include "components/DashboardStrip.h"
#include "components/DualLfoLabPanel.h"
#include "components/EngineDetailOverlay.h"
#include "components/EngineGridPanel.h"
#include "components/EngineNodeStrip.h"
#include "components/FilterLfoPanel.h"
#include "components/FxChainStrip.h"
#include "components/GlobalPanel.h"
#include "components/LiveTopologyStrip.h"
#include "components/IpadPlayMasterStrip.h"
#include "components/MasterOutputDeck.h"
#include "components/ModAssignmentController.h"
#include "components/MasterMotionLabPanel.h"
#include "components/MasterQuasarPanel.h"
#include "components/ModLauncherPanel.h"
#include "components/ModRoutingOverlay.h"
#include "components/OperatorEditorPanel.h"
#include "components/OscilloscopeView.h"
#include "components/PatchFocusPanel.h"
#include "components/SectionPanel.h"
#include "components/TopologyGraphOverlay.h"
#include "components/VocoderLabPanel.h"
#include "components/WavetableLabPanel.h"
#include "components/VstBottomBar.h"
#include "content/PresetIndex.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    class PlayModeEditor : public juce::Component, public juce::DragAndDropContainer
    {
    public:
        PlayModeEditor(PatchworkEightProcessor& processor, SharedEditorChrome& chrome);
        ~PlayModeEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        bool keyPressed(const juce::KeyPress& key) override;

        void refreshFromPatch();
        void setBrowseFilter(const content::PresetMetadataFilter& filter);
        [[nodiscard]] bool isCompactView() const noexcept;
        [[nodiscard]] layout::PlayViewMode getPlayViewMode() const noexcept { return viewMode_; }
        void setPlayViewMode(layout::PlayViewMode mode);
        void openArpDrawer();
        void openFilterPage();
        void syncIpadFooterPill();

        std::function<void()> onLayoutOrViewModeChanged;
        std::function<void(layout::EditorMode)> onEditorModeChangeRequested;
        std::function<void(layout::DesignSubPage)> onDesignSubPageChangeRequested;

    private:
        enum class Page
        {
            Osc = 0,
            Filter,
            Env,
            Mod,
            Fx,
            Global,
        };

        enum class AdvancedSubview
        {
            Board,
            Paged,
        };

        void showPage(Page page);
        void showBoard();
        void layoutAdvancedTabRow(juce::Rectangle<int> tabRow);
        void updateScopeUi();
        void refreshFilterPanelScope();
        void updateModAssignmentBanner();
        void openModRoutingOverlay();
        void closeModRoutingOverlay();
        void openArpPanel();
        void closeArpPanel();

        void openGraphOverlay();
        void closeGraphOverlay();
        void syncNodeSelection(int nodeIndex);

        void openEngineDetail(int engineIndex);
        void closeEngineDetail();

        void openVocoderLab(std::size_t fxSlotIndex);
        void closeVocoderLab();
        void openDualLfoLab();
        void closeDualLfoLab();
        void openMasterMotionLab();
        void closeMasterMotionLab();
        void openMasterQuasarLab(std::size_t fxSlotIndex);
        void closeMasterQuasarLab();
        [[nodiscard]] bool isMasterQuasarLabOpen() const noexcept
        {
            return playLabOverlay_ == layout::PlayLabOverlay::Quasar;
        }
        void openWavetableLab(int engineIndex);
        void closeWavetableLab();
        void requestDesignSubPage(layout::DesignSubPage page);
        void handleIpadFooterPill(layout::IpadFooterPill pill);
        [[nodiscard]] std::size_t preferredVocoderFxSlotIndex() const;
        [[nodiscard]] std::size_t preferredQuasarFxSlotIndex() const;

        ModAssignmentController modAssignmentController_;
        ObsidianLookAndFeel lookAndFeel_;
        PatchworkEightProcessor& processor_;
        SharedEditorChrome& chrome_;

        EngineNodeStrip nodeSelectorRow_;
        LiveTopologyStrip liveTopologyStrip_;
        juce::Component advancedGridPage_;
        EngineGridPanel engineGridPanel_;
        DashboardStrip dashboardStrip_;
        VstBottomBar vstBottomBar_;
        EngineDetailOverlay engineDetailOverlay_;
        TopologyGraphOverlay topologyGraphOverlay_;
        ContextStrip contextStrip_;
        OscilloscopeView desktopScope_;
        MasterOutputDeck masterOutputDeck_;
        IpadPlayMasterStrip ipadPlayMasterStrip_;
        MasterEnvelopePanel masterEnvelopePanel_;
        BasicPerformanceSidebar basicPerformanceSidebar_;
        PatchFocusPanel patchFocusPanel_;
        CompactModeEditor compactEditor_;
        std::array<juce::TextButton, 6> tabButtons_{
            juce::TextButton{"OSC"},
            juce::TextButton{"FILTER"},
            juce::TextButton{"ENV"},
            juce::TextButton{"MOD"},
            juce::TextButton{"FX"},
            juce::TextButton{"GLOBAL"},
        };
        juce::TextButton boardTabButton_{"BOARD"};
        juce::TextButton motionTabButton_{"MOTION"};
        AdvancedSubview advancedSubview_ = AdvancedSubview::Board;
        layout::PlayLabOverlay playLabOverlay_ = layout::PlayLabOverlay::None;
        layout::PlayViewMode viewMode_ = layout::PlayViewMode::Desktop;
        bool settingPlayViewMode_ = false;
        bool playViewLayoutApplied_ = false;
        Page currentPage_ = Page::Filter;
        juce::Component oscPage_;
        juce::Component filterPage_;
        juce::Component envPage_;
        juce::Component modPage_;
        juce::Component fxPage_;
        juce::Component globalPage_;

        juce::Label modAssignmentBanner_;
        SectionPanel oscPanel_{"Operator"};
        OperatorEditorPanel operatorEditorPanel_;
        FilterLfoPanel filterLfoPanel_;
        AmpEnvelopePanel ampEnvelopePanel_;
        ModLauncherPanel modLauncherPanel_;
        FxChainStrip fxChainStrip_;
        GlobalPanel globalPanel_;
        ModRoutingOverlay modRoutingOverlay_;
        ArpPanelOverlay arpPanelOverlay_;
        VocoderLabPanel vocoderLabPanel_;
        DualLfoLabPanel dualLfoLabPanel_;
        MasterMotionLabPanel masterMotionLabPanel_;
        MasterQuasarPanel masterQuasarPanel_;
        WavetableLabPanel wavetableLabPanel_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayModeEditor)
    };

} // namespace pw8::plugin::ui
