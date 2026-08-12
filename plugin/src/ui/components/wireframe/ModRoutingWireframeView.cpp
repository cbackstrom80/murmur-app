#include "ModRoutingWireframeView.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "WireframeProjection.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                       float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }

        void appendEnvSegment(juce::Path& path, float x0, float x1, float y0, float y1, float curveShape, bool shapeUp,
                               int steps)
        {
            path.lineTo(x0, y0);
            for (int i = 1; i <= steps; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(steps);
                const float shaped = shapeUp ? envelopeShapeUp(t, curveShape) : envelopeShapeDown(t, curveShape);
                path.lineTo(x0 + (x1 - x0) * t, y0 + (y1 - y0) * shaped);
            }
        }

        void buildCompactEnvelopePath(const EnvelopePreviewParams& params, juce::Rectangle<float> bounds,
                                       juce::Path& path)
        {
            const float minStage = 0.08f;
            const float sustainDisplay = 0.22f;
            const float delayW = params.delaySeconds > 0.0f ? juce::jmax(minStage, params.delaySeconds * 0.02f) : 0.0f;
            const float attackW = juce::jmax(minStage, params.attackSeconds * 0.15f);
            const float holdW = params.holdSeconds > 0.0f ? minStage : 0.0f;
            const float decayW = juce::jmax(minStage, params.decaySeconds * 0.08f);
            const float releaseW = juce::jmax(minStage, params.releaseSeconds * 0.08f);
            const float totalW = delayW + attackW + holdW + decayW + sustainDisplay + releaseW;

            auto xFor = [&](float start) { return bounds.getX() + (start / totalW) * bounds.getWidth(); };
            auto yFor = [&](float level) { return bounds.getBottom() - level * bounds.getHeight() * 0.85f; };

            path.clear();
            float cursor = 0.0f;
            path.startNewSubPath(xFor(cursor), yFor(0.0f));
            if (delayW > 0.0f)
            {
                path.lineTo(xFor(cursor + delayW), yFor(0.0f));
                cursor += delayW;
            }
            appendEnvSegment(path, xFor(cursor), xFor(cursor + attackW), yFor(0.0f), yFor(1.0f), params.curveShape,
                             true, 16);
            cursor += attackW;
            if (holdW > 0.0f)
            {
                path.lineTo(xFor(cursor + holdW), yFor(1.0f));
                cursor += holdW;
            }
            appendEnvSegment(path, xFor(cursor), xFor(cursor + decayW), yFor(1.0f), yFor(params.sustainLevel),
                             params.curveShape, false, 16);
            cursor += decayW;
            path.lineTo(xFor(cursor + sustainDisplay), yFor(params.sustainLevel));
            appendEnvSegment(path, xFor(cursor + sustainDisplay), xFor(cursor + sustainDisplay + releaseW),
                             yFor(params.sustainLevel), yFor(0.0f), params.curveShape, false, 16);
        }

        [[nodiscard]] juce::Point<float> sourceAnchor(int sourceIndex, juce::Rectangle<float> area) noexcept
        {
            const float y = area.getY() + area.getHeight() * (0.22f + static_cast<float>(sourceIndex) * 0.28f);
            return {area.getX() + area.getWidth() * 0.12f, y};
        }

        [[nodiscard]] juce::Point<float> destinationAnchor(int destIndex, juce::Rectangle<float> area) noexcept
        {
            const float y = area.getY() + area.getHeight() * (0.35f + static_cast<float>(destIndex) * 0.3f);
            return {area.getRight() - area.getWidth() * 0.12f, y};
        }

        [[nodiscard]] int sourceIndexFor(modulation::ModSource source) noexcept
        {
            switch (source)
            {
                case modulation::ModSource::Lfo1: return 0;
                case modulation::ModSource::Env1: return 1;
                case modulation::ModSource::Velocity: return 2;
                default: return -1;
            }
        }

        [[nodiscard]] int destinationIndexFor(modulation::ModDestination destination) noexcept
        {
            switch (destination)
            {
                case modulation::ModDestination::FilterCutoff:
                case modulation::ModDestination::OperatorFilterCutoff:
                    return 0;
                case modulation::ModDestination::FilterResonance:
                case modulation::ModDestination::OperatorFilterResonance:
                    return 1;
                default:
                    return -1;
            }
        }
    } // namespace

    ModRoutingWireframeView::ModRoutingWireframeView(PatchworkEightProcessor& processor,
                                                      juce::AudioProcessorValueTreeState& apvts)
        : processor_(processor), apvts_(apvts)
    {
        setCaption("MOD ROUTING · LIVE");
        setSubCaption("Drag sources onto ringed filter knobs");
        startTimerHz(10);
    }

    ModRoutingWireframeView::~ModRoutingWireframeView() { stopTimer(); }

    void ModRoutingWireframeView::timerCallback()
    {
        lfoWaveform_ =
            static_cast<lfo::LfoWaveform>(static_cast<int>(loadParam(apvts_, lfoParamId(0, "Waveform")) + 0.5f));
        lfoRateHz_ = loadParam(apvts_, lfoParamId(0, "RateHz"), 2.0f);
        lfoAnimPhase_ += lfoRateHz_ * 0.01f;
        if (lfoAnimPhase_ > 1.0f)
            lfoAnimPhase_ -= 1.0f;

        const auto envPrefix = envelopeParamId(0, "");
        envParams_.delaySeconds = loadParam(apvts_, envPrefix + "Delay");
        envParams_.attackSeconds = loadParam(apvts_, envPrefix + "Attack", 0.005f);
        envParams_.holdSeconds = loadParam(apvts_, envPrefix + "Hold");
        envParams_.decaySeconds = loadParam(apvts_, envPrefix + "Decay", 0.2f);
        envParams_.sustainLevel = loadParam(apvts_, envPrefix + "Sustain", 0.7f);
        envParams_.releaseSeconds = loadParam(apvts_, envPrefix + "Release", 0.3f);
        envParams_.curveShape = loadParam(apvts_, envPrefix + "Curve", 2.0f);

        repaint();
    }

    void ModRoutingWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);

        auto lfoArea = bounds.removeFromTop(bounds.getHeight() * 0.34f).reduced(2.0f, 0.0f);
        auto envArea = bounds.removeFromTop(bounds.getHeight() * 0.38f).reduced(2.0f, 4.0f);
        auto routeArea = bounds.reduced(2.0f, 0.0f);

        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("LFO 1", lfoArea.removeFromTop(10.0f), juce::Justification::centredLeft);
        paintFlatWaveform(g, lfoArea, 64,
                          [&](float t)
                          {
                              const float phase = std::fmod(t * 2.0f + lfoAnimPhase_, 1.0f);
                              if (lfoWaveform_ == lfo::LfoWaveform::SampleHold)
                                  return sampleLfoSampleHold(phase);
                              if (lfoWaveform_ == lfo::LfoWaveform::SmoothRandom)
                                  return sampleLfoSmoothRandom(phase, static_cast<int>(lfoAnimPhase_ * 6.0f));
                              return sampleLfoWaveform(lfoWaveform_, phase);
                          },
                          true);

        g.setColour(palette::kTextDim);
        g.drawText("AMP ENV", envArea.removeFromTop(10.0f), juce::Justification::centredLeft);
        juce::Path envPath;
        buildCompactEnvelopePath(envParams_, envArea, envPath);
        strokeGlowPath(g, envPath, 0.85f, 1.6f, false);

        g.setColour(palette::kTextDim);
        g.drawText("ROUTES", routeArea.removeFromTop(10.0f), juce::Justification::centredLeft);
        routeArea = routeArea.reduced(0.0f, 4.0f);

        const char* sourceLabels[] = {"LFO", "ENV", "VEL"};
        const char* destLabels[] = {"CUTOFF", "RESO"};
        for (int i = 0; i < 3; ++i)
        {
            const auto pt = sourceAnchor(i, routeArea);
            g.setColour(palette::kPanelRaised);
            g.fillEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f);
            g.setColour(palette::kBorderBright);
            g.drawEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f, 1.0f);
            g.drawText(sourceLabels[i], juce::Rectangle<float>(pt.x - 16.0f, pt.y + 6.0f, 32.0f, 10.0f),
                       juce::Justification::centred);
        }
        for (int i = 0; i < 2; ++i)
        {
            const auto pt = destinationAnchor(i, routeArea);
            g.setColour(palette::kAccent.withAlpha(0.2f));
            g.fillEllipse(pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);
            g.setColour(palette::kAccent);
            g.drawEllipse(pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f, 1.2f);
            g.drawText(destLabels[i], juce::Rectangle<float>(pt.x - 22.0f, pt.y + 8.0f, 44.0f, 10.0f),
                       juce::Justification::centred);
        }

        int activeRoutes = 0;
        for (const auto& route : processor_.getCurrentPatch().layerA.modRoutes)
        {
            if (!route.isActive())
                continue;

            const int src = sourceIndexFor(route.source);
            const int dst = destinationIndexFor(route.destination);
            if (src < 0 || dst < 0)
                continue;

            ++activeRoutes;
            const auto from = sourceAnchor(src, routeArea);
            const auto to = destinationAnchor(dst, routeArea);
            const auto colour = palette::modSourceColour(static_cast<int>(route.source));

            juce::Path link;
            link.startNewSubPath(from);
            const float cx = (from.x + to.x) * 0.5f;
            link.quadraticTo(cx, from.y, to.x, to.y);

            g.setColour(colour.withAlpha(0.2f));
            g.strokePath(link, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour(colour);
            g.strokePath(link, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        if (activeRoutes == 0)
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(9.5f));
            g.drawText("No active routes", routeArea, juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui::wireframe
