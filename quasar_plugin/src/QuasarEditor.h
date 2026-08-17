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
    class QuasarSpatialWireframeView;
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
        void rebuildMacroKnobs();
        void rebuildPlayKnobs();
        void rebuildDeepKnobs();
        void layoutKnobGrid(juce::Rectangle<int> area, const std::vector<juce::Component*>& knobs, int cols);
        void toggleDeepPanel();

        QuasarProcessor& processor_;
        ui::ObsidianLookAndFeel laf_;

        juce::Label titleLabel_;
        juce::Label helpLabel_;
        juce::Label playLegendLabel_;
        juce::Label sidechainLabel_;

        std::unique_ptr<ui::QuasarSpatialWireframeView> spatialScope_;
        std::vector<std::unique_ptr<ui::GlowKnob>> macroKnobs_;
        std::vector<std::unique_ptr<ui::GlowKnob>> playKnobs_;
        std::vector<std::unique_ptr<ui::GlowKnob>> deepKnobs_;

        juce::TextButton deepToggle_{"DEEP ▾"};
        std::unique_ptr<ui::MetadataFacetRow> sidechainModeRow_;
        std::unique_ptr<ui::MetadataFacetRow> delaySyncRow_;
        std::unique_ptr<ui::MetadataFacetRow> delayDivisionRow_;

        bool deepPanelOpen_ = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasarEditor)
    };

} // namespace pw8::quasar
