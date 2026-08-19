#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PlayModeLayout.h"
#include "SharedEditorChrome.h"
#include "components/ArpPanelOverlay.h"
#include "components/MasterQuasarPanel.h"
#include "components/DesignFxPanel.h"
#include "components/DesignDynamicsLabPanel.h"
#include "components/DesignGenerativeLabPanel.h"
#include "components/DesignUtilityPeaksPanel.h"
#include "components/DesignEnvelopeSegmentsPanel.h"
#include "components/DesignFilterLabPanel.h"
#include "components/DesignMorphEditorPanel.h"
#include "components/DualLfoLabPanel.h"
#include "components/EngineDetailOverlay.h"
#include "components/EngineGridPanel.h"
#include "components/MasterEnvelopePanel.h"
#include "components/ModAssignmentController.h"
#include "components/DesignModMatrixPanel.h"
#include "components/VocoderLabPanel.h"
#include "components/VstBottomBar.h"
#include "components/WavetableLabPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-design-engine` (37:787) — design sub-nav pages share one content shell.
    class DesignModeEditor : public juce::Component
    {
    public:
        DesignModeEditor(PatchworkEightProcessor& processor, SharedEditorChrome& chrome);
        ~DesignModeEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void refreshFromPatch();
        void setDesignSubPage(layout::DesignSubPage page);
        [[nodiscard]] layout::DesignSubPage getDesignSubPage() const noexcept { return designSubPage_; }

        std::function<void(layout::DesignSubPage)> onDesignSubPageChanged;
        std::function<void()> onOpenPlayFilterRequested;

    private:
        void openEngineDetail(int engineIndex);
        void closeEngineDetail();
        void closeMasterQuasarLab();
        void applySubPageVisibility();

        ModAssignmentController modAssignmentController_;
        ObsidianLookAndFeel lookAndFeel_;
        layout::DesignSubPage designSubPage_ = layout::DesignSubPage::Engine;
        int wavetableEngineIndex_ = 0;

        juce::Component enginePage_;
        MasterEnvelopePanel masterEnvelopePanel_;
        EngineGridPanel engineGridPanel_;
        VstBottomBar statusBar_;
        EngineDetailOverlay engineDetailOverlay_;

        ArpPanelOverlay arpPanel_;
        VocoderLabPanel vocoderPanel_;
        DesignFxPanel fxPanel_;
        MasterQuasarPanel masterQuasarPanel_;
        DesignModMatrixPanel modMatrixPanel_;
        WavetableLabPanel wavetablePanel_;
        DualLfoLabPanel dualLfoPanel_;
        DesignMorphEditorPanel morphPanel_;
        DesignFilterLabPanel filterLabPanel_;
        DesignDynamicsLabPanel dynamicsLabPanel_;
        DesignGenerativeLabPanel generativeLabPanel_;
        DesignUtilityPeaksPanel utilityPeaksPanel_;
        DesignEnvelopeSegmentsPanel envelopeSegmentsPanel_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignModeEditor)
    };

} // namespace pw8::plugin::ui
