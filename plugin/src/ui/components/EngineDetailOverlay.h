#pragma once

#include <array>
#include <functional>
#include <memory>

#include <juce_gui_basics/juce_gui_basics.h>

#include "EngineAdsrMini.h"
#include "FilterLfoPanel.h"
#include "GlowKnob.h"
#include "ModAssignmentController.h"
#include "OperatorEditorPanel.h"
#include "processor/MurmurProcessor.h"

namespace pw8::plugin::ui
{
    /// Full engine deep editor — Figma `murmur-engine-deep-editor` (28:4), letterboxed 1440×1024 → embed.
    class EngineDetailOverlay : public juce::Component, private juce::Timer
    {
    public:
        EngineDetailOverlay(MurmurProcessor& processor, ModAssignmentController& modAssignmentController);

        void showForEngine(int engineIndex);
        void dismiss();

        std::function<void()> onClosed;

        void resized() override;
        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        void rebindEngine(int engineIndex);
        void rebuildMixAttachments();
        void rebuildAmpKnobs();
        void layoutDesignSurface();
        void paintSectionHeading(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& kicker,
                                 const juce::String& title) const;
        void paintAmpColumn(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void paintFooter(juce::Graphics& g, juce::Rectangle<int> bounds) const;
        void mouseDown(const juce::MouseEvent& event) override;
        void timerCallback() override;

        MurmurProcessor& processor_;
        int engineIndex_ = 0;

        juce::Component contentHost_;
        juce::Label brandTitle_;
        juce::Label brandSubtitle_;
        std::array<juce::TextButton, 8> engineTabButtons_{};
        juce::TextButton onButton_{"ON"};
        juce::TextButton soloButton_{"SOLO"};
        juce::TextButton muteButton_{"MUTE"};
        juce::TextButton closeButton_{"← BACK"};
        juce::TextButton nextEngineButton_{"GOTO NEXT >"};

        juce::Component oscColumn_;
        juce::Component filterColumn_;
        juce::Component ampColumn_;
        OperatorEditorPanel operatorPanel_;
        FilterLfoPanel filterPanel_;
        std::unique_ptr<EngineAdsrMini> adsrMini_;
        std::unique_ptr<GlowKnob> levelKnob_;
        std::unique_ptr<GlowKnob> panKnob_;
        std::unique_ptr<GlowKnob> unisonDetuneKnob_;
        std::unique_ptr<GlowKnob> unisonSpreadKnob_;

        std::array<juce::Rectangle<int>, 4> mutable unisonVoicePillBounds_{};

        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> soloAttachment_;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteAttachment_;

        juce::Rectangle<int> oscHeadingBounds_;
        juce::Rectangle<int> filterHeadingBounds_;
        juce::Rectangle<int> ampHeadingBounds_;
        juce::Rectangle<int> footerBounds_;
        juce::Rectangle<int> ampPaintBounds_;
        juce::Rectangle<int> ampKnobRowBounds_;
        juce::Rectangle<int> ampRoutingBounds_;
    };

} // namespace pw8::plugin::ui
