#include "ModRoutingWireframeView.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "../ModRoutingUi.h"
#include "../../visualizer/PreviewDraw.h"
#include "../../visualizer/VisualPreviewCache.h"
#include "LfoPreviewSampler.h"
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
        startTimerHz(15);
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
        const auto lfoKey = preview::lfoPreviewKey(static_cast<int>(lfoWaveform_), 2, lfoAnimPhase_);
        const auto& lfoPoly = preview::VisualPreviewCache::instance().getOrBuild(
            lfoKey, preview::kDefaultPolylinePoints,
            [&](int n, preview::PreviewPolyline& out)
            {
                out.xNorm.clear();
                out.yNorm.resize(static_cast<std::size_t>(n));
                for (int i = 0; i < n; ++i)
                {
                    const float t = static_cast<float>(i) / static_cast<float>(n - 1);
                    const float phase = std::fmod(t * 2.0f + lfoAnimPhase_, 1.0f);
                    if (lfoWaveform_ == lfo::LfoWaveform::SampleHold)
                        out.yNorm[static_cast<std::size_t>(i)] = sampleLfoSampleHold(phase);
                    else if (lfoWaveform_ == lfo::LfoWaveform::SmoothRandom)
                        out.yNorm[static_cast<std::size_t>(i)] =
                            sampleLfoSmoothRandom(phase, static_cast<int>(lfoAnimPhase_ * 6.0f));
                    else
                        out.yNorm[static_cast<std::size_t>(i)] = sampleLfoWaveform(lfoWaveform_, phase);
                }
            });
        preview::paintPolylineCurve(g, lfoArea, lfoPoly);

        g.setColour(palette::kTextDim);
        g.drawText("AMP ENV", envArea.removeFromTop(10.0f), juce::Justification::centredLeft);
        const auto envKey = preview::envelopePreviewKey(
            envParams_.delaySeconds, envParams_.attackSeconds, envParams_.holdSeconds, envParams_.decaySeconds,
            envParams_.sustainLevel, envParams_.releaseSeconds, envParams_.curveShape);
        const auto& envPoly = preview::VisualPreviewCache::instance().getOrBuild(
            envKey, preview::kDefaultPolylinePoints,
            [&](int n, preview::PreviewPolyline& out) { preview::buildEnvelopePolyline(envParams_, n, out); });
        preview::PolylineDrawOptions envOpts;
        envOpts.liveGlow = false;
        envOpts.alpha = 0.85f;
        envOpts.strokeWidth = 1.6f;
        envOpts.yScale = 0.88f;
        preview::paintPolylineCurve(g, envArea, envPoly, envOpts);

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
            const auto range = modAmountRangeFor(route.destination);
            const float def = defaultModAmountFor(route.destination);
            const float norm = juce::jlimit(0.0f, 1.0f, std::abs(route.amount) / juce::jmax(0.001f, std::abs(def)));
            const float strokeW = 1.2f + norm * 3.0f;

            juce::Path link;
            link.startNewSubPath(from);
            const float cx = (from.x + to.x) * 0.5f;
            link.quadraticTo(cx, from.y, to.x, to.y);

            g.setColour(colour.withAlpha(0.15f + norm * 0.25f));
            g.strokePath(link, juce::PathStrokeType(strokeW + 2.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
            g.setColour(colour.withAlpha(0.65f + norm * 0.35f));
            g.strokePath(link, juce::PathStrokeType(strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        if (activeRoutes == 0)
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(9.5f));
            g.drawText("No active routes", routeArea, juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui::wireframe
