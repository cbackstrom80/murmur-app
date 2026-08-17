#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "EngineAdsrMini.h"
#include "EngineOscillatorPicker.h"
#include "GlowKnob.h"
#include "processor/PatchworkEightProcessor.h"
#include "ui/ScopeVuMeter.h"

namespace pw8::plugin::ui
{
    /// Single engine tile — locked to Figma `murmur-8-engine-vst` card layout (4:38).
    class EngineCard : public juce::Component, private juce::Timer
    {
    public:
        EngineCard(PatchworkEightProcessor& processor, int engineIndex);
        ~EngineCard() override;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDoubleClick(const juce::MouseEvent& event) override;

        /// Figma play-board stub layout (22:44) for the 4×2 grid; full card (4:38) stays in detail overlay.
        void setPlayBoardCompactMode(bool compact);

        /// Figma `murmur-design-engine` engine-card (37:832) — type strip + sub-picker + context + knobs + envelope.
        void setDesignModeV2Layout(bool designMode);

        std::function<void(int engineIndex)> onDoubleClicked;
        std::function<void(int engineIndex)> onWavetableLabRequested;

    private:
        void timerCallback() override;
        void setFilterMode(int modeOrdinal);
        void refreshMixButtonStates();
        void refreshLevelLabel();
        void refreshEngineTypeBadge();
        void applyPlayBoardCompactVisibility();
        void applyDesignModeV2Visibility();
        void refreshDesignModeV2ControlGroups();
        void paintLevelRow(juce::Graphics& g, juce::Rectangle<int> rowBounds);
        void paintPlayBoardKnobStubs(juce::Graphics& g) const;

        PatchworkEightProcessor& processor_;
        const int engineIndex_;

        juce::Label titleLabel_;
        juce::Label engineTypeBadge_;
        juce::TextButton onButton_{"ON"};
        juce::TextButton soloButton_{"S"};
        juce::TextButton muteButton_{"M"};

        EngineOscillatorPicker oscillatorPicker_;
        EngineAdsrMini adsrMini_;

        std::unique_ptr<GlowKnob> coarseKnob_;
        std::unique_ptr<GlowKnob> fineKnob_;
        std::unique_ptr<GlowKnob> cutoffKnob_;
        std::unique_ptr<GlowKnob> resKnob_;

        juce::Label levelCaption_{"LVL", "LVL"};
        juce::Slider levelSlider_;
        juce::Label levelValueLabel_;

        std::array<juce::TextButton, 4> filterModeButtons_{
            juce::TextButton{"LP"},
            juce::TextButton{"HP"},
            juce::TextButton{"BP"},
            juce::TextButton{"N"},
        };

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> levelAttachment_;

        juce::Rectangle<int> levelSliderBounds_;
        std::array<juce::Rectangle<int>, 2> playBoardKnobStubBounds_{};
        bool ledActive_ = true;
        bool playBoardCompactMode_ = false;
        bool designModeV2Layout_ = false;
        bool designShowPitchKnobs_ = true;
        bool designShowFilterKnobs_ = true;
        float ledPulsePhase_ = 0.0f;
        float livePeakNorm_ = 0.0f;
        scope::VuBallistics livePeakVu_;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineCard)
    };

} // namespace pw8::plugin::ui
