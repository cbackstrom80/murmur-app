#pragma once

#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"

// Engine Type 5 -- Phase / Shape.
//
// CZ-style phase distortion (Casio CZ "DCW" -- Distance of Closest Window): instead
// of generating a waveform directly, the oscillator's normally-linear read phase is
// warped through a shaping curve before a sine table lookup, which pushes harmonic
// energy into the signal without ever computing/summing partials directly. Followed
// by a post-generation wavefold stage (docs/DSP_ENGINE.md's "Warp" taxonomy already
// named bend/fold/asymmetry/shape as the target field list; this is the
// implementation of that taxonomy).
//
// Scope for this pass: a smooth (not piecewise-linear) sinusoidal phase warp rather
// than the CZ hardware's original breakpoint/segment tables -- a sinusoidal warp has
// no slope discontinuity to alias on its own (the wavefold stage still can, which is
// why it gets its own light 2x-oversampling below), and two warp curves (single-
// and double-cycle) already cover a useful "bright saw-like" to "formant-like"
// range via `phaseShape`'s blend between them. A true multi-segment DCW table is a
// documented possible follow-up, not required for this pass's scope.
//
// Wavefold: `asin(sin(x))`-style smooth reflection -- the same cheap, bounded,
// iteration-free trick used elsewhere in this codebase for smooth folding. Real
// wavefolding generates harmonics that can alias (docs/DSP_ENGINE.md "selective
// oversampling around the nonlinear stages"); this implements the pragmatic v1 that
// doc calls for: linear-interpolated 2x oversampling specifically around the fold
// stage, downsampled with a one-pole smoother, rather than a full polyphase halfband
// filter -- noted there as a real follow-up if the cheap version's aliasing proves
// audible under test.

namespace pw8::oscillator
{
    struct PhaseShapeParams
    {
        /// CZ-style warp amount, -1..1. 0 == undistorted (pure sine read). Sign
        /// mirrors which half of the cycle the read phase accelerates through.
        float phaseBend = 0.0f;
        /// Post-generation wavefold amount, 0..1. 0 == no folding (transparent).
        float phaseFold = 0.0f;
        /// Warp-curve skew, -1..1. Biases the warp toward the first or second half
        /// of the cycle -- covers "bias"/DC-skew too, a separate field for that
        /// would cover near-identical ground.
        float phaseAsymmetry = 0.0f;
        /// Blend between the single-cycle ("bright saw-like") and double-cycle
        /// ("formant-like") warp curves, 0..1.
        float phaseShape = 0.0f;
    };

    class PhaseShapeOscillator
    {
    public:
        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0; }

        void reset(float initialPhase = 0.0f) noexcept
        {
            phase_ = dsp::wrapPhase(initialPhase);
            prevCarrierRaw_ = 0.0f;
            foldSmoothState_ = 0.0f;
        }

        void setFrequency(float hz) noexcept { frequencyHz_ = hz; }

        /// Advance one sample and return the output in [-1, 1]. `externalPhaseModulation`
        /// is an additional phase offset in cycles (algorithm-graph PHASE_MOD edges).
        [[nodiscard]] float renderSample(const PhaseShapeParams& params, float externalPhaseModulation = 0.0f,
                                          int osFactor = 2) noexcept
        {
            const float dt = static_cast<float>(frequencyHz_ / sampleRate_);
            const float t = dsp::wrapPhase(phase_ + externalPhaseModulation);

            const float warpedT = warpPhase(t, params);
            const float carrierRaw = std::sin(dsp::kTwoPi * warpedT);

            const float out = applyFold(carrierRaw, params.phaseFold, osFactor);

            const float phaseBefore = phase_;
            phase_ = dsp::wrapPhase(phase_ + dt);
            didWrapThisSample_ = phase_ < phaseBefore;
            return out;
        }

        [[nodiscard]] bool didWrapThisSample() const noexcept { return didWrapThisSample_; }

