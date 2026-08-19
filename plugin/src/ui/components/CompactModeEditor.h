#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "../PerformanceMetricsUi.h"
#include "../ScopeVuMeter.h"
#include "GlowKnob.h"
#include "OscilloscopeView.h"
#include "PatchFocusPanel.h"
#include "processor/PatchworkEightProcessor.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    /// Compact PLAY — meter/scope surface + three decked mega knobs (master + two patch KOINS).
    /// No patch browse/switch; exit to Desktop PLAY via chrome only.
    class CompactModeEditor : public juce::Component, private juce::Timer
    {
    public:
        explicit CompactModeEditor(PatchworkEightProcessor& processor);

        ~CompactModeEditor() override;

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;

        void refreshFromPatch() { focusPanel_.refreshFromPatch(); }

    private:
        void timerCallback() override;
        void updateMissionCard();

        void paintCardBackground(juce::Graphics& g, juce::Rectangle<float> bounds) const;
        void paintHorizontalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                  const scope::VuBallistics& vu) const;

        PatchworkEightProcessor& processor_;

        OscilloscopeView scopeView_;
        PatchFocusPanel focusPanel_;
        std::unique_ptr<GlowKnob> masterKnob_;

        scope::VuBallistics leftVu_;
        scope::VuBallistics rightVu_;
        PerformanceMetricsSnapshot metrics_{};

        juce::Rectangle<int> scopePanelBounds_;
        juce::Rectangle<int> megaKnobDeckBounds_;
        juce::Rectangle<int> footerBounds_;
        juce::Rectangle<int> masterKnobBounds_;
        juce::Rectangle<int> koinKnobBounds_;
        juce::Rectangle<int> meterStatsBounds_;

        juce::String lastCategory_;
        juce::String lastHint_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompactModeEditor)
    };

} // namespace pw8::plugin::ui
