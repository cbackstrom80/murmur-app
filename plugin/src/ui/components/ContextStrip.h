#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "FilterLfoPanel.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// One-line scope label: "ENGINE 3 · WAVETABLE" or "LAYER A · GLOBAL".
    class ContextStrip : public juce::Component
    {
    public:
        explicit ContextStrip(MurmurProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;

        void setScope(FilterPanelScope scope, int engineIndex = 0);

    private:
        MurmurProcessor& processor_;
        FilterPanelScope scope_ = FilterPanelScope::Engine;
        int engineIndex_ = 0;
    };

} // namespace pw8::plugin::ui
