#include "EngineAdsrMini.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"
#include "../visualizer/PreviewDraw.h"
#include "../visualizer/VisualPreviewCache.h"

namespace pw8::plugin::ui
{
    namespace
    {
        using layout::kEngineAdsrLabelsHeight;
        using layout::kEngineAdsrPreviewHeight;
        using layout::kEngineAdsrTickLabelFontSize;
        using layout::kEngineAdsrTickTrackHeight;
        using layout::kEngineAdsrTickWidth;

        [[nodiscard]] float readEnvParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                         float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }
    } // namespace

    EngineAdsrMini::EngineAdsrMini(PatchworkEightProcessor& processor, int engineIndex)
        : processor_(processor), engineIndex_(engineIndex)
    {
        startTimerHz(12);
        refreshFromApvts();
    }

    void EngineAdsrMini::refreshFromApvts()
    {
        const auto env = static_cast<std::size_t>(engineIndex_);
        params_.delaySeconds = readEnvParam(processor_.apvts, envelopeParamId(env, "Delay"));
        params_.attackSeconds = readEnvParam(processor_.apvts, envelopeParamId(env, "Attack"), 0.01f);
        params_.holdSeconds = readEnvParam(processor_.apvts, envelopeParamId(env, "Hold"));
        params_.decaySeconds = readEnvParam(processor_.apvts, envelopeParamId(env, "Decay"), 0.2f);
        params_.sustainLevel = readEnvParam(processor_.apvts, envelopeParamId(env, "Sustain"), 0.7f);
        params_.releaseSeconds = readEnvParam(processor_.apvts, envelopeParamId(env, "Release"), 0.3f);
        params_.curveShape = readEnvParam(processor_.apvts, envelopeParamId(env, "Curve"), 0.5f);
        repaint();
    }

    void EngineAdsrMini::timerCallback() { refreshFromApvts(); }

    EngineAdsrMini::TickKind EngineAdsrMini::tickAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < 4; ++i)
        {
            if (tickLayout_[static_cast<std::size_t>(i)].contains(pos))
                return static_cast<TickKind>(static_cast<int>(TickKind::Attack) + i);
        }
        return TickKind::None;
    }

    void EngineAdsrMini::applyTickDrag(TickKind kind, juce::Point<int> pos)
    {
        const auto env = static_cast<std::size_t>(engineIndex_);
        const int index = static_cast<int>(kind) - static_cast<int>(TickKind::Attack);
        if (index < 0 || index >= 4)
            return;

        const auto& tick = tickLayout_[static_cast<std::size_t>(index)];
        const float norm = 1.0f - juce::jlimit(0.0f, 1.0f,
                                               static_cast<float>(pos.y - tick.getY()) / static_cast<float>(tick.getHeight()));
        auto setParam = [&](const char* suffix, float minVal, float maxVal) {
            if (auto* param = processor_.apvts.getParameter(envelopeParamId(env, suffix)))
            {
                const float value = minVal + norm * (maxVal - minVal);
                param->setValueNotifyingHost(param->convertTo0to1(value));
            }
        };

        switch (kind)
        {
            case TickKind::Attack: setParam("Attack", 0.001f, 8.0f); break;
            case TickKind::Decay: setParam("Decay", 0.001f, 8.0f); break;
            case TickKind::Sustain: setParam("Sustain", 0.0f, 1.0f); break;
            case TickKind::Release: setParam("Release", 0.001f, 12.0f); break;
            default: break;
        }
        refreshFromApvts();
    }

    void EngineAdsrMini::mouseDown(const juce::MouseEvent& event)
    {
        activeTick_ = tickAt(event.getPosition());
        if (activeTick_ != TickKind::None)
            applyTickDrag(activeTick_, event.getPosition());
    }

    void EngineAdsrMini::mouseDrag(const juce::MouseEvent& event)
    {
        if (activeTick_ != TickKind::None)
            applyTickDrag(activeTick_, event.getPosition());
    }

    void EngineAdsrMini::paintPreview(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        draw::fillRecessedRoundedRect(g, bounds, 3.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, 1.0f);

        const auto key = preview::envelopePreviewKey(params_.delaySeconds, params_.attackSeconds, params_.holdSeconds,
                                                      params_.decaySeconds, params_.sustainLevel, params_.releaseSeconds,
                                                      params_.curveShape);
        const auto& polyline = preview::VisualPreviewCache::instance().getOrBuild(
            key, preview::kDefaultPolylinePoints,
            [&](int n, preview::PreviewPolyline& out) { preview::buildEnvelopePolyline(params_, n, out); });
        preview::PolylineDrawOptions opts;
        opts.liveGlow = false;
        opts.alpha = 0.95f;
        opts.strokeWidth = 1.2f;
        opts.yScale = 0.88f;
        preview::paintPolylineCurve(g, bounds.reduced(1.0f, 1.0f), polyline, opts);
    }

    void EngineAdsrMini::paintTicks(juce::Graphics& g, juce::Rectangle<int> /*bounds*/)
    {
        static constexpr const char* kLabels[] = {"A", "D", "S", "R"};
        const float values[] = {params_.attackSeconds, params_.decaySeconds, params_.sustainLevel, params_.releaseSeconds};
        const float maxValues[] = {8.0f, 8.0f, 1.0f, 12.0f};

        for (int i = 0; i < 4; ++i)
        {
            const auto tick = tickLayout_[static_cast<std::size_t>(i)].toFloat();
            const float norm = juce::jlimit(0.0f, 1.0f, values[i] / maxValues[i]);
            const auto track = juce::Rectangle<float>(tick.getCentreX() - 1.0f, tick.getY(), 2.0f,
                                                      static_cast<float>(kEngineAdsrTickTrackHeight));
            g.setColour(palette::kBackgroundBottom);
            g.fillRoundedRectangle(track, 1.0f);
            auto fill = track;
            fill.setY(track.getBottom() - track.getHeight() * norm);
            fill.setHeight(track.getHeight() * norm);
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(fill, 1.0f);

            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(static_cast<float>(kEngineAdsrTickLabelFontSize)));
            g.drawText(kLabels[i], tick.withTrimmedTop(static_cast<float>(kEngineAdsrTickTrackHeight)),
                       juce::Justification::centred);
        }
    }

    void EngineAdsrMini::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds();
        paintPreview(g, bounds.removeFromTop(kEngineAdsrPreviewHeight).toFloat().reduced(0.0f, 0.0f));
        paintTicks(g, bounds.removeFromTop(kEngineAdsrLabelsHeight));
    }

    void EngineAdsrMini::resized()
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(kEngineAdsrPreviewHeight);
        auto labelRow = bounds.removeFromTop(kEngineAdsrLabelsHeight);

        const int totalTickWidth = kEngineAdsrTickWidth * 4;
        int x = labelRow.getX();
        if (labelRow.getWidth() > totalTickWidth)
            x += (labelRow.getWidth() - totalTickWidth) / 2;

        for (int i = 0; i < 4; ++i)
        {
            tickLayout_[static_cast<std::size_t>(i)] = {x, labelRow.getY(), kEngineAdsrTickWidth, kEngineAdsrLabelsHeight};
            x += kEngineAdsrTickWidth;
            if (i == 1)
                x += 4;
            if (i == 2)
                x += 4;
        }
    }

} // namespace pw8::plugin::ui
