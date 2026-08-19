#include "IpadPlayMasterStrip.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::String formatPortamento(float seconds)
        {
            if (seconds <= 0.0f)
                return "OFF";
            if (seconds < 1.0f)
                return juce::String(juce::roundToInt(seconds * 1000.0f)) + "ms";
            return juce::String(seconds, 2) + "s";
        }

        [[nodiscard]] juce::String formatCutoffHz(float hz)
        {
            if (hz >= 1000.0f)
                return juce::String(hz / 1000.0f, 1) + "k";
            return juce::String(juce::roundToInt(hz));
        }

        [[nodiscard]] juce::String formatResonance(float value)
        {
            return juce::String(juce::roundToInt(value * 100.0f)) + "%";
        }
    } // namespace

    IpadPlayMasterStrip::IpadPlayMasterStrip(PatchworkEightProcessor& processor)
        : processor_(processor), visualizer_(processor.apvts, 0)
    {
        auto& apvts = processor.apvts;
        visualizer_.attachVisualizerBus(processor.getVisualizerBus());

        attackKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Attack"), "A");
        decayKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Decay"), "D");
        sustainKnob_ = std::make_unique<GlowKnob>(
            apvts, envelopeParamId(0, "Sustain"), "S",
            [](float value) { return juce::String(juce::roundToInt(value * 100.0f)) + "%"; });
        releaseKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Release"), "R");
        portamentoKnob_ =
            std::make_unique<GlowKnob>(apvts, kPortamentoId, "GLIDE", [](float value) { return formatPortamento(value); });
        cutoffKnob_ = std::make_unique<GlowKnob>(
            apvts, juce::String(kFilterIdPrefix) + "CutoffHz", "CUTOFF",
            [](float value) { return formatCutoffHz(value); });
        resonanceKnob_ = std::make_unique<GlowKnob>(
            apvts, juce::String(kFilterIdPrefix) + "Resonance", "LPF",
            [](float value) { return formatResonance(value); });

        for (auto* knob : {attackKnob_.get(), decayKnob_.get(), sustainKnob_.get(), releaseKnob_.get(),
                           portamentoKnob_.get(), cutoffKnob_.get(), resonanceKnob_.get()})
        {
            knob->setHeaderCompactMode(true);
            knob->applyFigmaContext(figma::KnobContext::PanelGridMedium);
            addAndMakeVisible(*knob);
        }

        addAndMakeVisible(visualizer_);
        startTimerHz(12);
    }

    void IpadPlayMasterStrip::timerCallback() { repaint(); }

    void IpadPlayMasterStrip::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        const int pad = layout::kIpadPlayMasterStripPadding;
        auto header = getLocalBounds().reduced(pad, pad).removeFromTop(layout::kIpadPlayMasterStripHeaderHeight);

        draw::fillGlowDot(g, juce::Point<float>(static_cast<float>(header.getX() + 4),
                                                static_cast<float>(header.getCentreY())),
                          4.0f, palette::kFigmaTeal, 1.0f, 4);

        g.setFont(fonts::label(layout::kIpadPlaySectionTitleSize));
        g.setColour(palette::kFigmaTextPrimary);
        g.drawText("MASTER MOTION", header.withTrimmedLeft(18).removeFromLeft(140), juce::Justification::centredLeft,
                   true);

        g.setFont(fonts::label(layout::kIpadPlayCaptionSize));
        g.setColour(palette::kFigmaTextDim);
        g.drawText("ENV · GLIDE · LPF", header.withTrimmedRight(8), juce::Justification::centredRight, true);

        if (!curvePlotBounds_.isEmpty())
        {
            auto plotFrame = curvePlotBounds_.toFloat();
            g.setColour(juce::Colour(0xff0a0b0e));
            g.fillRoundedRectangle(plotFrame, 4.0f);
            g.setColour(palette::kBorder.withAlpha(0.75f));
            g.drawRoundedRectangle(plotFrame.reduced(0.5f), 4.0f, 1.0f);
        }
    }

    void IpadPlayMasterStrip::resized()
    {
        const int pad = layout::kIpadPlayMasterStripPadding;
        auto content = getLocalBounds().reduced(pad, pad);
        content.removeFromTop(layout::kIpadPlayMasterStripHeaderHeight + 4);

        auto curveColumn = content.removeFromLeft(layout::kIpadPlayMasterStripCurveWidth);
        curvePlotBounds_ =
            curveColumn.withSizeKeepingCentre(curveColumn.getWidth() - 8, layout::kIpadPlayMasterStripCurveHeight);
        visualizer_.setBounds(curvePlotBounds_);

        content.removeFromLeft(layout::kIpadPlayMasterStripColumnGap);

        const int knobWidth = layout::kIpadPlayMasterKnobSize + 36;
        const int knobHeight = layout::kIpadPlayMasterStripKnobRowHeight;

        auto placeKnob = [&](GlowKnob& knob) {
            auto slot = content.removeFromLeft(knobWidth);
            knob.setBounds(slot.withSizeKeepingCentre(knobWidth, knobHeight));
            content.removeFromLeft(4);
        };

        placeKnob(*attackKnob_);
        placeKnob(*decayKnob_);
        placeKnob(*sustainKnob_);
        placeKnob(*releaseKnob_);
        content.removeFromLeft(8);
        placeKnob(*portamentoKnob_);
        placeKnob(*cutoffKnob_);
        placeKnob(*resonanceKnob_);
    }

} // namespace pw8::plugin::ui
