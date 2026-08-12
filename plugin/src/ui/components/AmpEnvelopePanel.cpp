#include "AmpEnvelopePanel.h"

#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    AmpEnvelopePanel::AmpEnvelopePanel(PatchworkEightProcessor& processor)
    {
        auto& apvts = processor.apvts;
        addAndMakeVisible(panel_);

        delay_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Delay"), "Delay");
        attack_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Attack"), "Attack");
        hold_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Hold"), "Hold");
        decay_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Decay"), "Decay");
        sustain_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Sustain"), "Sustain");
        release_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Release"), "Release");
        curve_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Curve"), "Curve");

        for (auto* k :
             {delay_.get(), attack_.get(), hold_.get(), decay_.get(), sustain_.get(), release_.get(), curve_.get()})
            panel_.addAndMakeVisible(*k);

        legatoAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, envelopeParamId(0, "Legato"), legato_);
        panel_.addAndMakeVisible(legato_);
    }

    void AmpEnvelopePanel::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds();
        legato_.setBounds(content.removeFromTop(22).removeFromLeft(90));
        content.removeFromTop(6);

        const int knobWidth = content.getWidth() / 4;
        auto row1 = content.removeFromTop(110);
        delay_->setBounds(row1.removeFromLeft(knobWidth));
        attack_->setBounds(row1.removeFromLeft(knobWidth));
        hold_->setBounds(row1.removeFromLeft(knobWidth));
        decay_->setBounds(row1);

        auto row2 = content.removeFromTop(110);
        sustain_->setBounds(row2.removeFromLeft(knobWidth));
        release_->setBounds(row2.removeFromLeft(knobWidth));
        curve_->setBounds(row2.removeFromLeft(knobWidth));
    }

} // namespace pw8::plugin::ui