    private:
        [[nodiscard]] static float warpPhase(float t, const PhaseShapeParams& params) noexcept
        {
            // Curve A: single-cycle sinusoidal warp -- accelerates the read phase
            // through one half of the cycle and decelerates it through the other,
            // the smooth analogue of CZ's "saw" DCW family.
            const float curveA = std::sin(dsp::kTwoPi * t);
            // Curve B: double-cycle sinusoidal warp -- folds a second oscillation
            // into the cycle, closer to CZ's "resonant" DCW family (formant-like).
            const float curveB = std::sin(2.0f * dsp::kTwoPi * t);
            const float shape = dsp::clamp(params.phaseShape, 0.0f, 1.0f);
            const float base = dsp::lerp(curveA, curveB, shape);

            // Asymmetry biases the warp magnitude toward one half of the cycle
            // (cos(2*pi*t) is +1 at t=0, -1 at t=0.5) rather than shifting it
            // uniformly, which is what makes it read as a skew rather than a bend.
            const float asymmetry = dsp::clamp(params.phaseAsymmetry, -1.0f, 1.0f);
            const float skewed = base * (1.0f + asymmetry * std::cos(dsp::kTwoPi * t));

            const float bend = dsp::clamp(params.phaseBend, -1.0f, 1.0f);
            // kWarpDepth keeps the maximum phase displacement well under a full
            // cycle even at bend=+-1 and asymmetry=+-1 (worst case ~2x curve
            // amplitude), so the warped read phase never wraps more than once
            // relative to the unwarped phase in a single sample.
            constexpr float kWarpDepth = 0.2f;
            return dsp::wrapPhase(t + bend * skewed * kWarpDepth);
        }

        // Smooth "reflect back into range" wavefolder (asin(sin(...))) -- at
        // amount=0 this is the identity for any x in [-1,1] (asin(sin(theta)) ==
        // theta for theta in [-pi/2, pi/2], and x*pi/2 stays in that range whenever
        // |x| <= 1), so the fold stage is fully transparent until dialed in.
        [[nodiscard]] static float foldShape(float x, float amount) noexcept
        {
            const float drive = 1.0f + dsp::clamp(amount, 0.0f, 1.0f) * 5.0f;
            const float driven = dsp::clamp(x * drive, -32.0f, 32.0f); // guard asin()'s input range below.
            return std::asin(std::sin(driven * dsp::kPi * 0.5f)) * (2.0f / dsp::kPi);
        }

        /// Applies foldShape() at 2x oversampling (linear-interpolated midpoint
        /// between this sample's and the previous sample's pre-fold carrier,
        /// folded independently, then averaged back down -- a cheap boxcar
        /// downsample) followed by a light one-pole smoother, per this file's
        /// class doc comment on why this is the pragmatic v1 rather than a full
        /// polyphase halfband filter.
        [[nodiscard]] float applyFold(float carrierRaw, float amount, int osFactor) noexcept
        {
            const float direct = foldShape(carrierRaw, amount);
            if (osFactor <= 1 || amount <= 0.0f)
            {
                prevCarrierRaw_ = carrierRaw;
                return dsp::flushIfNotFinite(direct);
            }

            if (osFactor == 2)
            {
                const float midCarrier = dsp::lerp(prevCarrierRaw_, carrierRaw, 0.5f);
                const float y0 = foldShape(prevCarrierRaw_, amount);
                const float y1 = foldShape(midCarrier, amount);
                const float y2 = direct;
                const float combined = dsp::halfBandDecimate3Tap(y0, y1, y2);
                const float kSmoothCoeff = 0.35f;
                foldSmoothState_ += (combined - foldSmoothState_) * kSmoothCoeff;
                prevCarrierRaw_ = carrierRaw;
                return dsp::flushIfNotFinite(foldSmoothState_);
            }

            const float c1 = dsp::lerp(prevCarrierRaw_, carrierRaw, 0.25f);
            const float c2 = dsp::lerp(prevCarrierRaw_, carrierRaw, 0.5f);
            const float c3 = dsp::lerp(prevCarrierRaw_, carrierRaw, 0.75f);
            const float interior[3] = {foldShape(c1, amount), foldShape(c2, amount), foldShape(c3, amount)};
            const float combined =
                dsp::decimateHalfBandFold(foldShape(prevCarrierRaw_, amount), direct, osFactor, interior);
            const float kSmoothCoeff = osFactor >= 4 ? 0.25f : 0.35f;
            foldSmoothState_ += (combined - foldSmoothState_) * kSmoothCoeff;
            prevCarrierRaw_ = carrierRaw;
            return dsp::flushIfNotFinite(foldSmoothState_);
        }

        double sampleRate_ = 48000.0;
        float frequencyHz_ = 440.0f;
        float phase_ = 0.0f;
        float prevCarrierRaw_ = 0.0f;
        float foldSmoothState_ = 0.0f;
        bool didWrapThisSample_ = false;
    };

} // namespace pw8::oscillator
