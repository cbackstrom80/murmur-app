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
    /// Basic/Compact PLAY performance surface: 1–3 feature macro KOINS plus consistent standard APVTS knobs.
    class PatchFocusPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit PatchFocusPanel(PatchworkEightProcessor& processor);

        ~PatchFocusPanel() override;

        void resized() override;

        /// BASIC view: larger knobs, hide mod-matrix entry. Advanced entry lives in the view-mode toggle.
        void setBasicPerformanceLayout(bool basicLayout);

        /// Compact 320px column: smaller knobs, tighter grid, up to 3 feature macro KOINS.
        void setCompactLayout(bool compactLayout);

        /// Teleprompter: orbit up to 3 feature macro KOINS around `centerHole`.
        void setOrbitHole(juce::Rectangle<int> centerHole);

        /// Force rebuild from the processor's current patch (e.g. immediately after preset load).
        void refreshFromPatch();

        std::function<void()> onAdvancedClicked;
        std::function<void(int operatorHint)> onPerformanceActivity;

    private:
        void timerCallback() override;
        void paint(juce::Graphics& g) override;
        void rebuildKnobs(const PatchFocusLayout& layout);
        void applyLayoutMode();
        void updateBadgePulse();
        void layoutKnobGrid(juce::Rectangle<int> bounds, std::size_t startIndex, std::size_t count, int minCellWidth);

        PatchworkEightProcessor& processor_;
        SectionPanel panel_{"Performance Controls", palette::kAccentWarm, true};
        juce::Label introLabel_;
        juce::Label subtitleLabel_;
        juce::Label macroHintsLabel_;
        juce::Label standardSectionLabel_;
        juce::Label modWheelBadge_;
        juce::Label expressionBadge_;
        juce::Label sidechainBadge_;
        juce::TextButton advancedButton_{"Mod Matrix (M)"};
        bool basicLayout_ = true;
        bool compactLayout_ = false;
        juce::Rectangle<int> orbitHole_;
        PatchFocusLayout lastLayout_;
        std::size_t featureKnobCount_ = 0;
        std::vector<std::unique_ptr<GlowKnob>> knobs_;
        float lastModWheel_ = -1.0f;
        float lastExpression_ = -1.0f;
        float badgePulsePhase_ = 0.0f;
    };

} // namespace pw8::plugin::ui
