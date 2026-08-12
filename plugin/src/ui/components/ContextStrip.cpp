#include "ContextStrip.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "state/PluginState.h"

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

        juce::Colour engineScopeAccent(algorithm::EngineType engine)
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
                default: return palette::kAccent;
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
        juce::Colour scopeAccent = palette::kAccentWarm;
        algorithm::EngineType liveEngine = algorithm::EngineType::Classic;
        if (scope_ == FilterPanelScope::Engine)
        {
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(engineIndex_), "Engine")))
                liveEngine = static_cast<algorithm::EngineType>(static_cast<int>(raw->load() + 0.5f));
            scopeAccent = engineScopeAccent(liveEngine);
        }
        g.setColour(scopeAccent.withAlpha(0.32f));
        g.fillRoundedRectangle(badgeArea, 4.0f);
        g.setColour(scopeAccent);
        g.setFont(fonts::label(9.5f));
        g.drawText(scope_ == FilterPanelScope::Global ? "GLOBAL" : "ENGINE",
                   badgeArea, juce::Justification::centred);

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        juce::String line;
        if (scope_ == FilterPanelScope::Global)
            line = "LAYER A · SUM → GLOBAL FILTER → FX → MASTER OUT";
        else
        {
            line = "ENGINE " + juce::String(engineIndex_) + " · " + engineName(liveEngine) +
                   " · per-engine filter (see FILTER tab)";
        }
        g.drawText(line, bounds.reduced(8.0f, 0.0f), juce::Justification::centredLeft);
    }

    void ContextStrip::resized() {}

} // namespace pw8::plugin::ui
