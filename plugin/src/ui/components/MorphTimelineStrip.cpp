#include "MorphTimelineStrip.h"

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
        juce::Colour keyframeColour(const patch::MorphKoinKeyframe& kf, std::size_t index)
        {
            if (!kf.color.empty() && kf.color.front() == '#')
            {
                const auto hex = juce::String(kf.color.c_str()).substring(1);
                return juce::Colour::fromString("ff" + hex);
            }
            static constexpr juce::uint32 kDefault[] = {0xff4a90d9, 0xff9f80ff, 0xffe0a040, 0xff40e0c0,
                                                        0xffe06080, 0xff80e040, 0xff6080e0, 0xffe08040};
            return juce::Colour(kDefault[index % 8]);
        }

        juce::String autoplayChipLabel(const std::string& source)
        {
            if (source == "lfo1")
                return "LFO1";
            if (source == "lfo2")
                return "LFO2";
            if (source == "lfo3")
                return "LFO3";
            if (source == "lfo4")
                return "LFO4";
            if (source == "modWheel")
                return "MW";
            return "OFF";
        }

        juce::String curveChipLabel(const std::string& curve)
        {
            if (curve == "smooth")
                return "SMOOTH";
            if (curve == "step")
                return "STEP";
            if (curve == "sine" || curve == "inOutSine")
                return "SINE";
            if (curve == "bounce")
                return "BOUNCE";
            if (curve == "inQuartic")
                return "IN4";
            if (curve == "outQuartic")
                return "OUT4";
            return "LINEAR";
        }
    } // namespace

    MorphTimelineStrip::MorphTimelineStrip(MurmurProcessor& processor)
        : processor_(processor),
          morphKnob_(std::make_unique<GlowKnob>(processor.apvts, kMorphPositionId, "MORPH POS",
                                                [](float v) { return juce::String(v, 2); }))
    {
        morphKnob_->applyFigmaContext(figma::KnobContext::OperatorHero);
        addAndMakeVisible(*morphKnob_);
        startTimerHz(15);
    }

    void MorphTimelineStrip::setShowMorphKnob(bool show)
    {
        showMorphKnob_ = show;
        morphKnob_->setVisible(show);
        resized();
    }

    void MorphTimelineStrip::setCompactHubMode(bool compact)
    {
        compactHubMode_ = compact;
        setShowMorphKnob(!compact);
        resized();
    }

    void MorphTimelineStrip::refresh() { repaint(); }

    juce::Rectangle<float> MorphTimelineStrip::gradientTrackBounds() const noexcept
    {
        auto track = trackBounds_.toFloat();
        const float inset = static_cast<float>(layout::kMorphTimelineGradientTrackInsetX);
        const float trackY = track.getCentreY() - static_cast<float>(layout::kMorphTimelineGradientTrackHeight) * 0.5f;
        const float trackW =
            compactHubMode_
                ? track.getWidth() - inset * 2.0f
                : juce::jmin(track.getWidth() - inset * 2.0f,
                             static_cast<float>(layout::kMorphTimelineGradientTrackInsetX
                                                + 580)); // Figma gradient-bg-track width
        return {track.getX() + inset, trackY, trackW, static_cast<float>(layout::kMorphTimelineGradientTrackHeight)};
    }

    float MorphTimelineStrip::positionFromX(int x) const noexcept
    {
        const auto track = gradientTrackBounds();
        if (track.getWidth() <= 0.0f)
            return 0.0f;
        return juce::jlimit(0.0f, 1.0f, (static_cast<float>(x) - track.getX()) / track.getWidth());
    }

    int MorphTimelineStrip::xFromPosition(float position) const noexcept
    {
        const auto track = gradientTrackBounds();
        return static_cast<int>(track.getX()
                                + juce::jlimit(0.0f, 1.0f, position) * track.getWidth());
    }

    int MorphTimelineStrip::hitKeyframeIndex(juce::Point<int> pos) const
    {
        const auto& mk = processor_.getCurrentPatch().morphKoin;
        int best = -1;
        int bestDist = 14;
        for (std::size_t i = 0; i < mk.keyframes.size(); ++i)
        {
            const int kx = xFromPosition(mk.keyframes[i].position);
            const int dist = std::abs(pos.x - kx);
            if (dist <= bestDist)
            {
                bestDist = dist;
                best = static_cast<int>(i);
            }
        }
        return best;
    }

    void MorphTimelineStrip::setMorphPosition(float position)
    {
        if (auto* param = processor_.apvts.getParameter(kMorphPositionId))
            param->setValueNotifyingHost(param->convertTo0to1(juce::jlimit(0.0f, 1.0f, position)));
    }

    void MorphTimelineStrip::paintChip(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text,
                                       juce::Colour accent) const
    {
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(bounds, 4.0f);
        g.setColour(accent.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kTextDim);
        g.drawText(text, bounds, juce::Justification::centred, true);
    }

    void MorphTimelineStrip::paintHeader(juce::Graphics& g, juce::Rectangle<int> area) const
    {
        if (compactHubMode_ || area.isEmpty())
            return;

        const auto& mk = processor_.getCurrentPatch().morphKoin;
        auto row = area.reduced(0, 2);

        draw::fillGlowDot(g, juce::Point<float>(static_cast<float>(row.getX() + 3),
                                                static_cast<float>(row.getY() + 6)),
                          3.0f, palette::kFigmaTeal, 1.0f, 3);

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextPrimary);
        g.drawText("MORPH TIMELINE", row.withTrimmedLeft(12).removeFromTop(13), juce::Justification::centredLeft, true);

        auto badge = row.withTrimmedLeft(96).removeFromTop(10).removeFromLeft(48).toFloat();
        g.setColour(palette::kFigmaTeal.withAlpha(0.12f));
        g.fillRoundedRectangle(badge, 3.0f);
        g.setColour(palette::kFigmaTeal.withAlpha(0.85f));
        g.drawRoundedRectangle(badge.reduced(0.5f), 3.0f, 1.0f);
        g.setFont(fonts::label(6.5f));
        g.drawText(juce::String(mk.label.c_str()).toUpperCase(), badge, juce::Justification::centred, true);

        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kTextDim);
        g.drawText("Sweep seamlessly between keyframe snapshots", row.withTrimmedTop(15).removeFromTop(10),
                   juce::Justification::centredLeft, true);

        auto chipRow = row.removeFromBottom(layout::kMorphTimelineChipHeight);
        int chipX = chipRow.getX();
        const int chipGap = layout::kMorphTimelineChipGap;
        const juce::String curveText = "CURVE: " + curveChipLabel(mk.curve);
        const int curveW = juce::jmax(67, static_cast<int>(curveText.length() * 5.5f));
        paintChip(g, juce::Rectangle<float>(static_cast<float>(chipX), chipRow.getY(), static_cast<float>(curveW),
                                            static_cast<float>(chipRow.getHeight())),
                  curveText, palette::kAccent);
        chipX += curveW + chipGap;

        const juce::String wrapText = juce::String("WRAP: ") + (mk.wrap ? "ON" : "OFF");
        paintChip(g, juce::Rectangle<float>(static_cast<float>(chipX), chipRow.getY(), 46.0f,
                                            static_cast<float>(chipRow.getHeight())),
                  wrapText, palette::kAccentWarm);
        chipX += 46 + chipGap;

        const juce::String autoText = "AUTO: " + autoplayChipLabel(mk.autoplaySource);
        paintChip(g, juce::Rectangle<float>(static_cast<float>(chipX), chipRow.getY(), 54.0f,
                                            static_cast<float>(chipRow.getHeight())),
                  autoText, palette::kFigmaTeal);
    }

    void MorphTimelineStrip::paintTrack(juce::Graphics& g, juce::Rectangle<int> area)
    {
        trackBounds_ = area;
        const auto track = gradientTrackBounds();

        juce::ColourGradient hueRing(juce::Colour(0xff4a90d9), track.getX(), track.getCentreY(),
                                     juce::Colour(0xff40e0c0), track.getRight(), track.getCentreY(), false);
        hueRing.addColour(0.0, juce::Colour(0xff4a90d9));
        hueRing.addColour(0.33, juce::Colour(0xff9f80ff));
        hueRing.addColour(0.66, juce::Colour(0xffe0a040));
        hueRing.addColour(1.0, juce::Colour(0xff40e0c0));
        g.setGradientFill(hueRing);
        g.fillRoundedRectangle(track, static_cast<float>(layout::kMorphTimelineGradientTrackHeight) * 0.5f);

        g.setColour(palette::kBorder.withAlpha(0.65f));
        g.drawRoundedRectangle(track.reduced(0.5f), static_cast<float>(layout::kMorphTimelineGradientTrackHeight) * 0.5f,
                               1.0f);

        const auto& mk = processor_.getCurrentPatch().morphKoin;
        for (std::size_t i = 0; i < mk.keyframes.size(); ++i)
        {
            const float px = static_cast<float>(xFromPosition(mk.keyframes[i].position));
            const auto colour = keyframeColour(mk.keyframes[i], i);
            const bool flash = static_cast<int>(i) == frStepFlashKeyframe_;

            g.setColour(colour.withAlpha(0.85f));
            g.drawLine(px, track.getY() - static_cast<float>(layout::kMorphTimelineKeyframeTickHeight), px,
                       track.getY(), flash ? 2.0f : 1.0f);

            g.setFont(fonts::label(compactHubMode_ ? 6.5f : 7.0f));
            g.setColour(palette::kTextPrimary);
            const juce::String name = juce::String(mk.keyframes[i].name.c_str());
            g.drawText(name, static_cast<int>(px) - 24, static_cast<int>(track.getY()) - 22, 48, 10,
                       juce::Justification::centred);

            g.setColour(palette::kTextDim);
            g.drawText(juce::String(mk.keyframes[i].position, 2), static_cast<int>(px) - 16,
                       static_cast<int>(track.getBottom()) + 2, 32, 9, juce::Justification::centred);

            if (flash)
            {
                g.setColour(colour.withAlpha(0.35f));
                g.fillEllipse(px - 8.0f, track.getCentreY() - 8.0f, 16.0f, 16.0f);
            }
            g.setColour(colour);
            g.fillEllipse(px - 4.0f, track.getCentreY() - 4.0f, 8.0f, 8.0f);
        }

        float morphPos = 0.0f;
        if (auto* raw = processor_.apvts.getRawParameterValue(kMorphPositionId))
            morphPos = raw->load();

        const float playX = static_cast<float>(xFromPosition(morphPos));
        const float headSize = static_cast<float>(layout::kMorphTimelinePlayheadSize);
        juce::Path head;
        head.addTriangle(playX - headSize * 0.5f, track.getY() - 4.0f, playX + headSize * 0.5f, track.getY() - 4.0f,
                         playX, track.getY() - headSize - 4.0f);
        g.setColour(palette::kAccent);
        g.fillPath(head);
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.drawLine(playX, track.getY(), playX, track.getBottom() + 8.0f, 1.5f);
    }

    void MorphTimelineStrip::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds();
        g.setColour(palette::kPanelRaised.withAlpha(0.35f));
        g.fillRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);

        if (!compactHubMode_)
            paintHeader(g, headerBounds_);
        paintTrack(g, trackBounds_);
    }

    void MorphTimelineStrip::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kMorphTimelinePanelPadding);

        if (compactHubMode_)
        {
            headerBounds_ = {};
            if (showMorphKnob_)
            {
                morphKnob_->setBounds(bounds.removeFromRight(72).reduced(4));
                bounds.removeFromRight(4);
            }
            trackBounds_ = bounds;
        }
        else
        {
            headerBounds_ = bounds.removeFromLeft(layout::kMorphTimelineHeaderWidth);
            bounds.removeFromLeft(16);

            if (showMorphKnob_)
            {
                morphKnob_->setBounds(bounds.removeFromRight(layout::kMorphTimelineKnobBlockWidth).reduced(8, 4));
                bounds.removeFromRight(8);
            }

            trackBounds_ = bounds;
        }

        repaint();
    }

    void MorphTimelineStrip::mouseDown(const juce::MouseEvent& event)
    {
        if (const int kf = hitKeyframeIndex(event.getPosition()); kf >= 0)
        {
            const auto& mk = processor_.getCurrentPatch().morphKoin;
            if (static_cast<std::size_t>(kf) < mk.keyframes.size())
            {
                setMorphPosition(mk.keyframes[static_cast<std::size_t>(kf)].position);
                if (onKeyframeSelected)
                    onKeyframeSelected(static_cast<std::size_t>(kf));
                return;
            }
        }

        draggingPlayhead_ = true;
        setMorphPosition(positionFromX(event.x));
    }

    void MorphTimelineStrip::mouseDrag(const juce::MouseEvent& event)
    {
        if (draggingPlayhead_)
            setMorphPosition(positionFromX(event.x));
    }

    void MorphTimelineStrip::mouseUp(const juce::MouseEvent&)
    {
        draggingPlayhead_ = false;
    }

    void MorphTimelineStrip::timerCallback()
    {
        if (processor_.consumeMorphKeyframeCrossFlash())
        {
            frStepFlashKeyframe_ = processor_.getMorphKeyframeCrossedIndex();
            repaint();
        }
        else if (frStepFlashKeyframe_ >= 0)
        {
            frStepFlashKeyframe_ = -1;
            repaint();
        }
    }

} // namespace pw8::plugin::ui
