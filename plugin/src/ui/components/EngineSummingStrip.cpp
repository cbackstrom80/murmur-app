#include "EngineSummingStrip.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNumEngines = 8;
        constexpr float kLevelMax = 4.0f;

        juce::String engineAbbrev(algorithm::EngineType engine)
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic: return "CLS";
                case algorithm::EngineType::Wavetable: return "WT";
                case algorithm::EngineType::FmPm: return "FM";
                case algorithm::EngineType::Additive: return "ADD";
                case algorithm::EngineType::PhaseShape: return "PHS";
                case algorithm::EngineType::Granular: return "GRN";
                case algorithm::EngineType::NoiseChaos: return "NSE";
                case algorithm::EngineType::Resonator: return "RES";
                default: return "OFF";
            }
        }

        juce::Colour engineAccent(algorithm::EngineType engine)
        {
            switch (engine)
            {
                case algorithm::EngineType::Wavetable: return palette::kAccent;
                case algorithm::EngineType::Granular: return palette::kAccentWarm;
                case algorithm::EngineType::FmPm: return palette::kEdgePhaseMod;
                case algorithm::EngineType::Additive: return palette::kEdgeFrequencyMod;
                case algorithm::EngineType::PhaseShape: return palette::kEdgeAmplitudeMod;
                case algorithm::EngineType::NoiseChaos: return palette::kModVelocity;
                case algorithm::EngineType::Resonator: return palette::kEdgeRingMod;
                case algorithm::EngineType::Classic: return palette::kEdgeSync;
                default: return palette::kTextDim;
            }
        }

        [[nodiscard]] algorithm::EngineType readEngine(juce::AudioProcessorValueTreeState& apvts, int index)
        {
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(static_cast<std::size_t>(index), "Engine")))
                return static_cast<algorithm::EngineType>(static_cast<int>(raw->load() + 0.5f));
            return algorithm::EngineType::Classic;
        }
    } // namespace

    EngineSummingStrip::EngineSummingStrip(PatchworkEightProcessor& processor) : processor_(processor)
    {
        addAndMakeVisible(panel_);

        helpLabel_.setText(
            "Eight engines sum into Layer A, then Global Filter, FX, and Master Out. Drag faders to set each engine Level.",
            juce::dontSendNotification);
        helpLabel_.setJustificationType(juce::Justification::centredLeft);
        helpLabel_.setFont(fonts::value(10.0f));
        helpLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        panel_.addAndMakeVisible(helpLabel_);

        layerGainKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kLayerGainId, "Layer");
        masterGainKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "Master", nullptr, palette::kAccentWarm);
        panel_.addAndMakeVisible(*layerGainKnob_);
        panel_.addAndMakeVisible(*masterGainKnob_);

        panel_.addMouseListener(this, false);

        startTimerHz(12);
    }

    EngineSummingStrip::~EngineSummingStrip()
    {
        panel_.removeMouseListener(this);
        stopTimer();
    }

    void EngineSummingStrip::setHighlightedEngine(int engineIndex)
    {
        highlightedEngine_ = juce::jlimit(0, kNumEngines - 1, engineIndex);
        repaint();
    }

    juce::Rectangle<float> EngineSummingStrip::faderArea() const
    {
        auto content = panel_.getContentBounds().toFloat();
        content = content.translated(static_cast<float>(panel_.getX()), static_cast<float>(panel_.getY()));
        content.removeFromTop(18.0f);
        content.removeFromRight(108.0f);
        return content.reduced(6.0f, 4.0f);
    }

    juce::Rectangle<float> EngineSummingStrip::faderColumnBounds(int engineIndex) const
    {
        auto area = faderArea();
        const float colW = area.getWidth() / static_cast<float>(kNumEngines);
        return {area.getX() + colW * static_cast<float>(engineIndex), area.getY(), colW, area.getHeight()};
    }

    int EngineSummingStrip::faderIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < kNumEngines; ++i)
        {
            if (faderColumnBounds(i).contains(pos.toFloat()))
                return i;
        }
        return -1;
    }

    float EngineSummingStrip::readLevel(int engineIndex) const
    {
        if (auto* raw = processor_.apvts.getRawParameterValue(
                operatorParamId(static_cast<std::size_t>(engineIndex), "Level")))
            return raw->load();
        return 1.0f;
    }

    void EngineSummingStrip::setLevelFromY(int engineIndex, float yInColumn)
    {
        const auto col = faderColumnBounds(engineIndex);
        const float trackTop = col.getY() + 22.0f;
        const float trackBottom = col.getBottom() - 28.0f;
        const float trackH = juce::jmax(1.0f, trackBottom - trackTop);
        const float norm = 1.0f - juce::jlimit(0.0f, 1.0f, (yInColumn - trackTop) / trackH);
        const float level = norm * kLevelMax;

        const auto id = operatorParamId(static_cast<std::size_t>(engineIndex), "Level");
        if (auto* param = processor_.apvts.getParameter(id))
            param->setValueNotifyingHost(param->convertTo0to1(level));
    }

    void EngineSummingStrip::adjustLevelAtMouse(juce::Point<int> pos)
    {
        const int index = faderIndexAt(pos);
        if (index < 0)
            return;
        dragEngine_ = index;
        highlightedEngine_ = index;
        setLevelFromY(index, static_cast<float>(pos.y));
        if (onEngineClicked)
            onEngineClicked(index);
        repaint();
    }

    void EngineSummingStrip::timerCallback()
    {
        bool changed = false;
        for (int i = 0; i < kNumEngines; ++i)
        {
            const float level = readLevel(i);
            const int engine = static_cast<int>(readEngine(processor_.apvts, i));
            if (level != cachedLevels_[static_cast<std::size_t>(i)] || engine != cachedEngines_[static_cast<std::size_t>(i)])
            {
                cachedLevels_[static_cast<std::size_t>(i)] = level;
                cachedEngines_[static_cast<std::size_t>(i)] = engine;
                changed = true;
            }
        }
        if (changed)
            repaint();
    }

    void EngineSummingStrip::paintOverChildren(juce::Graphics& g)
    {
        const auto area = faderArea();
        if (area.isEmpty())
            return;

        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(area, 6.0f);
        g.setColour(palette::kBorderBright.withAlpha(0.65f));
        g.drawRoundedRectangle(area.reduced(0.5f), 6.0f, 1.0f);

        const float sumX = area.getRight() - 2.0f;
        const float sumY = area.getCentreY();
        const float busY = area.getBottom() - 14.0f;

        juce::Path busPath;
        busPath.startNewSubPath(area.getX() + 8.0f, busY);
        busPath.lineTo(sumX - 4.0f, busY);
        busPath.lineTo(sumX - 4.0f, sumY + 18.0f);

        for (int i = 0; i < kNumEngines; ++i)
        {
            const auto col = faderColumnBounds(i);
            const auto engine = static_cast<algorithm::EngineType>(cachedEngines_[static_cast<std::size_t>(i)]);
            const auto accent = engineAccent(engine);
            const float level = cachedLevels_[static_cast<std::size_t>(i)];
            const float norm = juce::jlimit(0.0f, 1.0f, level / kLevelMax);
            const bool highlighted = i == highlightedEngine_;

            const float trackX = col.getCentreX() - 7.0f;
            const float trackTop = col.getY() + 22.0f;
            const float trackBottom = col.getBottom() - 28.0f;
            const float trackH = trackBottom - trackTop;
            juce::Rectangle<float> track(trackX, trackTop, 14.0f, trackH);

            g.setColour(palette::kBackgroundBottom.withAlpha(0.85f));
            g.fillRoundedRectangle(track, 3.0f);
            g.setColour(highlighted ? accent : palette::kBorder);
            g.drawRoundedRectangle(track, 3.0f, highlighted ? 1.4f : 0.9f);

            if (norm > 0.001f)
            {
                auto fill = track.withTop(trackBottom - trackH * norm);
                g.setColour(accent.withAlpha(highlighted ? 0.92f : 0.62f));
                g.fillRoundedRectangle(fill, 3.0f);
            }

            const float capY = trackBottom - trackH * norm;
            g.setColour(accent.brighter(highlighted ? 0.15f : 0.0f));
            g.fillEllipse(trackX - 1.0f, capY - 4.0f, 16.0f, 8.0f);

            auto labelCol = col;
            g.setColour(highlighted ? palette::kTextPrimary : palette::kTextSecondary);
            g.setFont(fonts::label(9.5f));
            g.drawText("E" + juce::String(i + 1), labelCol.removeFromTop(16.0f), juce::Justification::centred);

            g.setColour(accent.withAlpha(0.9f));
            g.setFont(fonts::label(8.5f));
            g.drawText(engineAbbrev(engine), labelCol.removeFromBottom(14.0f), juce::Justification::centred);

            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(8.5f));
            g.drawText(juce::String(level, 2), labelCol.removeFromBottom(14.0f), juce::Justification::centred);

            juce::Path tap;
            tap.startNewSubPath(col.getCentreX(), trackBottom);
            tap.lineTo(col.getCentreX(), busY - 2.0f);
            tap.lineTo(sumX - 18.0f, busY - 2.0f);
            g.setColour(accent.withAlpha(0.18f + norm * 0.35f));
            g.strokePath(tap, juce::PathStrokeType(1.1f));
        }

        g.setColour(branding::glowColour().withAlpha(0.45f));
        g.strokePath(busPath, juce::PathStrokeType(1.4f));

        const juce::Rectangle<float> sumNode(sumX - 22.0f, sumY - 10.0f, 20.0f, 20.0f);
        g.setColour(palette::kAccentWarm.withAlpha(0.22f));
        g.fillEllipse(sumNode);
        g.setColour(palette::kAccentWarm);
        g.drawEllipse(sumNode, 1.3f);
        g.setFont(fonts::label(7.5f));
        g.setColour(palette::kTextPrimary);
        g.drawText("Σ", sumNode, juce::Justification::centred);

        g.setFont(fonts::label(8.5f));
        g.setColour(palette::kTextDim);
        g.drawText("LAYER A BUS", juce::Rectangle<float>(area.getX() + 8.0f, busY - 2.0f, 80.0f, 12.0f),
                   juce::Justification::centredLeft);
        g.drawText("→ FILTER → FX → OUT", juce::Rectangle<float>(sumX - 120.0f, busY - 2.0f, 116.0f, 12.0f),
                   juce::Justification::centredRight);
    }

    void EngineSummingStrip::resized()
    {
        panel_.setBounds(getLocalBounds());
        auto content = panel_.getContentBounds();
        helpLabel_.setBounds(content.removeFromTop(18));
        content.removeFromRight(4);

        auto outColumn = content.removeFromRight(104);
        const int knobH = outColumn.getHeight() / 2;
        layerGainKnob_->setBounds(outColumn.removeFromTop(knobH).reduced(2));
        masterGainKnob_->setBounds(outColumn.reduced(2));
    }

    void EngineSummingStrip::mouseDown(const juce::MouseEvent& event)
    {
        if (event.eventComponent == &panel_ || event.eventComponent == &helpLabel_)
            adjustLevelAtMouse(event.getEventRelativeTo(this).getPosition());
    }

    void EngineSummingStrip::mouseDrag(const juce::MouseEvent& event)
    {
        if (dragEngine_ >= 0)
            setLevelFromY(dragEngine_, static_cast<float>(event.getEventRelativeTo(this).getPosition().y));
        else if (event.eventComponent == &panel_ || event.eventComponent == &helpLabel_)
            adjustLevelAtMouse(event.getEventRelativeTo(this).getPosition());
    }

} // namespace pw8::plugin::ui
