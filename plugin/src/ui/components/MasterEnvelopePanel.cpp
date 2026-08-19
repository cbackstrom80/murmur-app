#include "MasterEnvelopePanel.h"

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "../visualizer/PreviewDraw.h"
#include "../visualizer/PreviewSurface.h"
#include "../visualizer/VisualPreviewCache.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::String formatTimeSeconds(float seconds)
        {
            if (seconds < 1.0f)
                return juce::String(juce::roundToInt(seconds * 1000.0f)) + "ms";
            return juce::String(seconds, 2) + "s";
        }

        void styleReadoutValue(juce::Label& label)
        {
            label.setFont(fonts::label(10.0f));
            label.setColour(juce::Label::textColourId, palette::kFigmaTeal);
            label.setJustificationType(juce::Justification::centredLeft);
            label.setInterceptsMouseClicks(false, false);
        }
    } // namespace

    MasterEnvelopePanel::MasterEnvelopePanel(MurmurProcessor& processor)
        : processor_(processor)
    {
        if (murmur8::visualizerGpuEnabled())
        {
            visualizer_ = std::make_unique<murmur8::MurmurVisualizerComponent>(processor_.getVisualizerBus());
            visualizer_->setMode(murmur8::MurmurVisualizerComponent::Mode::EnvelopeVu);
            visualizer_->setEnvelopeOnly(true);
            addAndMakeVisible(*visualizer_);
        }

        auto& apvts = processor.apvts;

        attackKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Attack"), "A");
        decayKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Decay"), "D");
        sustainKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Sustain"), "S",
                                                 [](float value) { return juce::String(juce::roundToInt(value * 100.0f)) + "%"; });
        releaseKnob_ = std::make_unique<GlowKnob>(apvts, envelopeParamId(0, "Release"), "R");

        for (auto* knob : {attackKnob_.get(), decayKnob_.get(), sustainKnob_.get(), releaseKnob_.get()})
        {
            knob->applyFigmaContext(figma::KnobContext::ChromeMaster);
            addAndMakeVisible(*knob);
        }

        styleReadoutValue(attackValueLabel_);
        styleReadoutValue(decayValueLabel_);
        styleReadoutValue(sustainValueLabel_);
        styleReadoutValue(releaseValueLabel_);
        addAndMakeVisible(attackValueLabel_);
        addAndMakeVisible(decayValueLabel_);
        addAndMakeVisible(sustainValueLabel_);
        addAndMakeVisible(releaseValueLabel_);

        startTimerHz(12);
        refreshReadouts();
        refreshSegmentStrip();
    }

    void MasterEnvelopePanel::refreshReadouts()
    {
        auto read = [this](const char* suffix, float fallback) {
            if (auto* raw = processor_.apvts.getRawParameterValue(envelopeParamId(0, suffix)))
                return raw->load();
            return fallback;
        };

        attackValueLabel_.setText(formatTimeSeconds(read("Attack", 0.01f)), juce::dontSendNotification);
        decayValueLabel_.setText(formatTimeSeconds(read("Decay", 0.2f)), juce::dontSendNotification);
        sustainValueLabel_.setText(juce::String(juce::roundToInt(read("Sustain", 0.7f) * 100.0f)) + "%",
                                   juce::dontSendNotification);
        releaseValueLabel_.setText(formatTimeSeconds(read("Release", 0.3f)), juce::dontSendNotification);
    }

    void MasterEnvelopePanel::setCompactSectionMode(bool compact)
    {
        compactSectionMode_ = compact;
        for (auto* knob : {attackKnob_.get(), decayKnob_.get(), sustainKnob_.get(), releaseKnob_.get()})
        {
            if (designEngineLayout_ && !compact)
                knob->applyFigmaContext(figma::KnobContext::DesignEngineMaster);
            else
                knob->applyFigmaContext(compact ? figma::KnobContext::PlayBlades : figma::KnobContext::ChromeMaster);
        }
        resized();
        repaint();
    }

    void MasterEnvelopePanel::setDesignEngineLayout(bool enabled)
    {
        if (designEngineLayout_ == enabled)
            return;

        designEngineLayout_ = enabled;
        const auto styleReadout = [enabled](juce::Label& label) {
            label.setFont(fonts::label(enabled ? 10.0f : 10.0f));
            label.setColour(juce::Label::textColourId, palette::kFigmaTeal);
            label.setJustificationType(enabled ? juce::Justification::centred : juce::Justification::centredLeft);
        };
        for (auto* label : {&attackValueLabel_, &decayValueLabel_, &sustainValueLabel_, &releaseValueLabel_})
            styleReadout(*label);
        if (enabled && !compactSectionMode_)
        {
            for (auto* knob : {attackKnob_.get(), decayKnob_.get(), sustainKnob_.get(), releaseKnob_.get()})
                knob->applyFigmaContext(figma::KnobContext::DesignEngineMaster);
        }
        resized();
        repaint();
    }

    void MasterEnvelopePanel::timerCallback()
    {
        refreshReadouts();
        refreshSegmentStrip();
        repaint(curvePlotBounds_);
    }

    void MasterEnvelopePanel::refreshSegmentStrip()
    {
        segmentChain_ = processor_.getSegmentEnvelopeChain(0);
        if (selectedSegment_ >= static_cast<int>(segmentChain_.segmentCount))
            selectedSegment_ = segmentChain_.segmentCount > 0 ? static_cast<int>(segmentChain_.segmentCount) - 1 : 0;
        repaint(segmentStripBounds_);
    }

    void MasterEnvelopePanel::cycleSelectedSegmentShape()
    {
        if (selectedSegment_ < 0 || selectedSegment_ >= static_cast<int>(segmentChain_.segmentCount))
            return;
        auto chain = segmentChain_;
        auto& seg = chain.segments[static_cast<std::size_t>(selectedSegment_)];
        const auto easing = pw8::modulation::parseMorphEasing(seg.shape.empty() ? "linear" : seg.shape);
        seg.shape = pw8::modulation::morphEasingToString(pw8::modulation::cycleMorphEasing(easing));
        processor_.setSegmentEnvelopeChain(0, chain);
        refreshSegmentStrip();
    }

    void MasterEnvelopePanel::mouseDown(const juce::MouseEvent& event)
    {
        for (std::size_t i = 0; i < segmentChain_.segmentCount; ++i)
        {
            if (segmentDotBounds_[i].contains(event.getPosition()))
            {
                selectedSegment_ = static_cast<int>(i);
                repaint(segmentStripBounds_);
                return;
            }
            if (segmentEasingChipBounds_[i].contains(event.getPosition()))
            {
                selectedSegment_ = static_cast<int>(i);
                cycleSelectedSegmentShape();
                return;
            }
        }
    }

    void MasterEnvelopePanel::paintEnvelopeCurve(juce::Graphics& g, juce::Rectangle<float> plot) const
    {
        wireframe::EnvelopePreviewParams params;
        auto read = [this](const char* suffix, float fallback) {
            if (auto* raw = processor_.apvts.getRawParameterValue(envelopeParamId(0, suffix)))
                return raw->load();
            return fallback;
        };

        params.delaySeconds = read("Delay", 0.0f);
        params.attackSeconds = read("Attack", 0.01f);
        params.holdSeconds = read("Hold", 0.0f);
        params.decaySeconds = read("Decay", 0.2f);
        params.sustainLevel = read("Sustain", 0.7f);
        params.releaseSeconds = read("Release", 0.3f);

        const auto key = preview::envelopePreviewKey(params.delaySeconds, params.attackSeconds, params.holdSeconds,
                                                      params.decaySeconds, params.sustainLevel, params.releaseSeconds,
                                                      params.curveShape);
        envelopeSurface_.setPlotBounds(plot);
        envelopeSurface_.setDataKey(key);

        envelopeSurface_.paintBackground(g, [&](juce::Graphics& bg, juce::Rectangle<float> p)
                                         {
                                             bg.setColour(palette::kBorder.withAlpha(0.35f));
                                             bg.drawHorizontalLine(static_cast<int>(p.getBottom() - p.getHeight() * 0.06f),
                                                                   p.getX(), p.getRight());
                                         });

        envelopeSurface_.paintData(g, [&](juce::Graphics& dg, juce::Rectangle<float> p)
                                   {
                                       const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
                                           key, preview::kDefaultPolylinePoints,
                                           [&](int n, preview::PreviewPolyline& out)
                                           { preview::buildEnvelopePolyline(params, n, out); });
                                       preview::PolylineDrawOptions opts;
                                       opts.yScale = 0.88f;
                                       preview::paintPolylineCurve(dg, p, polyline, opts);
                                   });

        envelopeSurface_.paintOverlay(g, [&](juce::Graphics& og, juce::Rectangle<float> p)
                                      {
                                          const auto& env = processor_.getVisualizerBus().getEnvelope();
                                          const float level = env.level.load(std::memory_order_relaxed);
                                          const float progress = env.stageProgress.load(std::memory_order_relaxed);
                                          const float headX = p.getX() + progress * p.getWidth() * 0.85f;
                                          const float headY =
                                              p.getBottom() - level * p.getHeight() * 0.82f - p.getHeight() * 0.06f;
                                          draw::fillGlowDot(og, { headX, headY }, 4.0f, palette::kAccent, 0.95f, 5);
                                      });
    }

    void MasterEnvelopePanel::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        const int pad = compactSectionMode_ ? 12 : layout::kDesignModeV2MasterEnvelopePadding;
        const int headerHeight =
            compactSectionMode_ ? 20 : layout::kDesignModeV2MasterEnvelopeHeaderHeight;
        const auto headerBounds = getLocalBounds().reduced(pad, pad).removeFromTop(headerHeight);

        if (!curvePlotBounds_.isEmpty())
        {
            juce::Rectangle<int> curveShell;
            if (designEngineLayout_ && !compactSectionMode_)
            {
                curveShell = curvePlotBounds_.expanded(12, 10).withTrimmedTop(18).withTrimmedBottom(2);
            }
            else
            {
                curveShell = compactSectionMode_
                                 ? curvePlotBounds_.expanded(6, 4)
                                 : curvePlotBounds_.expanded(12, 30).withTrimmedTop(18).withTrimmedBottom(10);
            }
            draw::fillRecessedRoundedRect(g, curveShell.toFloat(), 6.0f);
            g.setColour(palette::kBorder.withAlpha(0.65f));
            g.drawRoundedRectangle(curveShell.toFloat().reduced(0.5f), 6.0f, 1.0f);

            if (!compactSectionMode_)
            {
                g.setFont(fonts::label(8.0f));
                g.setColour(palette::kFigmaTextDim);
                g.drawText("CURVE", curveShell.withHeight(10), juce::Justification::centredLeft, true);
            }

            if (visualizer_ == nullptr || !murmur8::visualizerGpuEnabled())
                paintEnvelopeCurve(g, curvePlotBounds_.toFloat().reduced(2.0f, 2.0f));

            if (!compactSectionMode_)
            {
                auto plotFrame = curvePlotBounds_.toFloat();
                g.setColour(palette::kBorder.withAlpha(0.75f));
                g.drawRoundedRectangle(plotFrame.reduced(0.5f), 4.0f, 1.0f);

                static constexpr const char* kAxisLabels[] = {"0ms", "100ms", "200ms", "300ms", "400ms"};
                auto axisRow = curvePlotBounds_.withY(curvePlotBounds_.getBottom() + 8).withHeight(9);
                g.setFont(fonts::label(7.0f));
                for (int i = 0; i < 5; ++i)
                {
                    const int x = curvePlotBounds_.getX()
                                  + (curvePlotBounds_.getWidth() * i) / 4
                                  - (i == 0 ? 0 : (i == 4 ? 18 : 10));
                    g.drawText(kAxisLabels[i], x, axisRow.getY(), 40, 9, juce::Justification::centredLeft, true);
                }
            }
        }

        static constexpr const char* kParamNames[] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE"};
        static constexpr const char* kKnobLetters[] = {"A", "D", "S", "R"};
        const juce::Label* valueLabels[] = {&attackValueLabel_, &decayValueLabel_, &sustainValueLabel_,
                                            &releaseValueLabel_};
        GlowKnob* knobs[] = {attackKnob_.get(), decayKnob_.get(), sustainKnob_.get(), releaseKnob_.get()};
        for (int i = 0; i < 4; ++i)
        {
            auto row = valueLabels[i]->getBounds();
            g.setFont(fonts::label(7.0f));
            g.setColour(palette::kFigmaTextDim);
            if (designEngineLayout_ && !compactSectionMode_)
            {
                g.drawText(kParamNames[i], row.getX(), row.getY() - 11, row.getWidth(), 9,
                           juce::Justification::centredTop, true);
                if (knobs[i] != nullptr)
                {
                    auto letterBounds = knobs[i]->getBounds().withY(knobs[i]->getBottom() - 2).withHeight(9);
                    g.setFont(fonts::label(8.0f));
                    g.setColour(palette::kFigmaTextDim);
                    g.drawText(kKnobLetters[i], letterBounds, juce::Justification::centred);
                }
            }
            else
            {
                g.drawText(kParamNames[i], row.getX(), row.getY() - 11, row.getWidth(), 9,
                           juce::Justification::centredLeft, true);
            }
        }

        auto header = headerBounds;
        if (compactSectionMode_)
        {
            g.setColour(palette::kPanelRaised.withAlpha(0.98f));
            g.fillRect(header.toFloat());
        }

        draw::fillGlowDot(g, juce::Point<float>(static_cast<float>(header.getX() + 4),
                                                static_cast<float>(header.getCentreY())),
                          4.0f, palette::kFigmaTeal, 1.0f, 4);

        g.setFont(fonts::label(compactSectionMode_ ? 11.0f : 12.0f));
        g.setColour(palette::kFigmaTextPrimary);
        g.drawText("MASTER ENVELOPE", header.withTrimmedLeft(18).removeFromLeft(160), juce::Justification::centredLeft,
                   false);

        if (!compactSectionMode_)
        {
            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kFigmaTextDim);
            g.drawText("AMP SHAPE", header.withTrimmedLeft(138).removeFromLeft(160), juce::Justification::centredLeft,
                       true);
        }

        auto onBounds = header.removeFromRight(22).withSizeKeepingCentre(22, 14);
        g.setColour(palette::kFigmaTeal.withAlpha(0.18f));
        g.fillRoundedRectangle(onBounds.toFloat(), 4.0f);
        g.setColour(palette::kFigmaTeal.withAlpha(0.85f));
        g.drawRoundedRectangle(onBounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kFigmaTeal);
        g.drawText("ON", onBounds, juce::Justification::centred, true);

        if (!compactSectionMode_ && !designEngineLayout_)
        {
            static constexpr const char* kSlopeLabels[] = {"LINEAR", "EXP", "LOG"};
            auto slopeRow = header.removeFromRight(132);
            for (int i = 0; i < 3; ++i)
            {
                auto pill = slopeRow.removeFromLeft(40).reduced(0, 1);
                slopeRow.removeFromLeft(4);
                const bool active = i == 0;
                g.setColour(active ? palette::kFigmaTeal.withAlpha(0.18f) : palette::kPanelRaised.withAlpha(0.35f));
                g.fillRoundedRectangle(pill.toFloat(), 4.0f);
                g.setColour(active ? palette::kFigmaTeal.withAlpha(0.85f) : palette::kBorder.withAlpha(0.55f));
                g.drawRoundedRectangle(pill.toFloat().reduced(0.5f), 4.0f, 1.0f);
                g.setFont(fonts::label(7.0f));
                g.setColour(active ? palette::kFigmaTeal : palette::kFigmaTextDim);
                g.drawText(kSlopeLabels[i], pill, juce::Justification::centred, true);
            }
        }

        if (!compactSectionMode_ && !designEngineLayout_ && !segmentStripBounds_.isEmpty() && segmentChain_.segmentCount > 0)
        {
            g.setFont(fonts::label(7.0f));
            g.setColour(palette::kFigmaTextDim);
            g.drawText("SEGMENTS", segmentStripBounds_.withHeight(10), juce::Justification::centredLeft, true);

            for (std::size_t i = 0; i < segmentChain_.segmentCount; ++i)
            {
                const bool selected = static_cast<int>(i) == selectedSegment_;
                auto dot = segmentDotBounds_[i].toFloat();
                g.setColour(selected ? palette::kFigmaTeal.withAlpha(0.35f) : palette::kPanelRaised.withAlpha(0.6f));
                g.fillEllipse(dot);
                g.setColour(selected ? palette::kFigmaTeal : palette::kBorder.withAlpha(0.7f));
                g.drawEllipse(dot.reduced(0.5f), 1.0f);

                auto chip = segmentEasingChipBounds_[i];
                g.setColour(selected ? palette::kAccent.withAlpha(0.2f) : palette::kPanelRaised.withAlpha(0.5f));
                g.fillRoundedRectangle(chip.toFloat(), 3.0f);
                g.setColour(selected ? palette::kAccent : palette::kFigmaTextDim);
                g.setFont(fonts::label(6.5f));
                const auto easing =
                    pw8::modulation::parseMorphEasing(segmentChain_.segments[i].shape.empty()
                                                     ? "linear"
                                                     : segmentChain_.segments[i].shape);
                g.drawText(pw8::modulation::morphEasingLabel(easing), chip, juce::Justification::centred, true);
            }

            if (segmentChain_.segmentCount > 1)
            {
                g.setColour(palette::kFigmaTeal.withAlpha(0.45f));
                const auto y = static_cast<float>(segmentDotBounds_[0].getCentreY());
                g.drawLine(static_cast<float>(segmentDotBounds_[0].getCentreX()), y,
                           static_cast<float>(segmentDotBounds_[segmentChain_.segmentCount - 1].getCentreX()), y, 1.0f);
            }
        }
    }

    void MasterEnvelopePanel::resized()
    {
        const int pad = compactSectionMode_ ? 12 : layout::kDesignModeV2MasterEnvelopePadding;
        auto content = getLocalBounds().reduced(pad, pad);
        content.removeFromTop(compactSectionMode_ ? 20 : layout::kDesignModeV2MasterEnvelopeHeaderHeight
                                                  + layout::kDesignModeV2MasterEnvelopeContentGap);

        const int curveWidth =
            compactSectionMode_ ? juce::jmax(240, content.getWidth() * 2 / 5) : layout::kDesignModeV2MasterEnvelopeCurveWidth;
        const int plotHeight =
            compactSectionMode_ ? juce::jmin(content.getHeight(), 100) : layout::kDesignModeV2MasterEnvelopePlotHeight;

        auto curveColumn = content.removeFromLeft(curveWidth);
        curvePlotBounds_ = curveColumn.withSizeKeepingCentre(curveColumn.getWidth() - (compactSectionMode_ ? 12 : 24),
                                                             plotHeight);
        envelopeSurface_.invalidateAll();
        if (visualizer_ != nullptr)
            visualizer_->setBounds(curvePlotBounds_);

        content.removeFromLeft(compactSectionMode_ ? 8 : layout::kDesignModeV2MasterEnvelopeContentGap);

        if (designEngineLayout_ && !compactSectionMode_)
        {
            content.removeFromLeft(24);

            const int colWidth = layout::kDesignModeV2MasterEnvelopeKnobColumnWidth;
            const int knobSize = layout::kDesignModeV2MasterEnvelopeKnobSize;
            const int colGap = 12;

            auto placeDesignColumn = [&](GlowKnob& knob, juce::Label& valueLabel, bool lastColumn) {
                auto col = content.removeFromLeft(colWidth);
                if (!lastColumn)
                    content.removeFromLeft(colGap);

                knob.setBounds(col.withTrimmedTop(16).removeFromTop(knobSize + 14));
                valueLabel.setBounds(col.getX(), col.getY() + 85, colWidth, 13);
                valueLabel.setJustificationType(juce::Justification::centred);
            };

            placeDesignColumn(*attackKnob_, attackValueLabel_, false);
            placeDesignColumn(*decayKnob_, decayValueLabel_, false);
            placeDesignColumn(*sustainKnob_, sustainValueLabel_, false);
            placeDesignColumn(*releaseKnob_, releaseValueLabel_, true);

            segmentStripBounds_ = {};
            return;
        }

        const int rowHeight = compactSectionMode_ ? layout::kDesktopPlayModeMasterEnvelopeCompactRowHeight
                                                  : layout::kDesignModeV2MasterEnvelopeControlRowHeight;
        const int rowGap = compactSectionMode_ ? layout::kDesktopPlayModeMasterEnvelopeCompactRowGap
                                               : layout::kDesignModeV2MasterEnvelopeControlRowGap;
        const int knobColumnWidth = compactSectionMode_ ? 52 : layout::kDesignModeV2MasterEnvelopeKnobColumnWidth;
        const int knobSize = compactSectionMode_ ? layout::kDesktopPlayModeMasterEnvelopeCompactKnobSize
                                                 : layout::kDesignModeV2MasterEnvelopeKnobSize;

        auto placeRow = [&](GlowKnob& knob, juce::Label& valueLabel) {
            auto row = content.removeFromTop(rowHeight);
            content.removeFromTop(rowGap);
            const int knobBoundsWidth = compactSectionMode_ ? knobSize : knobSize + 44;
            knob.setBounds(row.removeFromLeft(knobColumnWidth).withSizeKeepingCentre(knobBoundsWidth, rowHeight));
            valueLabel.setBounds(row.getX() + (compactSectionMode_ ? 8 : 12), row.getY() + (compactSectionMode_ ? 8 : 11),
                                 row.getWidth() - 8, 13);
        };

        placeRow(*attackKnob_, attackValueLabel_);
        placeRow(*decayKnob_, decayValueLabel_);
        placeRow(*sustainKnob_, sustainValueLabel_);
        placeRow(*releaseKnob_, releaseValueLabel_);

        if (!compactSectionMode_ && !designEngineLayout_ && (segmentChain_.segmentCount > 0))
        {
            segmentStripBounds_ = getLocalBounds().reduced(pad, pad);
            segmentStripBounds_.setY(getHeight() - (compactSectionMode_ ? 34 : 40));
            segmentStripBounds_.setHeight(compactSectionMode_ ? 28 : 34);

            auto dotRow = segmentStripBounds_.withTrimmedTop(12);
            for (std::size_t i = 0; i < pw8::envelope::kMaxSegmentEnvelopeSegments; ++i)
            {
                segmentDotBounds_[i] = dotRow.removeFromLeft(22).reduced(3);
                segmentEasingChipBounds_[i] =
                    juce::Rectangle<int>(segmentDotBounds_[i].getX() - 2, segmentStripBounds_.getBottom() - 12, 26, 10);
                dotRow.removeFromLeft(2);
            }
        }
        else
        {
            segmentStripBounds_ = {};
        }
    }

} // namespace pw8::plugin::ui
