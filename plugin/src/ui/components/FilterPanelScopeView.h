#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "HeaderSpectrumScope.h"
#include "OscilloscopeView.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    enum class FilterScopeDisplayMode
    {
        Waveform,
        Spectrum,
    };

    /// FILTER tab scope: toggles live waveform vs log-frequency spectrum (Horizon 2).
    class FilterPanelScopeView : public juce::Component
    {
    public:
        explicit FilterPanelScopeView(MurmurProcessor& processor);

        void setDisplayMode(FilterScopeDisplayMode mode);
        [[nodiscard]] FilterScopeDisplayMode getDisplayMode() const noexcept { return mode_; }

        void resized() override;
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        [[nodiscard]] juce::Rectangle<float> toggleBounds() const;
        [[nodiscard]] int toggleSegmentAt(juce::Point<float> pos) const;

        MurmurProcessor& processor_;
        FilterScopeDisplayMode mode_ = FilterScopeDisplayMode::Waveform;
        OscilloscopeView oscilloscope_;
        HeaderSpectrumScope spectrum_;
    };

} // namespace pw8::plugin::ui
