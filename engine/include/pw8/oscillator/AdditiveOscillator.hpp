#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"

// Engine Type 4 -- Additive.
//
// A vectorized oscillator bank (not independent oscillator objects per partial,
// docs/DSP_ENGINE.md's explicit target): each of up to kMaxPartials partials is a
// coupled-form recursive oscillator (x[n+1] = x*cosT - y*sinT; y[n+1] = x*sinT +
// y*cosT) -- O(1) per-sample update (4 mults, 2 adds), no sin()/cos() call in the
// inner render loop for the common "held note, no active pitch modulation" case.
// Rotation coefficients (cosT/sinT, one trig pair per partial) and per-partial
// amplitude weights are cached and only recomputed when the inputs that determine
// them actually change (carrier frequency, stretch, tilt, odd/even, partial count)
// -- recomputing unconditionally every sample would mean up to 128 transcendental
// calls/sample and defeat the point of the coupled-form technique.
//
// V1 scope (docs/DSP_ENGINE.md's fuller target also names formant, spectral blur,
// and partial randomization -- explicitly deferred, not silently dropped):
// tilt (spectral falloff), odd/even blend, and stretch (piano-string-style
// inharmonicity) via 4 fields total.
//
// Coupled-form recursion accumulates floating-point error over a long held note
// (a well-known property of this technique) -- periodically renormalized (every
// kRenormalizeInterval samples, rescaling each active partial's (x,y) back to unit
// magnitude) to stay bounded indefinitely, verified by a multi-second-hold test.

namespace pw8::oscillator
{
    struct AdditiveParams
    {
        /// How many of the kMaxPartials slots are active, 1..64.
        int partialCount = 32;
        /// Spectral tilt, -1..1. 0 == natural 1/n sawtooth-like falloff; -1 ==
        /// equal-amplitude (organ-like); +1 == steeper 1/n^2 falloff (darker).
        float tilt = 0.0f;
        /// 0 == odd harmonics only (clarinet-like), 1 == full harmonic series
        /// (odd and even equally present, saw-like). Even harmonics fade in
        /// linearly with this value; odd harmonics are always fully present.
        float oddEven = 0.5f;
        /// Inharmonicity, -1..1. 0 == exact harmonic series. Piano-string-style
        /// f_n = n * f0 * sqrt(1 + B*n^2), B swept by this value -- positive
        /// sharpens upper partials, negative flattens them.
        float stretch = 0.0f;
    };

    class AdditiveOscillator
    {
    public:
        static constexpr int kMaxPartials = 64;

        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0; }

        void reset(float initialPhase = 0.0f) noexcept
        {
            for (int i = 0; i < kMaxPartials; ++i)
            {
                const float harmonicNum = static_cast<float>(i + 1);
                const float theta = dsp::kTwoPi * dsp::wrapPhase(initialPhase * harmonicNum);
                x_[i] = std::cos(theta);
                y_[i] = std::sin(theta);
            }
            lastCarrierHz_ = -1.0f;   // forces a coefficient recompute on the next renderSample().
            lastStretch_ = -999.0f;
            lastTilt_ = -999.0f;
            lastOddEven_ = -999.0f;
            lastPartialCount_ = -1;
            sampleCounter_ = 0;
        }

        void setFrequency(float hz) noexcept { frequencyHz_ = hz; }

