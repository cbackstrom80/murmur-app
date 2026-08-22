#pragma once

#include <memory>
#include <vector>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "FathomProcessor.h"
#include "ui/theme/ObsidianLookAndFeel.h"
#include "ui/components/SectionPanel.h"
#include "ui/components/GlowRingButton.h"

namespace pw8::fathom
{
    /// Fathom's real dedicated reverb UI -- Phase 1 scope (see the approved
    /// plan): every one of the real 15 algorithmic reverb params gets a
    /// real on-screen knob (today's generic MURMUR/Undertow FX-slot UI only
    /// ever shows 4-6 of them), plus a real IR browser for the new
    /// convolution engine. Built from plain juce::Slider/ComboBox styled by
    /// the real, already-shared ObsidianLookAndFeel -- not GlowKnob (its
    /// header pulls in MurmurProcessor.h, a real instrument-only dependency
    /// Fathom has no reason to carry -- see fathom_plugin/CMakeLists.txt's
    /// own comment). A real bespoke visual identity (own launch screen,
    /// custom knob art, etc.) is real follow-on design work, not this pass.
    class FathomEditor : public juce::AudioProcessorEditor
    {
    public:
        explicit FathomEditor(FathomProcessor& processor);
        ~FathomEditor() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        struct KnobControl
        {
            std::unique_ptr<juce::Slider> slider;
            std::unique_ptr<juce::Label> label;
            std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
        };

        [[nodiscard]] std::unique_ptr<KnobControl> makeKnob(const juce::String& paramId, const juce::String& labelText);
        void layoutKnobGrid(juce::Rectangle<int> area, const std::vector<KnobControl*>& knobs, int cols);
        void updateModeVisibility();

        FathomProcessor& processor_;
        plugin::ui::ObsidianLookAndFeel laf_;

        juce::Label titleLabel_;
        juce::Label subtitleLabel_;

        // Single real toggle over the one real discrete `reverbMode` param
        // (0=Algorithmic, 1=Convolution) -- pressed means Convolution.
        std::unique_ptr<plugin::ui::GlowRingButton> modeToggle_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> modeToggleAttachment_;
        juce::Label modeLabel_;

        plugin::ui::SectionPanel algoPanel_{"Algorithmic"};
        std::vector<std::unique_ptr<KnobControl>> algoKnobs_;
        std::unique_ptr<juce::ComboBox> characterBox_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> characterAttachment_;
        juce::Label characterLabel_;

        plugin::ui::SectionPanel convPanel_{"Convolution"};
        std::vector<std::unique_ptr<KnobControl>> convKnobs_;
        std::unique_ptr<juce::ComboBox> irBox_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> irAttachment_;
        juce::Label irLabel_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FathomEditor)
    };

} // namespace pw8::fathom
