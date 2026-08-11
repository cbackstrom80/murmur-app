#include "FilterLfoPanel.h"

#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::String filterModeToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "LOWPASS";
                case 1: return "HIGHPASS";
                case 2: return "BANDPASS";
                case 3: return "NOTCH";
                case 4: return "PEAK";
                default: return juce::String(v);
            }
        }

        juce::String lfoWaveformToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "SINE";
                case 1: return "TRIANGLE";
                case 2: return "SAW";
                case 3: return "SQUARE";
                case 4: return "S & H";
                case 5: return "S.RANDOM";
                default: return juce::String(v);
            }
        }

        juce::String lfoModeToText(float v)
        {
            switch (static_cast<int>(v))
            {
                case 0: return "FREE";
                case 1: return "RETRIGGER";
                case 2: return "ONE SHOT";
                case 3: return "TEMPO SYNC";
                default: return juce::String(v);
            }
        }
    } // namespace

    FilterLfoPanel::FilterLfoPanel(PatchworkEightProcessor& processor)
    {
        auto& apvts = processor.apvts;
        addAndMakeVisible(filterPanel_);
        addAndMakeVisible(lfoPanel_);

        filterEnabledAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
            apvts, juce::String(kFilterIdPrefix) + "Enabled", filterEnabledToggle_);
        filterPanel_.addAndMakeVisible(filterEnabledToggle_);

        filterMode_ = std::make_unique<GlowKnob>(apvts, juce::String(kFilterIdPrefix) + "Mode", "Mode",
                                                   filterModeToText);
        filterCutoff_ = std::make_unique<GlowKnob>(apvts, juce::String(kFilterIdPrefix) + "CutoffHz", "Cutoff");
        filterResonance_ =
            std::make_unique<GlowKnob>(apvts, juce::String(kFilterIdPrefix) + "Resonance", "Resonance");
        filterKeyTrack_ = std::make_unique<GlowKnob>(apvts, juce::String(kFilterIdPrefix) + "KeyTrack", "Key Trk");
        // The two real drag-to-modulate destinations this pass surfaces (docs/UI.md)
        // -- Mode and Key Track aren't ModMatrixExecutor destinations at all, so they
        // deliberately stay plain knobs, not drop targets.
        filterCutoff_->enableModulationTarget(processor, modulation::ModDestination::FilterCutoff);
        filterResonance_->enableModulationTarget(processor, modulation::ModDestination::FilterResonance);
        for (auto* k : {filterMode_.get(), filterCutoff_.get(), filterResonance_.get(), filterKeyTrack_.get()})
            filterPanel_.addAndMakeVisible(*k);

        lfoWaveform_ = std::make_unique<GlowKnob>(apvts, lfoParamId(0, "Waveform"), "Wave", lfoWaveformToText);
        lfoMode_ = std::make_unique<GlowKnob>(apvts, lfoParamId(0, "Mode"), "Mode", lfoModeToText);
        lfoRate_ = std::make_unique<GlowKnob>(apvts, lfoParamId(0, "RateHz"), "Rate");
        for (auto* k : {lfoWaveform_.get(), lfoMode_.get(), lfoRate_.get()})
            lfoPanel_.addAndMakeVisible(*k);
    }

    void FilterLfoPanel::resized()
    {
        auto bounds = getLocalBounds();
        const int half = bounds.getWidth() / 2;
        filterPanel_.setBounds(bounds.removeFromLeft(half).reduced(4, 0));
        lfoPanel_.setBounds(bounds.reduced(4, 0));

        {
            auto content = filterPanel_.getContentBounds();
            filterEnabledToggle_.setBounds(content.removeFromTop(20).removeFromLeft(110));
            const int knobWidth = content.getWidth() / 4;
            filterMode_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            filterCutoff_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            filterResonance_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            filterKeyTrack_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        }
        {
            auto content = lfoPanel_.getContentBounds();
            const int knobWidth = content.getWidth() / 3;
            lfoWaveform_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            lfoMode_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
            lfoRate_->setBounds(content.removeFromLeft(knobWidth).reduced(3));
        }
    }

} // namespace pw8::plugin::ui
