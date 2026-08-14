#pragma once

#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Consolidated mod matrix + modulators screen — shared by PLAY MOD tab and DESIGN Matrix tab.
    class ModMatrixScreen : public juce::Component, private juce::Timer
    {
    public:
        explicit ModMatrixScreen(PatchworkEightProcessor& processor);
        ~ModMatrixScreen() override;

        void refreshFromPatch();
        void resized() override;
        void paint(juce::Graphics& g) override;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        PatchworkEightProcessor& processor_;

        void timerCallback() override;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixScreen)
    };

} // namespace pw8::plugin::ui
