#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "ModAssignmentController.h"
#include "EngineNodeStrip.h"
#include "WavetableStackView.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// DESIGN Wavetable tab — stack preview, wavetable assign/browse, and full warp
    /// controls (Bend, Asym, Sync Ratio, Sync Amt, Formant). Sync ratio lives here
    /// per docs/adr/play-warp-knobs.md (PLAY exposes Bend/Asym/Sync Amt/Formant only).
    class WavetableWarpPanel : public juce::Component, private juce::Timer
    {
    public:
        explicit WavetableWarpPanel(PatchworkEightProcessor& processor);

        void resized() override;
        void refreshFromPatch();

    private:
        void showNode(int nodeIndex);
        void rebuildKnobsForNode();
        void wireModTargets();
        void updateEngineVisibility();
        [[nodiscard]] int currentEngineOrdinal() const noexcept;
        void timerCallback() override;

        PatchworkEightProcessor& processor_;
        ModAssignmentController assignmentController_;
        EngineNodeStrip nodeSelector_;
        WavetableStackView stackView_;
        juce::Label engineHint_{"", ""};

        int selectedNode_ = 0;
        int lastKnownEngine_ = -1;

        std::unique_ptr<GlowKnob> wavetablePosKnob_;
        std::unique_ptr<GlowKnob> wtBendKnob_;
        std::unique_ptr<GlowKnob> wtAsymmetryKnob_;
        std::unique_ptr<GlowKnob> wtSyncRatioKnob_;
        std::unique_ptr<GlowKnob> wtSyncAmountKnob_;
        std::unique_ptr<GlowKnob> wtFormantKnob_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableWarpPanel)
    };

} // namespace pw8::plugin::ui
