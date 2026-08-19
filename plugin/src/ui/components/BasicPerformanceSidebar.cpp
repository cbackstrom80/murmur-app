#include "BasicPerformanceSidebar.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ModRoutingUi.h"
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
    } // namespace

    BasicPerformanceSidebar::BasicPerformanceSidebar(PatchworkEightProcessor& processor) : processor_(processor)
    {
        portamentoKnob_ =
            std::make_unique<GlowKnob>(processor_.apvts, kPortamentoId, "GLIDE",
                                       [](float value) { return formatPortamento(value); });
        portamentoKnob_->applyFigmaContext(figma::KnobContext::BasicPortamento);
        addAndMakeVisible(*portamentoKnob_);

        startTimerHz(30);
        refreshFromPatch();
    }

    void BasicPerformanceSidebar::refreshFromPatch() { rebuildMacroKnobs(); }

    void BasicPerformanceSidebar::rebuildMacroKnobs()
    {
        macroKnobs_.clear();
        const auto& patch = processor_.getCurrentPatch();
        const auto layout = inferPatchFocusLayout(patch, layout::kMurmurBasicViewMacroCount, 0, &processor_.apvts);

        auto pushKnob = [this](const PatchFocusKnobSpec& spec) {
            if (macroKnobs_.size() >= layout::kMurmurBasicViewMacroCount)
                return;

            if (spec.kind == PatchFocusKnobKind::Macro && spec.macroIndex < processor_.getCurrentPatch().macros.size())
            {
                const auto paramId = kMacroParameterIds[spec.macroIndex];
                auto knob = std::make_unique<GlowKnob>(processor_.apvts, paramId, spec.label);
                knob->applyFigmaContext(figma::KnobContext::BasicMacro);
                addAndMakeVisible(*knob);
                macroKnobs_.push_back(std::move(knob));
                return;
            }

            if (spec.kind == PatchFocusKnobKind::Morph)
            {
                auto knob = std::make_unique<GlowKnob>(processor_.apvts, kMorphPositionId, spec.label);
                knob->applyFigmaContext(figma::KnobContext::BasicMacro);
                addAndMakeVisible(*knob);
                macroKnobs_.push_back(std::move(knob));
            }
        };

        for (const auto& spec : layout.featureKnobs)
            pushKnob(spec);

        for (std::size_t i = 0; i < patch.macros.size() && macroKnobs_.size() < layout::kMurmurBasicViewMacroCount; ++i)
        {
            PatchFocusKnobSpec spec{PatchFocusKnobKind::Macro, i, {},
                                    patch.macros[i].name.empty()
                                        ? juce::String(kMacroParameterNames[i])
                                        : juce::String(patch.macros[i].name)};
            const bool duplicate =
                std::any_of(layout.featureKnobs.begin(), layout.featureKnobs.end(),
                            [&](const PatchFocusKnobSpec& existing) {
                                return existing.kind == PatchFocusKnobKind::Macro && existing.macroIndex == i;
                            });
            if (!duplicate)
                pushKnob(spec);
        }

        resized();
    }

    void BasicPerformanceSidebar::timerCallback()
    {
        const int pulled =
            processor_.readScopeSamples(scopeScratch_.data(), static_cast<int>(scopeScratch_.size()));
        if (pulled > 0)
        {
            const auto [rms, peak] = scope::measureMonoBlock(scopeScratch_.data(), pulled);
            leftVu_.processFrame(rms, peak);
            rightVu_.processFrame(rms * 0.92f, peak * 0.95f);
        }
        else
        {
            const float masterPeak = processor_.getMasterOutPeakLinear();
            leftVu_.processFrame(masterPeak * 0.707f, masterPeak);
            rightVu_.processFrame(masterPeak * 0.65f, masterPeak * 0.95f);
        }

        repaint(leftMeterBounds_);
        repaint(rightMeterBounds_);
    }

    void BasicPerformanceSidebar::paintVerticalMeter(juce::Graphics& g, juce::Rectangle<float> bounds,
                                                     const scope::VuBallistics& vu, const char* label) const
    {
        if (bounds.isEmpty())
            return;

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kFigmaTextDim);
        g.drawText(label, bounds.removeFromTop(10.0f), juce::Justification::centred, true);

        auto track = bounds.reduced(4.0f, 2.0f);
        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.75f));
        g.drawRoundedRectangle(track.reduced(0.5f), 4.0f, 1.0f);

        auto fill = track.reduced(2.0f);
        const float fillHeight = fill.getHeight() * vu.rmsNorm();
        auto fillRect = fill.withTop(fill.getBottom() - fillHeight);
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.fillRoundedRectangle(fillRect, 2.0f);

        const float peakY = fill.getBottom() - fill.getHeight() * vu.peakHoldNorm();
        g.setColour(palette::kAccentWarm.withAlpha(0.9f));
        g.fillRect(fill.getX() + 1.0f, peakY - 1.0f, fill.getWidth() - 2.0f, 2.0f);
    }

    void BasicPerformanceSidebar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        const int pad = layout::kMurmurBasicViewSidebarPadding;
        auto header = getLocalBounds().reduced(pad, pad).removeFromTop(16);
        draw::fillGlowDot(g, juce::Point<float>(static_cast<float>(header.getX() + 4),
                                                static_cast<float>(header.getCentreY())),
                          4.0f, palette::kFigmaTeal, 1.0f, 4);
        g.setFont(fonts::label(12.0f));
        g.setColour(palette::kFigmaTextPrimary);
        g.drawText("PERFORMANCE", header.withTrimmedLeft(18), juce::Justification::centredLeft, true);

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kFigmaTextDim);
        g.drawText("GLIDE · MACROS · OUTPUT", header.withTrimmedRight(8), juce::Justification::centredRight, true);

        if (!leftMeterBounds_.isEmpty())
            paintVerticalMeter(g, leftMeterBounds_.toFloat(), leftVu_, "L");
        if (!rightMeterBounds_.isEmpty())
            paintVerticalMeter(g, rightMeterBounds_.toFloat(), rightVu_, "R");
    }

    void BasicPerformanceSidebar::resized()
    {
        const int pad = layout::kMurmurBasicViewSidebarPadding;
        auto content = getLocalBounds().reduced(pad, pad);
        content.removeFromTop(20);

        auto vuRow = content.removeFromBottom(layout::kMurmurBasicViewVuMeterHeight);
        content.removeFromBottom(8);
        const int meterWidth = (vuRow.getWidth() - 8) / 2;
        leftMeterBounds_ = vuRow.removeFromLeft(meterWidth);
        vuRow.removeFromLeft(8);
        rightMeterBounds_ = vuRow;

        auto glideRow = content.removeFromTop(layout::kMurmurBasicViewPortamentoKnobSize + 28);
        portamentoKnob_->setBounds(glideRow.withSizeKeepingCentre(layout::kMurmurBasicViewPortamentoKnobSize + 40,
                                                                   layout::kMurmurBasicViewPortamentoKnobSize + 24));
        content.removeFromTop(8);

        if (macroKnobs_.empty())
            return;

        const int rows = static_cast<int>(macroKnobs_.size());
        const int rowHeight = juce::jmax(96, content.getHeight() / rows);
        for (auto& knob : macroKnobs_)
        {
            auto row = content.removeFromTop(rowHeight);
            knob->setBounds(row.withSizeKeepingCentre(layout::kMurmurBasicViewMacroKnobSize + 48, rowHeight - 4));
        }
    }

} // namespace pw8::plugin::ui
