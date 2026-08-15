#pragma once

#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "QuasarProcessor.h"
#include "ui/theme/ObsidianLookAndFeel.h"

namespace pw8::quasar::ui
{
    class GlowKnob;
    class MetadataFacetRow;
}

namespace pw8::quasar
{
    class QuasarEditor : public juce::AudioProcessorEditor, private juce::Timer
    {
    public:
        explicit QuasarEditor(QuasarProcessor& processor);
        ~QuasarEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void rebuildKnobs();
        void layoutKnobGrid(juce::Rectangle<int> area, const std::vector<juce::Component*>& knobs, int cols);

        QuasarProcessor& processor_;
        ui::ObsidianLookAndFeel laf_;

        juce::Label titleLabel_;
        juce::Label helpLabel_;
        juce::Label scopeLabel_;
        juce::Label sidechainLabel_;

        std::unique_ptr<ui::MetadataFacetRow> delaySyncRow_;
        std::unique_ptr<ui::MetadataFacetRow> delayDivisionRow_;

        std::vector<std::unique_ptr<ui::GlowKnob>> knobs_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasarEditor)
    };

} // namespace pw8::quasar
