#include "AmpEnvelopePanel.h"

#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    AmpEnvelopePanel::AmpEnvelopePanel(PatchworkEightProcessor& processor)
        : visualizer_(processor.apvts, 0)
    {
        auto& apvts = processor.apvts;
        addAndMakeVisible(panel_);
        visualizer_.attachVisualizerBus(processor.getVisualizerBus());
        panel_.addAndMakeVisible(visualizer_);

        delay_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Delay"), "Delay");
        attack_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Attack"), "Attack");
        hold_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Hold"), "Hold");
        decay_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Decay"), "Decay");
        sustain_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Sustain"), "Sustain");
        release_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Release"), "Release");
        curve_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Curve"), "Curve");

        for (auto* k :
             {delay_.get(), attack_.get(), hold_.get(), decay_.get(), sustain_.get(), release_.get(), curve_.get()})
        {
            k->applyFigmaContext(figma::KnobContext::PlayBlades);
            panel_.addAndMakeVisible(*k);
        }

        legatoAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, envelopeParamId(0, "Legato"), legato_);
        panel_.addAndMakeVisible(legato_);
    }

    void AmpEnvelopePanel::paint(juce::Graphics& g)
    {
        auto banner = getLocalBounds().removeFromTop(28).toFloat().reduced(0.0f, 2.0f);
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(banner, 6.0f);
        g.setColour(palette::kBorderBright);
        g.drawRoundedRectangle(banner.reduced(0.5f), 6.0f, 1.0f);

        const auto badge = banner.removeFromLeft(72.0f).reduced(6.0f, 5.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.35f));
        g.fillRoundedRectangle(badge, 4.0f);
        g.setColour(palette::kAccentWarm);
        g.setFont(fonts::label(9.5f));
        g.drawText("LAYER", badge, juce::Justification::centred);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        g.drawText("LAYER A · amp envelope (env0) · affects all voices on this layer",
                   banner.reduced(8.0f, 0.0f), juce::Justification::centredLeft);
    }

    void AmpEnvelopePanel::resized()
    {
        panel_.setBounds(getLocalBounds().withTrimmedTop(34));
        auto content = panel_.getContentBounds();

        auto visualizerBounds = content.removeFromLeft(static_cast<int>(content.getWidth() * 0.60f)).reduced(0, 2);
        visualizer_.setBounds(visualizerBounds);

        content = content.reduced(6, 0);
        legato_.setBounds(content.removeFromTop(22).removeFromLeft(90));
        content.removeFromTop(6);

        const int knobWidth = content.getWidth() / 4;
        auto row1 = content.removeFromTop(120);
        delay_->setBounds(row1.removeFromLeft(knobWidth).reduced(4));
        attack_->setBounds(row1.removeFromLeft(knobWidth).reduced(4));
        hold_->setBounds(row1.removeFromLeft(knobWidth).reduced(4));
        decay_->setBounds(row1.reduced(4));

        auto row2 = content.removeFromTop(120);
        sustain_->setBounds(row2.removeFromLeft(knobWidth).reduced(4));
        release_->setBounds(row2.removeFromLeft(knobWidth).reduced(4));
        curve_->setBounds(row2.removeFromLeft(knobWidth).reduced(4));
    }

} // namespace pw8::plugin::ui