        /// Advance one sample and return the (normalized) sum in roughly [-1, 1].
        /// `externalPhaseModulation` is applied as a single uniform rotation shared
        /// by every partial (not scaled per-harmonic) -- a deliberate simplification
        /// for a graph-level PHASE_MOD edge landing on an Additive node: cheap (one
        /// extra sin/cos pair regardless of partial count) and always finite/bounded,
        /// at the cost of not being a physically exact per-partial phase shift.
        [[nodiscard]] float renderSample(const AdditiveParams& params, float externalPhaseModulation = 0.0f) noexcept
        {
            const int count = std::clamp(params.partialCount, 1, kMaxPartials);
            const float stretch = dsp::clamp(params.stretch, -1.0f, 1.0f);
            const float tilt = dsp::clamp(params.tilt, -1.0f, 1.0f);
            const float oddEven = dsp::clamp(params.oddEven, 0.0f, 1.0f);

            if (frequencyHz_ != lastCarrierHz_ || stretch != lastStretch_ || count != lastPartialCount_)
                recomputeRotationCoefficients(count, stretch);

            if (tilt != lastTilt_ || oddEven != lastOddEven_ || count != lastPartialCount_)
                recomputeAmplitudes(count, tilt, oddEven);

            lastPartialCount_ = count;

            float modCos = 1.0f, modSin = 0.0f;
            const bool hasPhaseMod = externalPhaseModulation != 0.0f;
            if (hasPhaseMod)
            {
                const float modTheta = dsp::kTwoPi * externalPhaseModulation;
                modCos = std::cos(modTheta);
                modSin = std::sin(modTheta);
            }

            float sum = 0.0f;
            for (int i = 0; i < count; ++i)
            {
                // Complex-multiply by this partial's per-sample rotation -- advances
                // its phase by one sample's worth without any trig call.
                const float nx = x_[i] * cosT_[i] - y_[i] * sinT_[i];
                const float ny = x_[i] * sinT_[i] + y_[i] * cosT_[i];
                x_[i] = nx;
                y_[i] = ny;

                const float sampleX = hasPhaseMod ? (x_[i] * modCos - y_[i] * modSin) : x_[i];
                sum += sampleX * amplitude_[i];
            }

            if (++sampleCounter_ >= kRenormalizeInterval)
            {
                sampleCounter_ = 0;
                for (int i = 0; i < count; ++i)
                {
                    const float mag = std::sqrt(x_[i] * x_[i] + y_[i] * y_[i]);
                    if (mag > 1.0e-9f)
                    {
                        x_[i] /= mag;
                        y_[i] /= mag;
                    }
                }
            }

            // Normalized by the summed partial weights at construction time (see
            // recomputeAmplitudes()) so partialCount/tilt/oddEven changes don't also
            // swing overall loudness -- otherwise adding partials would quietly get
            // louder/quieter depending on tilt, which isn't how any of these controls
            // are meant to read.
            return dsp::flushIfNotFinite(sum * weightNorm_);
        }

    private:
        static constexpr int kRenormalizeInterval = 512;

        void recomputeRotationCoefficients(int count, float stretch) noexcept
        {
            for (int i = 0; i < count; ++i)
            {
                const float harmonicNum = static_cast<float>(i + 1);
                const float ratio = harmonicRatio(harmonicNum, stretch);
                const float partialHz = frequencyHz_ * ratio;
                const float dTheta = dsp::kTwoPi * static_cast<float>(partialHz / sampleRate_);
                cosT_[i] = std::cos(dTheta);
                sinT_[i] = std::sin(dTheta);
            }
            lastCarrierHz_ = frequencyHz_;
            lastStretch_ = stretch;
        }

        void recomputeAmplitudes(int count, float tilt, float oddEven) noexcept
        {
            const float tiltExponent = 1.0f + tilt; // -1..1 -> exponent 0..2, see AdditiveParams::tilt doc.
            float weightSum = 0.0f;
            for (int i = 0; i < count; ++i)
            {
                const int harmonicNum = i + 1;
                const bool isEven = (harmonicNum % 2) == 0;
                const float presence = isEven ? oddEven : 1.0f;
                const float amp = presence / std::pow(static_cast<float>(harmonicNum), tiltExponent);
                amplitude_[i] = amp;
                weightSum += amp;
            }
            weightNorm_ = weightSum > 1.0e-6f ? 1.0f / weightSum : 0.0f;
            lastTilt_ = tilt;
            lastOddEven_ = oddEven;
        }

        [[nodiscard]] static float harmonicRatio(float harmonicNum, float stretch) noexcept
        {
            // See AdditiveParams::stretch. kMaxCoeff keeps sqrt's argument positive
            // for every harmonic up to kMaxPartials at stretch = -1.
            constexpr float kMaxCoeff = 0.0006f;
            const float coeff = stretch * kMaxCoeff;
            const float inside = std::max(1.0f + coeff * harmonicNum * harmonicNum, 0.05f);
            return harmonicNum * std::sqrt(inside);
        }

        double sampleRate_ = 48000.0;
        float frequencyHz_ = 440.0f;

        float lastCarrierHz_ = -1.0f;
        float lastStretch_ = -999.0f;
        float lastTilt_ = -999.0f;
        float lastOddEven_ = -999.0f;
        int lastPartialCount_ = -1;
        int sampleCounter_ = 0;
        float weightNorm_ = 0.0f;

        std::array<float, kMaxPartials> x_{};
        std::array<float, kMaxPartials> y_{};
        std::array<float, kMaxPartials> cosT_{};
        std::array<float, kMaxPartials> sinT_{};
        std::array<float, kMaxPartials> amplitude_{};
    };

} // namespace pw8::oscillator
