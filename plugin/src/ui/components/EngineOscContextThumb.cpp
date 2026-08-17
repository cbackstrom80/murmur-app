#include "EngineOscContextThumb.h"

#include <cmath>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"
#include "wireframe/WavetableMeshPaint.h"
#include "wireframe/WireframeProjection.h"

namespace pw8::plugin::ui
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

        [[nodiscard]] float noiseSpectralSlope(int variantIndex) noexcept
        {
            switch (variantIndex)
            {
                case 0: return 0.0f;
                case 1: return -0.55f;
                case 2: return -1.0f;
                case 3: return 0.45f;
                default: return -0.25f;
            }
        }

        [[nodiscard]] oscillator::WtWarpParams loadWavetableWarpParams(juce::AudioProcessorValueTreeState& apvts,
                                                                       std::size_t opIndex)
        {
            oscillator::WtWarpParams warpParams;
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtBend")))
                warpParams.bend = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtAsymmetry")))
                warpParams.asymmetry = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtSyncRatio")))
                warpParams.syncRatio = raw->load();
            if (auto* raw = apvts.getRawParameterValue(operatorParamId(opIndex, "WtSyncAmount")))
                warpParams.syncAmount = raw->load();
            return warpParams;
        }
    } // namespace

    EngineOscContextThumb::EngineOscContextThumb(PatchworkEightProcessor& processor, int engineIndex)
        : processor_(processor), engineIndex_(engineIndex)
    {
        setInterceptsMouseClicks(false, false);
    }

    void EngineOscContextThumb::setPreviewData(const EngineOscContextPreviewData& data) { preview_ = data; }

    void EngineOscContextThumb::paintFrame(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);
    }

    float EngineOscContextThumb::previewSample(const std::array<float, wireframe::kPreviewPoints>& buf, float t,
                                               float phaseOffset, float amp) const
    {
        float u = t + phaseOffset;
        u -= std::floor(u);
        const float f = u * static_cast<float>(wireframe::kPreviewPoints - 1);
        const int i0 = juce::jlimit(0, wireframe::kPreviewPoints - 1, static_cast<int>(f));
        const int i1 = (i0 + 1) % wireframe::kPreviewPoints;
        const float frac = f - static_cast<float>(i0);
        return juce::jlimit(-1.2f, 1.2f,
                            (buf[static_cast<std::size_t>(i0)] * (1.0f - frac) + buf[static_cast<std::size_t>(i1)] * frac) * amp);
    }

    void EngineOscContextThumb::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds();
        if (bounds.isEmpty())
            return;

        paintFrame(g, bounds);
        paintContent(g, bounds.reduced(1));
    }

    void EngineOscContextThumb::paintContent(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        switch (preview_.engineType)
        {
            case algorithm::EngineType::Classic: paintClassic(g, bounds); break;
            case algorithm::EngineType::Wavetable: paintWavetable(g, bounds); break;
            case algorithm::EngineType::FmPm: paintFm(g, bounds); break;
            case algorithm::EngineType::Additive: paintAdditive(g, bounds); break;
            case algorithm::EngineType::PhaseShape: paintPhaseShape(g, bounds); break;
            case algorithm::EngineType::Granular: paintGranular(g, bounds); break;
            case algorithm::EngineType::NoiseChaos: paintNoise(g, bounds); break;
            case algorithm::EngineType::Resonator: paintResonator(g, bounds); break;
            case algorithm::EngineType::External: paintExternal(g, bounds); break;
            default: break;
        }
    }

    void EngineOscContextThumb::paintClassic(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        if (preview_.classicPreviews == nullptr)
            return;

        const auto idx = static_cast<std::size_t>(engineIndex_);
        const int wf = juce::jlimit(0, 3, static_cast<int>(loadParam(processor_.apvts, operatorParamId(idx, "Waveform"))));
        const auto& samples = (*preview_.classicPreviews)[static_cast<std::size_t>(juce::jmin(wf, 3))];
        auto waveArea = bounds.reduced(8, 12).toFloat();
        const float centreY = waveArea.getCentreY();

        g.setColour(palette::kBorder.withAlpha(0.28f));
        g.drawHorizontalLine(static_cast<int>(centreY), waveArea.getX(), waveArea.getRight());

        if (wf == 3)
        {
            juce::Path zigzag;
            const int segments = 12;
            for (int i = 0; i <= segments; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(segments);
                const float x = waveArea.getX() + t * waveArea.getWidth();
                const float y = centreY + ((i % 2 == 0) ? -1.0f : 1.0f) * waveArea.getHeight() * 0.34f * preview_.motionGain;
                if (i == 0)
                    zigzag.startNewSubPath(x, y);
                else
                    zigzag.lineTo(x, y);
            }
            g.setColour(palette::kAccent.withAlpha(preview_.engineLive ? 0.92f : 0.72f));
            g.strokePath(zigzag, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        }
        else
        {
            wireframe::paintFlatWaveform(
                g, waveArea, wireframe::kPreviewPoints,
                [&](float t) {
                    return previewSample(samples, t, preview_.engineLive ? preview_.animPhase : 0.0f, preview_.motionGain);
                },
                preview_.engineLive);
        }

        static constexpr const char* kLabels[] = {"CLASSIC SINE", "CLASSIC TRI", "CLASSIC SAW", "CLASSIC SQR"};
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText(kLabels[juce::jmin(wf, 3)], bounds.removeFromBottom(10), juce::Justification::centred);
    }

    void EngineOscContextThumb::paintWavetable(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        const float pos = loadParam(processor_.apvts, operatorParamId(idx, "WavetablePos"));
        auto meshArea = bounds.reduced(6, 8).toFloat();

        if (const auto* table = processor_.getActiveWavetableTable(idx); table != nullptr && table->isValid())
        {
            const auto warpParams = loadWavetableWarpParams(processor_.apvts, idx);
            wireframe::paintWavetableTableMesh(g, meshArea, table, warpParams, pos, wireframe::contextThumbMeshOptions());
        }
        else
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(7.0f));
            g.drawText("No wavetable loaded", meshArea, juce::Justification::centred);
        }

        const int index = juce::jlimit(1, 256, static_cast<int>(pos * 255.0f + 1.0f));
        g.setColour(palette::kAccent.withAlpha(0.55f));
        const float markerX = meshArea.getX() + meshArea.getWidth() * pos;
        g.drawVerticalLine(static_cast<int>(markerX), meshArea.getY(), meshArea.getBottom());

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("INDEX: " + juce::String(index), bounds.removeFromBottom(10), juce::Justification::centredRight);
    }

    void EngineOscContextThumb::paintFm(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        if (preview_.fmLiveCarrier == nullptr || preview_.fmLiveMod == nullptr)
            return;

        const auto idx = static_cast<std::size_t>(engineIndex_);
        auto opArea = bounds.reduced(8, 12);
        auto waveArea = opArea.toFloat();

        const float modBandH = waveArea.getHeight() * 0.38f;
        auto modBand = waveArea.removeFromTop(modBandH);
        waveArea.removeFromTop(2.0f);
        auto carBand = waveArea;

        wireframe::paintFlatWaveform(
            g, modBand, wireframe::kPreviewPoints,
            [&](float t) { return previewSample(*preview_.fmLiveMod, t, 0.0f, 0.85f); }, false);
        wireframe::paintFlatWaveform(
            g, carBand, wireframe::kPreviewPoints,
            [&](float t) {
                return previewSample(*preview_.fmLiveCarrier, t, preview_.engineLive ? preview_.animPhase : 0.0f,
                                   preview_.motionGain);
            },
            preview_.engineLive);

        static constexpr const char* kOps[] = {"OP 4", "OP 3", "OP 2", "OP 1"};
        const int boxW = juce::jmin(44, (opArea.getWidth() - 36) / 4);
        const int boxH = 18;
        int x = opArea.getX();

        for (int i = 0; i < 4; ++i)
        {
            const auto box = juce::Rectangle<int>(x, opArea.getCentreY() - boxH / 2, boxW, boxH);
            const bool isCarrier = i == 3;
            g.setColour(isCarrier ? palette::kAccentDim.withAlpha(0.72f) : palette::kAccentDim.withAlpha(0.48f));
            g.fillRoundedRectangle(box.toFloat(), 3.0f);
            g.setColour(isCarrier ? palette::kAccent : palette::kAccent.withAlpha(0.65f));
            g.drawRoundedRectangle(box.toFloat().reduced(0.5f), 3.0f, isCarrier ? 1.4f : 1.0f);
            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::label(7.0f));
            g.drawText(kOps[i], box, juce::Justification::centred);

            if (i < 3)
            {
                g.setColour(palette::kAccent.withAlpha(0.75f));
                g.drawText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x92")),
                           juce::Rectangle<int>(box.getRight(), box.getY(), 12, boxH), juce::Justification::centred);
            }
            x += boxW + 12;
        }

        const auto op1 = juce::Rectangle<int>(opArea.getX() + (boxW + 12) * 3, opArea.getCentreY() - boxH / 2, boxW, boxH);
        juce::Path feedbackArc;
        feedbackArc.addArc(static_cast<float>(op1.getX() - 6), static_cast<float>(op1.getY() - 4),
                           static_cast<float>(boxW + 12), static_cast<float>(boxH + 8),
                           juce::MathConstants<float>::pi * 0.15f, juce::MathConstants<float>::pi * 1.35f, true);
        g.setColour(palette::kAccentWarm.withAlpha(0.55f));
        g.strokePath(feedbackArc, juce::PathStrokeType(1.0f));

        const float ratio = loadParam(processor_.apvts, operatorParamId(idx, "FmModulatorRatio"), 1.0f);
        const float index = loadParam(processor_.apvts, operatorParamId(idx, "FmModulatorIndex"), 0.5f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("RATIO " + juce::String(ratio, 2) + "  IDX " + juce::String(index, 2),
                   bounds.removeFromBottom(10), juce::Justification::centred);
    }

    void EngineOscContextThumb::paintAdditive(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        if (preview_.additiveHeights == nullptr)
            return;

        auto barArea = bounds.reduced(10, 14).toFloat();
        const int bars = juce::jmin(16, preview_.additiveBarCount);
        const float barW = barArea.getWidth() / static_cast<float>(bars + 1);

        juce::Path tiltCurve;
        for (int i = 0; i < bars; ++i)
        {
            const float h = (*preview_.additiveHeights)[static_cast<std::size_t>(i % preview_.additiveBarCount)];
            const float x = barArea.getX() + barW * (static_cast<float>(i) + 0.5f);
            const float towerH = h * barArea.getHeight() * 0.72f;
            const float yBase = barArea.getBottom();
            g.setColour(palette::kAccent.withAlpha(0.45f + static_cast<float>(i) * 0.025f));
            g.fillRoundedRectangle(x - barW * 0.22f, yBase - towerH, barW * 0.44f, towerH, 1.0f);

            const float curveY = yBase - towerH - 2.0f;
            if (i == 0)
                tiltCurve.startNewSubPath(x, curveY);
            else
                tiltCurve.lineTo(x, curveY);
        }

        g.setColour(palette::kAccent.withAlpha(0.35f));
        g.strokePath(tiltCurve, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
        for (int dash = 0; dash < bars; dash += 2)
        {
            const float x = barArea.getX() + barW * (static_cast<float>(dash) + 0.5f);
            g.setColour(palette::kBorder.withAlpha(0.22f));
            g.drawVerticalLine(static_cast<int>(x), barArea.getY(), barArea.getBottom());
        }

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("PARTIALS: " + juce::String(preview_.additiveBarCount), bounds.removeFromBottom(10),
                   juce::Justification::centredLeft);
    }

    void EngineOscContextThumb::paintPhaseShape(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        if (preview_.phaseOutPreviews == nullptr)
            return;

        auto gridArea = bounds.reduced(8, 10).toFloat();
        g.setColour(palette::kBackgroundBottom.withAlpha(0.35f));
        g.fillRoundedRectangle(gridArea, 3.0f);

        g.setColour(palette::kBorder.withAlpha(0.22f));
        for (int i = 1; i < 4; ++i)
        {
            const float t = static_cast<float>(i) / 4.0f;
            g.drawVerticalLine(static_cast<int>(gridArea.getX() + gridArea.getWidth() * t), gridArea.getY(),
                               gridArea.getBottom());
            g.drawHorizontalLine(static_cast<int>(gridArea.getY() + gridArea.getHeight() * t), gridArea.getX(),
                                 gridArea.getRight());
        }

        const int cell = juce::jlimit(0, 3, preview_.activeSubPickerIndex);
        const auto& samples = (*preview_.phaseOutPreviews)[static_cast<std::size_t>(juce::jmin(cell, 3))];

        juce::Path distortionCurve;
        for (int i = 0; i <= wireframe::kPreviewPoints; i += 2)
        {
            const float t = static_cast<float>(i) / static_cast<float>(wireframe::kPreviewPoints);
            const float yNorm = previewSample(samples, t, 0.0f, 1.0f);
            const float x = gridArea.getX() + t * gridArea.getWidth();
            const float y = gridArea.getCentreY() - yNorm * gridArea.getHeight() * 0.38f;
            if (i == 0)
                distortionCurve.startNewSubPath(x, y);
            else
                distortionCurve.lineTo(x, y);
        }
        g.setColour(palette::kAccent.withAlpha(0.18f));
        g.strokePath(distortionCurve, juce::PathStrokeType(2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        wireframe::paintFlatWaveform(
            g, gridArea, wireframe::kPreviewPoints,
            [&](float t) {
                return previewSample(samples, t, preview_.engineLive ? preview_.animPhase * 0.5f : 0.0f,
                                     preview_.motionGain);
            },
            preview_.engineLive);

        const float markerX = gridArea.getX() + gridArea.getWidth() * (static_cast<float>(cell) / 3.0f);
        g.setColour(palette::kMurmurViolet.withAlpha(0.55f));
        g.drawVerticalLine(static_cast<int>(markerX), gridArea.getY(), gridArea.getBottom());

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("PHASE DISTORT", bounds.removeFromBottom(10), juce::Justification::centred);
    }

    void EngineOscContextThumb::paintGranular(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        const auto idx = static_cast<std::size_t>(engineIndex_);
        const float pos = loadParam(processor_.apvts, operatorParamId(idx, "WavetablePos"));
        const float grainSizeMs = loadParam(processor_.apvts, operatorParamId(idx, "GrainSizeMs"), 60.0f);
        auto cloud = bounds.reduced(8, 10).toFloat();

        if (const auto* table = processor_.getActiveWavetableTable(idx); table != nullptr && table->isValid())
        {
            const auto warpParams = loadWavetableWarpParams(processor_.apvts, idx);
            wireframe::paintWavetableTableMesh(g, cloud, table, warpParams, pos, wireframe::contextThumbMeshOptions());

            wireframe::GranularOverlayParams grainParams;
            grainParams.wavetablePos = pos;
            grainParams.grainSizeMs = grainSizeMs;
            wireframe::paintGranularGrainOverlay(g, cloud, grainParams);
        }
        else
        {
            g.setColour(palette::kBackgroundBottom.withAlpha(0.4f));
            g.fillRoundedRectangle(cloud, 3.0f);
        }

        const float centreX = cloud.getX() + cloud.getWidth() * pos;
        g.setColour(palette::kMurmurViolet.withAlpha(0.22f));
        g.fillEllipse(centreX - 14.0f, cloud.getCentreY() - cloud.getHeight() * 0.34f, 28.0f, cloud.getHeight() * 0.68f);
        g.setColour(palette::kMurmurViolet.withAlpha(0.85f));
        g.drawVerticalLine(static_cast<int>(centreX), cloud.getY(), cloud.getBottom());

        const std::uint32_t seed = static_cast<std::uint32_t>(engineIndex_ * 997 + static_cast<int>(pos * 1000.0f));
        juce::Random rng(static_cast<int>(seed));
        for (int i = 0; i < 24; ++i)
        {
            const float gx = cloud.getX() + rng.nextFloat() * cloud.getWidth();
            const float gy = cloud.getY() + rng.nextFloat() * cloud.getHeight();
            const float dist = std::abs(gx - centreX) / cloud.getWidth();
            const float alpha = juce::jlimit(0.2f, 0.95f, 1.0f - dist * 1.35f);
            const float grainSize = 1.5f + rng.nextFloat() * 2.5f;
            g.setColour((i % 3 == 0 ? palette::kMurmurViolet : palette::kAccent).withAlpha(alpha));
            g.fillEllipse(gx - grainSize * 0.5f, gy - grainSize * 0.5f, grainSize, grainSize);
        }

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("GRAIN CLOUD", bounds.removeFromBottom(10), juce::Justification::centred);
    }

    void EngineOscContextThumb::paintNoise(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        auto specArea = bounds.reduced(8, 12).toFloat();
        const float slope = noiseSpectralSlope(preview_.activeSubPickerIndex);

        juce::Path curve;
        for (int i = 0; i <= 48; ++i)
        {
            const float t = static_cast<float>(i) / 48.0f;
            const float freqT = t * t;
            const float amp = juce::jlimit(0.08f, 0.92f, 0.15f + (1.0f - freqT) * (0.75f + slope * 0.35f));
            const float x = specArea.getX() + t * specArea.getWidth();
            const float y = specArea.getBottom() - amp * specArea.getHeight();
            if (i == 0)
                curve.startNewSubPath(x, y);
            else
                curve.lineTo(x, y);
        }
        juce::Path fill = curve;
        fill.lineTo(specArea.getRight(), specArea.getBottom());
        fill.lineTo(specArea.getX(), specArea.getBottom());
        fill.closeSubPath();
        g.setColour(palette::kAccent.withAlpha(0.12f));
        g.fillPath(fill);

        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.strokePath(curve, juce::PathStrokeType(1.4f));

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(6.5f));
        static constexpr const char* kFreqLabels[] = {"20Hz", "200Hz", "2kHz", "20kHz"};
        for (int i = 0; i < 4; ++i)
        {
            const float x = specArea.getX() + specArea.getWidth() * static_cast<float>(i) / 3.0f;
            g.drawText(kFreqLabels[i], static_cast<int>(x - 14.0f), static_cast<int>(specArea.getBottom() + 1), 28, 8,
                       juce::Justification::centred);
        }
    }

    void EngineOscContextThumb::paintResonator(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        if (preview_.resonatorHeights == nullptr)
            return;

        auto peakArea = bounds.reduced(10, 14).toFloat();
        g.setColour(palette::kBackgroundBottom.withAlpha(0.35f));
        g.fillRoundedRectangle(peakArea, 3.0f);

        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(peakArea.getBottom() - 2.0f), peakArea.getX(), peakArea.getRight());

        const int peaks = juce::jmin(5, preview_.resonatorBarCount);
        juce::Path envelope;
        for (int i = 0; i < peaks; ++i)
        {
            const float h = (*preview_.resonatorHeights)[static_cast<std::size_t>(i % preview_.resonatorBarCount)];
            const float x = peakArea.getX() + peakArea.getWidth() * (static_cast<float>(i + 1) / static_cast<float>(peaks + 1));
            const float peakH = h * peakArea.getHeight() * 0.85f;
            const float yTop = peakArea.getBottom() - peakH;

            if (i == 0)
                envelope.startNewSubPath(x, yTop);
            else
                envelope.lineTo(x, yTop);

            juce::Path spike;
            spike.startNewSubPath(x, peakArea.getBottom());
            spike.lineTo(x - 3.5f, peakArea.getBottom() - peakH * 0.35f);
            spike.lineTo(x, yTop);
            spike.lineTo(x + 3.5f, peakArea.getBottom() - peakH * 0.35f);
            spike.closeSubPath();
            g.setColour(palette::kAccent.withAlpha(0.42f + static_cast<float>(i) * 0.1f));
            g.fillPath(spike);
            g.setColour(palette::kAccent.withAlpha(0.75f));
            g.strokePath(spike, juce::PathStrokeType(0.8f));
        }
        envelope.lineTo(peakArea.getRight() - 4.0f, peakArea.getBottom() - 2.0f);
        g.setColour(palette::kAccent.withAlpha(0.28f));
        g.strokePath(envelope, juce::PathStrokeType(1.0f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

        const auto idx = static_cast<std::size_t>(engineIndex_);
        const float decay = loadParam(processor_.apvts, operatorParamId(idx, "ResonatorDecay"), 0.5f);
        const float damping = loadParam(processor_.apvts, operatorParamId(idx, "ResonatorDamping"), 0.5f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("DECAY " + juce::String(decay * 4.0f, 1) + "s  DAMP " + juce::String(static_cast<int>(damping * 100.0f)) + "%",
                   bounds.removeFromBottom(10), juce::Justification::centred);
    }

    void EngineOscContextThumb::paintExternal(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        static constexpr const char* kInputLabels[] = {"AUD1", "AUD2", "S.CH", "RESMP"};
        const int inputIdx = juce::jlimit(0, 3, preview_.activeSubPickerIndex);

        auto meterArea = bounds.reduced(10, 14);
        const int meterH = 6;
        const int gap = 4;
        auto leftRow = meterArea.removeFromTop(meterH);
        meterArea.removeFromTop(gap);
        auto rightRow = meterArea.removeFromTop(meterH);
        meterArea.removeFromTop(6);

        const float sidechainLevel = processor_.getSidechainLevel();
        const bool sidechainActive = processor_.getSidechainActive();
        const float baseLevel = sidechainActive ? juce::jlimit(0.05f, 1.0f, sidechainLevel)
                                                : (preview_.engineLive ? juce::jlimit(0.08f, 1.0f, preview_.motionGain * 0.62f)
                                                                       : 0.28f);

        float leftLevel = baseLevel;
        float rightLevel = baseLevel;
        switch (inputIdx)
        {
            case 0: rightLevel = baseLevel * 0.18f; break;
            case 1: leftLevel = baseLevel * 0.18f; break;
            case 2: break;
            default: leftLevel = baseLevel * 0.96f; rightLevel = baseLevel * 0.96f; break;
        }

        auto paintMeter = [&](juce::Rectangle<int> row, const char* label, float fill) {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(7.0f));
            g.drawText(label, row.removeFromLeft(10), juce::Justification::centredLeft);
            g.setColour(palette::kBackgroundBottom);
            g.fillRoundedRectangle(row.toFloat(), 2.0f);
            g.setColour(palette::kBorder.withAlpha(0.65f));
            g.drawRoundedRectangle(row.toFloat().reduced(0.5f), 2.0f, 0.8f);
            auto fillRect = row.toFloat().reduced(1.0f);
            fillRect.setWidth(fillRect.getWidth() * juce::jlimit(0.05f, 1.0f, fill));
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(fillRect, 1.5f);
        };

        paintMeter(leftRow, "L", leftLevel);
        paintMeter(rightRow, "R", rightLevel);

        if (preview_.engineLive || sidechainActive)
        {
            g.setColour(palette::kAccent.withAlpha(0.9f));
            g.fillEllipse(static_cast<float>(bounds.getRight() - 14), static_cast<float>(bounds.getY() + 8), 4.0f, 4.0f);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(6.5f));
            g.drawText(sidechainActive ? "SC" : "SIG", bounds.getRight() - 28, bounds.getY() + 6, 20, 8,
                       juce::Justification::centredLeft);
        }

        auto envArea = meterArea.toFloat();
        g.setColour(palette::kBorder.withAlpha(0.25f));
        g.drawHorizontalLine(static_cast<int>(envArea.getBottom() - 1.0f), envArea.getX(), envArea.getRight());

        juce::Path envCurve;
        const int envPoints = 32;
        for (int i = 0; i <= envPoints; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(envPoints);
            float env = baseLevel;
            if (sidechainActive)
                env *= 0.35f + 0.65f * std::sin(t * juce::MathConstants<float>::twoPi * 1.5f + preview_.animPhase * 4.0f);
            else
            {
                const float attack = juce::jmin(1.0f, t * 6.0f);
                const float release = std::exp(-static_cast<float>(i) * 0.08f);
                env *= attack * release
                       * (0.55f + 0.45f * std::sin(t * juce::MathConstants<float>::twoPi * 2.0f + preview_.animPhase * 5.0f));
            }
            const float x = envArea.getX() + t * envArea.getWidth();
            const float y = envArea.getBottom() - env * envArea.getHeight() * 0.85f;
            if (i == 0)
                envCurve.startNewSubPath(x, y);
            else
                envCurve.lineTo(x, y);
        }
        g.setColour(palette::kAccent.withAlpha(0.55f));
        g.strokePath(envCurve, juce::PathStrokeType(1.2f));

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.0f));
        g.drawText("IN: " + juce::String(kInputLabels[inputIdx]) + "  ENV FOLLOW", bounds.removeFromBottom(10),
                   juce::Justification::centred);
    }

} // namespace pw8::plugin::ui
