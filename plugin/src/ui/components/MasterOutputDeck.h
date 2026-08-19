#pragma once

#include <array>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "GlowKnob.h"
#include "processor/MurmurProcessor.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-mi-ui-play-master-dynamics` (`89:1798`) — master output + Streams modes.
    class MasterOutputDeck : public juce::Component, private juce::Timer
    {
    public:
        explicit MasterOutputDeck(MurmurProcessor& processor);
        ~MasterOutputDeck() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void timerCallback() override;
        void paintVerticalMeter(juce::Graphics& g, juce::Rectangle<float> bounds, const scope::VuBallistics& vu,
                                const char* label) const;
        void paintGainReductionMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float grDb) const;
        void paintSidechainViz(juce::Graphics& g, juce::Rectangle<float> bounds) const;
        void styleModePill(juce::TextButton& btn, bool active);
        void refreshModePills();
        void setDynamicsMode(int modeIndex);

        MurmurProcessor& processor_;
        std::unique_ptr<GlowKnob> masterKnob_;
        std::array<juce::TextButton, 4> modePills_{};
        juce::TextButton enableButton_{"STREAMS"};
        scope::VuBallistics leftVu_;
        scope::VuBallistics rightVu_;
        float grMeterDb_ = 0.0f;
        float sidechainEnvelope_ = 0.0f;
        std::array<float, 64> sidechainHistory_{};
        std::size_t sidechainHistoryWrite_ = 0;
        std::array<float, 512> scopeScratch_{};
        juce::Rectangle<int> leftMeterBounds_;
        juce::Rectangle<int> rightMeterBounds_;
        juce::Rectangle<int> grMeterBounds_;
        juce::Rectangle<int> sidechainVizBounds_;
        juce::Rectangle<int> modePillRowBounds_;

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> enabledAttachment_;
    };

} // namespace pw8::plugin::ui
