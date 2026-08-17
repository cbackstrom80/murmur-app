#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "FilterLfoPanel.h"
#include "FxChainStrip.h"
#include "ModAssignmentController.h"
#include "processor/PatchworkEightProcessor.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Bottom dashboard: shared FX rack + global filter summary (PLAY board strip).
    class DashboardStrip : public juce::Component, private juce::Timer
    {
    public:
        DashboardStrip(PatchworkEightProcessor& processor, ModAssignmentController& modAssignmentController);

        void paint(juce::Graphics& g) override;
        void resized() override;

        std::function<void(std::size_t fxSlotIndex)> onVocoderLabRequested;
        std::function<void()> onLfoLabRequested;

    private:
        void timerCallback() override;
        void paintLiveMeter(juce::Graphics& g, juce::Rectangle<int> bounds, const char* label,
                              const scope::VuBallistics& vu) const;
        void updateMeterLayout();

        PatchworkEightProcessor& processor_;
        FxChainStrip fxChainStrip_;
        FilterLfoPanel filterLfoPanel_;

        scope::VuBallistics fxOutVu_;
        scope::VuBallistics filterBusVu_;
        std::array<float, 256> scopeScratch_{};
        juce::Rectangle<int> fxOutMeterBounds_;
        juce::Rectangle<int> filterOutMeterBounds_;
    };

} // namespace pw8::plugin::ui
