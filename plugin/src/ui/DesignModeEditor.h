#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "components/DesignTabIconGrid.h"
#include "components/AlgorithmGraphEditor.h"
#include "components/DesignFxDetailPanel.h"
#include "components/ModMatrixDesignPanel.h"
#include "components/SectionPanel.h"
#include "components/WavetableWarpPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "theme/ObsidianLookAndFeel.h"

namespace pw8::plugin::ui
{
    /// DESIGN mode shell — Graph / Matrix / FX / Wavetable tabs.
    class DesignModeEditor : public juce::Component
    {
    public:
        explicit DesignModeEditor(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;

        void refreshFromPatch();

        /// Fired after DESIGN graph Apply succeeds — wire to refresh PLAY graph display.
        std::function<void()> onGraphApplied;

    private:
        enum class Page
        {
            Graph = 0,
            Matrix,
            Fx,
            Wavetable,
        };

        void showPage(Page page);

        PatchworkEightProcessor& processor_;
        ObsidianLookAndFeel lookAndFeel_;
        std::array<std::unique_ptr<juce::TextButton>, 4> tabButtons_;
        Page currentPage_ = Page::Graph;
        juce::Component graphPage_;
        juce::Component matrixPage_;
        juce::Component fxPage_;
        juce::Component wavetablePage_;
        SectionPanel graphPanel_{"Algorithm Graph"};
        SectionPanel matrixPanel_{"Mod Matrix"};
        SectionPanel fxPanel_{"FX Detail"};
        SectionPanel wavetablePanel_{"Wavetable"};
        std::unique_ptr<AlgorithmGraphEditor> graphEditor_;
        std::unique_ptr<ModMatrixDesignPanel> matrixEditor_;
        std::unique_ptr<DesignFxDetailPanel> fxEditor_;
        std::unique_ptr<WavetableWarpPanel> wavetableEditor_;
        juce::TextButton openBuilderButton_{"Open Builder…"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignModeEditor)
    };

} // namespace pw8::plugin::ui
