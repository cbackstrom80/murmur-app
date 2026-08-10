#include "FxChainStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        const char* effectTypeName(int typeOrdinal) noexcept
        {
            switch (typeOrdinal)
            {
                case 0: return "BYPASS";
                case 1: return "SATURATION";
                case 2: return "CHORUS";
                case 3: return "TAPE DELAY";
                case 4: return "NODE DELAY";
                case 5: return "FREQ ECHO";
                case 6: return "FRACTAL ECHO";
                case 7: return "REVERB";
                case 8: return "EQ";
                case 9: return "COMPRESSOR";
                case 10: return "LIMITER";
                default: return "?";
            }
        }
    } // namespace

    FxChainStrip::FxChainStrip(juce::AudioProcessorValueTreeState& apvts)
    {
        addAndMakeVisible(panel_);
        for (std::size_t i = 0; i < slots_.size(); ++i)
        {
            auto& slot = slots_[i];
            const bool isMaster = i >= 3;
            const std::size_t localIndex = isMaster ? i - 3 : i;
            const juce::String prefix = isMaster ? masterFxParamId(localIndex, "") : insertFxParamId(localIndex, "");

            slot.typeParam = apvts.getRawParameterValue(prefix + "Type");
            slot.typeLabel.setJustificationType(juce::Justification::centred);
            slot.typeLabel.setFont(fonts::label(9.5f));
            slot.typeLabel.setColour(juce::Label::textColourId, palette::kTextSecondary);
            panel_.addAndMakeVisible(slot.typeLabel);

            slot.mixKnob = std::make_unique<GlowKnob>(apvts, prefix + "Mix", isMaster ? ("M" + juce::String(localIndex + 1))
                                                                                       : ("I" + juce::String(localIndex + 1)));
            panel_.addAndMakeVisible(*slot.mixKnob);
        }
        startTimerHz(6); // Type rarely changes; a slow poll is plenty.
        timerCallback();
    }

    FxChainStrip::~FxChainStrip()
    {
        stopTimer();
    }

    void FxChainStrip::timerCallback()
    {
        for (auto& slot : slots_)
        {
            if (slot.typeParam == nullptr)
                continue;
            const auto name = juce::String(effectTypeName(static_cast<int>(slot.typeParam->load())));
            if (slot.typeLabel.getText() != name)
                slot.typeLabel.setText(name, juce::dontSendNotification);
        }
    }

    void FxChainStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto bounds = panel_.getContentBounds();
        const int slotWidth = bounds.getWidth() / static_cast<int>(slots_.size());
        for (auto& slot : slots_)
        {
            auto slotBounds = bounds.removeFromLeft(slotWidth).reduced(3);
            slot.typeLabel.setBounds(slotBounds.removeFromTop(24));
            slot.mixKnob->setBounds(slotBounds);
        }
    }

} // namespace pw8::plugin::ui
