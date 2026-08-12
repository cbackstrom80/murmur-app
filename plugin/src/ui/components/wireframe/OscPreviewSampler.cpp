#include "OscPreviewSampler.h"

#include <cmath>

#include <juce_core/juce_core.h>

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        constexpr double kPreviewSampleRate = 48000.0;

        [[nodiscard]] float warpPhasePreview(float t, const PhaseShapePreviewParams& params) noexcept
        {
            const float curveA = std::sin(pw8::dsp::kTwoPi * t);
            const float curveB = std::sin(2.0f * pw8::dsp::kTwoPi * t);
            const float shape = pw8::dsp::clamp(params.phaseShape, 0.0f, 1.0f);
            const float base = pw8::dsp::lerp(curveA, curveB, shape);
            const float asymmetry = pw8::dsp::clamp(params.phaseAsymmetry, -1.0f, 1.0f);
            const float skewed = base * (1.0f + asymmetry * std::cos(pw8::dsp::kTwoPi * t));
            const float bend = pw8::dsp::clamp(params.phaseBend, -1.0f, 1.0f);
            constexpr float kWarpDepth = 0.2f;
            return pw8::dsp::wrapPhase(t + bend * skewed * kWarpDepth);
        }

        [[nodiscard]] float harmonicRatio(float harmonicNum, float stretch) noexcept
        {
            constexpr float kMaxCoeff = 0.0006f;
            const float coeff = stretch * kMaxCoeff;
            const float inside = std::max(1.0f + coeff * harmonicNum * harmonicNum, 0.05f);
            return harmonicNum * std::sqrt(inside);
        }
    } // namespace

    oscillator::ClassicWaveform OscPreviewSampler::waveformFromOrdinal(int ordinal) noexcept
    {
        switch (juce::jlimit(0, 3, ordinal))
        {
            case 0: return oscillator::ClassicWaveform::Sine;
            case 1: return oscillator::ClassicWaveform::Triangle;
            case 2: return oscillator::ClassicWaveform::Saw;
            default: return oscillator::ClassicWaveform::Square;
        }
    }

    noise::NoiseVariant OscPreviewSampler::noiseVariantFromOrdinal(int ordinal) noexcept
    {
        switch (juce::jlimit(0, 6, ordinal))
        {
            case 0: return noise::NoiseVariant::White;
            case 1: return noise::NoiseVariant::Pink;
            case 2: return noise::NoiseVariant::Brown;
            case 3: return noise::NoiseVariant::Blue;
            case 4: return noise::NoiseVariant::SampleAndHold;
            case 5: return noise::NoiseVariant::SmoothRandom;
            default: return noise::NoiseVariant::Dust;
        }
    }

    void OscPreviewSampler::sampleClassicCycle(const ClassicPreviewParams& params,
                                                std::array<float, kPreviewPoints>& out)
    {
        oscillator::ClassicOscillator osc;
        osc.prepare(kPreviewSampleRate);
        osc.setFrequency(static_cast<float>(kPreviewSampleRate / static_cast<double>(kPreviewPoints)));

        oscillator::ClassicOscillatorParams p;
        p.waveform = params.waveform;
        p.pulseWidth = params.pulseWidth;
        p.morph = -1.0f;
        osc.reset(0.0f);

        for (int i = 0; i < kPreviewPoints; ++i)
            out[static_cast<std::size_t>(i)] = osc.renderSample(p, 0.0f);
    }

    void OscPreviewSampler::sampleFmLayers(const FmPreviewParams& params, std::array<float, kPreviewPoints>& carrierOut,
                                            std::array<float, kPreviewPoints>& modOut)
    {
        oscillator::ClassicOscillator carrier;
        oscillator::ClassicOscillator modulator;
        carrier.prepare(kPreviewSampleRate);
        modulator.prepare(kPreviewSampleRate);

        const float baseHz = static_cast<float>(kPreviewSampleRate / static_cast<double>(kPreviewPoints));
        carrier.setFrequency(baseHz);
        modulator.setFrequency(baseHz * juce::jmax(0.25f, params.modRatio));

        oscillator::ClassicOscillatorParams cp;
        cp.waveform = params.carrier.waveform;
        cp.pulseWidth = params.carrier.pulseWidth;
        cp.morph = -1.0f;

        oscillator::ClassicOscillatorParams mp;
        mp.waveform = params.modulator.waveform;
        mp.pulseWidth = params.modulator.pulseWidth;
        mp.morph = -1.0f;

        carrier.reset(0.0f);
        modulator.reset(0.0f);

        const float indexScale = juce::jmap(juce::jlimit(0.0f, 10.0f, params.modIndex), 0.0f, 10.0f, 0.0f, 0.45f);

        for (int i = 0; i < kPreviewPoints; ++i)
        {
            const float mod = modulator.renderSample(mp, 0.0f);
            modOut[static_cast<std::size_t>(i)] = mod * juce::jmin(1.0f, params.modIndex * 0.15f);
            carrierOut[static_cast<std::size_t>(i)] = carrier.renderSample(cp, mod * indexScale);
        }
    }

    void OscPreviewSampler::samplePhaseShapeCycle(const PhaseShapePreviewParams& params,
                                                   std::array<float, kPreviewPoints>& outputOut,
                                                   std::array<float, kPreviewPoints>& warpOut)
    {
        oscillator::PhaseShapeParams pp;
        pp.phaseBend = params.phaseBend;
        pp.phaseFold = params.phaseFold;
        pp.phaseAsymmetry = params.phaseAsymmetry;
        pp.phaseShape = params.phaseShape;

        oscillator::PhaseShapeOscillator osc;
        osc.prepare(kPreviewSampleRate);
        osc.setFrequency(static_cast<float>(kPreviewSampleRate / static_cast<double>(kPreviewPoints)));
        osc.reset(0.0f);

        for (int i = 0; i < kPreviewPoints; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kPreviewPoints - 1);
            warpOut[static_cast<std::size_t>(i)] = warpPhasePreview(t, params) * 2.0f - 1.0f;
            outputOut[static_cast<std::size_t>(i)] = osc.renderSample(pp, 0.0f);
        }
    }

    void OscPreviewSampler::computeAdditiveHeights(const AdditivePreviewParams& params,
                                                    std::array<float, kMaxPreviewPartials>& heights, int& outCount)
    {
        const int count = std::clamp(params.partialCount, 1, kMaxPreviewPartials);
        outCount = count;
        const float tilt = pw8::dsp::clamp(params.tilt, -1.0f, 1.0f);
        const float oddEven = pw8::dsp::clamp(params.oddEven, 0.0f, 1.0f);
        const float tiltExponent = 1.0f + tilt;

        float maxH = 0.0f;
        for (int i = 0; i < count; ++i)
        {
            const int harmonicNum = i + 1;
            const bool isEven = (harmonicNum % 2) == 0;
            const float presence = isEven ? oddEven : 1.0f;
            const float amp = presence / std::pow(static_cast<float>(harmonicNum), tiltExponent);
            const float ratio = harmonicRatio(static_cast<float>(harmonicNum), params.stretch);
            const float stretchVisual = 1.0f / juce::jmax(1.0f, ratio / static_cast<float>(harmonicNum));
            heights[static_cast<std::size_t>(i)] = amp * stretchVisual;
            maxH = juce::jmax(maxH, heights[static_cast<std::size_t>(i)]);
        }
        if (maxH > 1.0e-6f)
        {
            for (int i = 0; i < count; ++i)
                heights[static_cast<std::size_t>(i)] /= maxH;
        }
    }

    void OscPreviewSampler::computeResonatorHeights(const ResonatorPreviewParams& params,
                                                     std::array<float, oscillator::ResonatorOscillator::kMaxModes>& heights,
                                                     int& outCount)
    {
        outCount = std::clamp(params.modeCount, 2, oscillator::ResonatorOscillator::kMaxModes);
        const float structure = pw8::dsp::clamp(params.structure, 0.0f, 1.0f);
        const float decay = pw8::dsp::clamp(params.decay, 0.0f, 1.0f);
        const float damping = pw8::dsp::clamp(params.damping, 0.0f, 1.0f);
        const float brightness = pw8::dsp::clamp(params.brightness, 0.0f, 1.0f);

        float maxH = 0.0f;
        for (int i = 0; i < outCount; ++i)
        {
            const float modeIndex = static_cast<float>(i + 1);
            const float detune = 1.0f + structure * (modeIndex - 1.0f) * 0.08f;
            const float dampFactor = 1.0f - damping * (modeIndex - 1.0f) / static_cast<float>(outCount);
            const float h = brightness * (0.35f + decay * 0.65f) * juce::jmax(0.15f, dampFactor) / detune;
            heights[static_cast<std::size_t>(i)] = h;
            maxH = juce::jmax(maxH, h);
        }
        if (maxH > 1.0e-6f)
        {
            for (int i = 0; i < outCount; ++i)
                heights[static_cast<std::size_t>(i)] /= maxH;
        }
    }

    void OscPreviewSampler::sampleNoiseTrace(const NoisePreviewParams& params, std::array<float, kNoisePoints>& out)
    {
        noise::NoiseSource source;
        source.prepare(kPreviewSampleRate);
        source.reset(params.seed);

        noise::NoiseSourceParams np;
        np.variant = params.variant;
        np.rateHz = juce::jmax(0.5f, params.rateHz);

        for (int i = 0; i < kNoisePoints; ++i)
            out[static_cast<std::size_t>(i)] = source.renderSample(np);
    }

} // namespace pw8::plugin::ui::wireframe
