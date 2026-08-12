#pragma once

#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "SectionPanel.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    class AmpEnvelopePanel : public juce::Component
    {
    public:
        explicit AmpEnvelopePanel(PatchworkEightProcessor& processor);

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        SectionPanel panel_{"Layer Amp Envelope"};
        std::unique_ptr<GlowKnob> delay_;
        std::unique_ptr<GlowKnob> attack_;
        std::unique_ptr<GlowKnob> hold_;
        std::unique_ptr<GlowKnob> decay_;
        std::unique_ptr<GlowKnob> sustain_;
        std::unique_ptr<GlowKnob> release_;
        std::unique_ptr<GlowKnob> curve_;
        juce::ToggleButton legato_{"LEGATO"};
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> legatoAttachment_;
    };

} // namespace pw8::plugin::ui
