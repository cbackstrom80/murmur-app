#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "PlayModeLayout.h"
#include "SharedEditorChrome.h"
#include "components/ArpPanelOverlay.h"
#include "components/DesignFxPanel.h"
#include "components/DualLfoLabPanel.h"
#include "components/EngineDetailOverlay.h"
#include "components/EngineGridPanel.h"
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

    private:
        void openEngineDetail(int engineIndex);
        void closeEngineDetail();
        void applySubPageVisibility();

        ModAssignmentController modAssignmentController_;
        ObsidianLookAndFeel lookAndFeel_;
        layout::DesignSubPage designSubPage_ = layout::DesignSubPage::Engine;
        int wavetableEngineIndex_ = 0;

        juce::Component enginePage_;
        EngineGridPanel engineGridPanel_;
        VstBottomBar statusBar_;
        EngineDetailOverlay engineDetailOverlay_;

        ArpPanelOverlay arpPanel_;
        VocoderLabPanel vocoderPanel_;
        DesignFxPanel fxPanel_;
        DesignModMatrixPanel modMatrixPanel_;
        WavetableLabPanel wavetablePanel_;
        DualLfoLabPanel dualLfoPanel_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignModeEditor)
    };

} // namespace pw8::plugin::ui
