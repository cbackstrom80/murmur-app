#include "ContextStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::String engineName(algorithm::EngineType engine)
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic: return "CLASSIC";
                case algorithm::EngineType::Wavetable: return "WAVETABLE";
                case algorithm::EngineType::FmPm: return "FM/PM";
                case algorithm::EngineType::Additive: return "ADDITIVE";
                case algorithm::EngineType::PhaseShape: return "PHASE SHAPE";
                case algorithm::EngineType::Granular: return "GRANULAR";
                case algorithm::EngineType::NoiseChaos: return "NOISE";
                case algorithm::EngineType::Resonator: return "RESONATOR";
                default: return "ENGINE";
            }
        }
    } // namespace

    ContextStrip::ContextStrip(PatchworkEightProcessor& processor) : processor_(processor) {}

    void ContextStrip::setScope(FilterPanelScope scope, int engineIndex)
    {
        scope_ = scope;
        engineIndex_ = juce::jlimit(0, 7, engineIndex);
        repaint();
    }

    void ContextStrip::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(palette::kBorderBright);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        const auto badgeArea = bounds.removeFromLeft(scope_ == FilterPanelScope::Global ? 72.0f : 88.0f).reduced(6.0f, 5.0f);
        g.setColour(scope_ == FilterPanelScope::Global ? palette::kAccentWarm.withAlpha(0.35f)
                                                       : palette::kAccent.withAlpha(0.28f));
        g.fillRoundedRectangle(badgeArea, 4.0f);
        g.setColour(scope_ == FilterPanelScope::Global ? palette::kAccentWarm : palette::kAccent);
        g.setFont(fonts::label(9.5f));
        g.drawText(scope_ == FilterPanelScope::Global ? "GLOBAL" : "ENGINE",
                   badgeArea, juce::Justification::centred);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        juce::String line;
        if (scope_ == FilterPanelScope::Global)
            line = "LAYER A · GLOBAL · affects full layer output after all engines";
        else
        {
            const auto& op = processor_.getCurrentPatch().layerA.operators[static_cast<std::size_t>(engineIndex_)];
            line = "ENGINE " + juce::String(engineIndex_) + " · " + engineName(op.engine) +
                   " · filters this engine's own output";
        }
        g.drawText(line, bounds.reduced(8.0f, 0.0f), juce::Justification::centredLeft);
    }

    void ContextStrip::resized() {}

} // namespace pw8::plugin::ui
