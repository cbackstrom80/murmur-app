#pragma once

#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "ModRoutingUi.h"
#include "ModSourceChip.h"
#include "SectionPanel.h"
#include "../theme/ObsidianPalette.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Per-patch "Knobs of Interest" — patch-authored via `uiFocus` in .pw8, or inferred
    /// from named macros, macro routes, and active mod destinations.
    class PatchFocusPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit PatchFocusPanel(PatchworkEightProcessor& processor);

        ~PatchFocusPanel() override;

        void resized() override;

        /// BASIC view: larger knobs, hide mod-matrix entry. Advanced entry lives in the view-mode toggle.
        void setBasicPerformanceLayout(bool basicLayout);

        /// Compact 320px column: smaller knobs, tighter grid, max 4 focus controls.
        void setCompactLayout(bool compactLayout);

        /// Force rebuild from the processor's current patch (e.g. immediately after preset load).
        void refreshFromPatch();

        std::function<void()> onAdvancedClicked;

    private:
        void timerCallback() override;
        void rebuildKnobs(const std::vector<PatchFocusKnobSpec>& specs);
        void applyLayoutMode();

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Knobs of Interest", palette::kAccentWarm, true};
        juce::Label introLabel_;
        juce::Label subtitleLabel_;
        juce::Label modWheelBadge_;
        juce::Label expressionBadge_;
        juce::TextButton advancedButton_{"Mod Matrix (M)"};
        bool basicLayout_ = true;
        bool compactLayout_ = false;
        std::vector<PatchFocusKnobSpec> lastSpecs_;
        std::vector<std::unique_ptr<GlowKnob>> knobs_;
    };

} // namespace pw8::plugin::ui
