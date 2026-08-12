#pragma once

#include <array>
#include <cstdint>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/noise/NoiseSource.hpp"
#include "pw8/oscillator/AdditiveOscillator.hpp"
#include "pw8/oscillator/ClassicOscillator.hpp"
#include "pw8/oscillator/PhaseShapeOscillator.hpp"
#include "pw8/oscillator/ResonatorOscillator.hpp"

namespace pw8::plugin::ui::wireframe
{
    inline constexpr int kPreviewPoints = 48;
    inline constexpr int kNoisePoints = 96;
    inline constexpr int kMaxPreviewPartials = 32;

    struct ClassicPreviewParams
    {
        oscillator::ClassicWaveform waveform = oscillator::ClassicWaveform::Saw;
        float pulseWidth = 0.5f;
    };

    struct FmPreviewParams
    {
        ClassicPreviewParams carrier;
        ClassicPreviewParams modulator;
        float modRatio = 1.0f;
        float modIndex = 1.0f;
    };

    struct PhaseShapePreviewParams
    {
        float phaseBend = 0.0f;
        float phaseFold = 0.0f;
        float phaseAsymmetry = 0.0f;
        float phaseShape = 0.0f;
    };

    struct AdditivePreviewParams
    {
        int partialCount = 32;
        float tilt = 0.0f;
        float oddEven = 0.5f;
        float stretch = 0.0f;
    };

    struct ResonatorPreviewParams
    {
        float structure = 0.3f;
        float decay = 0.5f;
        float damping = 0.5f;
        float brightness = 0.5f;
        int modeCount = 6;
    };

    struct NoisePreviewParams
    {
        noise::NoiseVariant variant = noise::NoiseVariant::White;
        float rateHz = 10.0f;
        std::uint64_t seed = 1;
    };

    class OscPreviewSampler
    {
    public:
        static void sampleClassicCycle(const ClassicPreviewParams& params, std::array<float, kPreviewPoints>& out);

        static void sampleFmLayers(const FmPreviewParams& params, std::array<float, kPreviewPoints>& carrierOut,
                                    std::array<float, kPreviewPoints>& modOut);

        static void samplePhaseShapeCycle(const PhaseShapePreviewParams& params,
                                           std::array<float, kPreviewPoints>& outputOut,
                                           std::array<float, kPreviewPoints>& warpOut);

        static void computeAdditiveHeights(const AdditivePreviewParams& params,
                                            std::array<float, kMaxPreviewPartials>& heights, int& outCount);

        static void computeResonatorHeights(const ResonatorPreviewParams& params,
                                             std::array<float, oscillator::ResonatorOscillator::kMaxModes>& heights,
                                             int& outCount);

        static void sampleNoiseTrace(const NoisePreviewParams& params, std::array<float, kNoisePoints>& out);

        [[nodiscard]] static oscillator::ClassicWaveform waveformFromOrdinal(int ordinal) noexcept;
        [[nodiscard]] static noise::NoiseVariant noiseVariantFromOrdinal(int ordinal) noexcept;
    };

} // namespace pw8::plugin::ui::wireframe
