#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "components/AlgorithmGraphEditor.h"
#include "components/SectionPanel.h"
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
        std::array<juce::TextButton, 4> tabButtons_{
            juce::TextButton{"Graph"},
            juce::TextButton{"Matrix"},
            juce::TextButton{"FX"},
            juce::TextButton{"Wavetable"},
        };
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
        juce::Label matrixPlaceholder_{"", "Full mod matrix — Week 4"};
        juce::Label fxPlaceholder_{"", "FX detail panels — Week 5"};
        juce::Label wavetablePlaceholder_{"", "Wavetable warp panel — Week 7"};
        juce::TextButton openBuilderButton_{"Open Builder…"};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DesignModeEditor)
    };

} // namespace pw8::plugin::ui
