#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "pw8/dsp/Biquad.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"

// Engine Type 8 -- Resonator / Spectral.
//
// A Karplus-Strong-adjacent, modal-synthesis design (docs/PRIOR_ART.md's Rings
// reference, docs/COMPETITIVE_ANALYSIS.md's Zebra-3 "modal resonators + comb
// filters, detailed feedback/damping control" comparison): an internal
// noise-burst exciter feeds a bank of up to kMaxModes parallel bandpass filters
// (dsp::Biquad::setBandpass(), a new primitive this engine needed -- see that
// method's own doc comment for why a true bandpass, not a high-gain peaking
// filter, is required for a modal bank). Each mode is tuned to a harmonic (or
// `structure`-detuned inharmonic) multiple of the carrier frequency; feeding a
// short burst into a resonant (high-Q) bandpass filter and letting it ring
// naturally IS the modal-synthesis technique -- no separate "ring" stage is
// needed once the filter itself is resonant enough.
//
// Self-contained (prepare/reset/setFrequency/renderSample), same shape as every
// other engine's oscillator component in this directory.

namespace pw8::oscillator
{
    struct ResonatorParams
    {
        /// Inharmonicity/detune of upper modes, 0..1. 0 == exact harmonic series;
        /// higher values progressively detune upper modes (bell/metallic character).
        float structure = 0.3f;
        /// Overall mode decay/ring time, 0..1. 0 == short buzzy pluck; 1 == a long,
        /// bell-like ring. Maps to each mode's filter Q.
        float decay = 0.5f;
        /// High-frequency damping tilt, 0..1. Higher values shorten upper modes'
        /// ring time relative to the fundamental's, matching real modal physics
        /// (a struck bell's upper partials always die out faster than its hum tone).
        float damping = 0.5f;
        /// Exciter noise burst's spectral tilt, 0..1. 0 == a smoothed/dark burst;
        /// 1 == full-spectrum white noise burst.
        float brightness = 0.5f;
        /// How many of the kMaxModes filter slots are active, 2..8.
        int modeCount = 6;
    };

    class ResonatorOscillator
    {
    public:
        static constexpr int kMaxModes = 8;

        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
            // exciterDecayCoeff_ is recomputed per-Q in recomputeModeFilters() now
            // (see kBurstSecondsMin/Max there) -- this just seeds a sane value
            // before the first renderSample() call establishes the real one.
            constexpr double kBurstSeconds = 0.006;
            exciterDecayCoeff_ = static_cast<float>(std::exp(-1.0 / (sampleRate_ * kBurstSeconds)));
        }

        /// `seed` should come from dsp::DeterministicRng::deriveSeed() at note-on
        /// (see Voice::noteOn / OperatorState::seedResonator()) -- never a
        /// free-running/time-based seed. Also re-triggers the exciter burst, which
        /// is the intended, documented side effect of an algorithm-graph Sync edge
        /// landing on a Resonator node (OperatorState::reset() calls this with a
        /// fixed, non-randomized seed for that path -- see its own doc comment).
        void reset(std::uint64_t seed) noexcept
        {
            rng_.reseed(seed);
            exciterEnvelope_ = 1.0f;
            brightState_ = 0.0f;
            for (auto& mode : modes_)
                mode.reset();
            lastCarrierHz_ = -1.0f; // forces a coefficient recompute on the next renderSample().
            lastStructure_ = -1.0f;
            lastDecay_ = -1.0f;
            lastDamping_ = -1.0f;
            lastModeCount_ = -1;
        }

        void setFrequency(float hz) noexcept { frequencyHz_ = hz; }

        [[nodiscard]] float renderSample(const ResonatorParams& params) noexcept
        {
            const int count = std::clamp(params.modeCount, 2, kMaxModes);
            const float structure = dsp::clamp(params.structure, 0.0f, 1.0f);
            const float decayParam = dsp::clamp(params.decay, 0.0f, 1.0f);
            const float damping = dsp::clamp(params.damping, 0.0f, 1.0f);
            const float brightness = dsp::clamp(params.brightness, 0.0f, 1.0f);

            if (frequencyHz_ != lastCarrierHz_ || structure != lastStructure_ || decayParam != lastDecay_ ||
                damping != lastDamping_ || count != lastModeCount_)
                recomputeModeFilters(count, structure, decayParam, damping);

            // Exciter: a brief decaying noise burst, spectrally tilted by
            // `brightness` (blend between a smoothed/dark version and the raw
            // white sample).
            const float whiteSample = rng_.nextRange(-1.0f, 1.0f);
            constexpr float kBrightSmoothCoeff = 0.25f;
            brightState_ += (whiteSample - brightState_) * kBrightSmoothCoeff;
            const float tilted = dsp::lerp(brightState_, whiteSample, brightness);
            const float excitation = tilted * exciterEnvelope_;
            exciterEnvelope_ *= exciterDecayCoeff_;

            float sum = 0.0f;
            for (int i = 0; i < count; ++i)
                sum += modes_[static_cast<std::size_t>(i)].renderSample(excitation) *
                       modeAmplitude_[static_cast<std::size_t>(i)];

            // Real output-level bug fix (found investigating systematic near-silent
            // renders on resonant/physically-modeled briefs -- confirmed via isolated
            // A/B render: this engine alone measured peak=0.0003 where an otherwise
            // identical patch with a masking second operator measured 0.137).
            // weightNorm_ only balances the *relative* mix between modes -- it was
            // never a real output-level calibration, so a filtered, transient-excited
            // noise burst reads far quieter than a continuous oscillator at the same
            // nominal level, and reads progressively quieter still as `decay` (Q)
            // rises -- a narrower passband converts less of a fixed-energy burst into
            // audible amplitude. Measured empirically (isolated resonator-only render,
            // burst-length fix already applied): peak fell from 0.117 at decay=0 to
            // 0.0086 at decay=1, and the ratio 0.117/peak(decay) tracks
            // `1 + decay*12.5` closely across the whole range (checked at 10 points).
            // kOutputMakeupGain (flat) plus this linear decay-dependent factor
            // together flatten output level to ~0.117 peak regardless of decay/Q, so
            // a long bell-like ring isn't quieter than a short buzzy pluck just
            // because of its physical-modeling character.
            constexpr float kOutputMakeupGain = 9.0f;
            const float decayCompensation = 1.0f + decayParam * 12.5f;
            return dsp::clamp(dsp::flushIfNotFinite(sum * weightNorm_ * kOutputMakeupGain * decayCompensation),
                               -4.0f, 4.0f);
        }

    private:
        void recomputeModeFilters(int count, float structure, float decayParam, float damping) noexcept
        {
            // Exponential Q mapping so `decay` reads naturally across its range:
            // a short buzzy pluck near 0, a long bell-like ring near 1.
            const float baseQ = 2.0f + decayParam * decayParam * 300.0f;
            const float nyquistGuard = static_cast<float>(sampleRate_) * 0.45f;

            // Real fix: the exciter burst was a fixed ~6ms regardless of Q. A
            // high-Q (long-decay) mode bank is a narrow-band filter -- its natural
            // ring-up time is proportionally longer than a low-Q one's, so a fixed
            // short burst barely excited it before decaying away (confirmed: peak
            // dropped from 0.014 at decay=0 to 0.0003 at decay=0.75 with the old
            // fixed burst). Scale the burst linearly with `decayParam` itself
            // (0..1, same knob the user already turns for "how long should this
            // ring") from a short percussive 6ms up to a full 80ms at decay=1 --
            // still well inside "burst that reads as a strike, not a drone", just
            // long enough to actually charge a narrow passband.
            constexpr float kBurstSecondsMin = 0.006f;
            constexpr float kBurstSecondsMax = 0.08f;
            const float burstSeconds = kBurstSecondsMin + decayParam * (kBurstSecondsMax - kBurstSecondsMin);
            exciterDecayCoeff_ = static_cast<float>(std::exp(-1.0 / (sampleRate_ * burstSeconds)));

            float weightSum = 0.0f;
            for (int i = 0; i < count; ++i)
            {
                const auto idx = static_cast<std::size_t>(i);
                const float harmonicNum = static_cast<float>(i + 1);
                const float ratio = modeRatio(harmonicNum, structure);
                const float modeHz = dsp::clamp(frequencyHz_ * ratio, 20.0f, nyquistGuard);

                // Damping tilts higher modes to decay faster (lower Q) -- matches
                // real modal physics (a struck bell's upper partials always die out
                // faster than its hum tone) and the Zebra-3 comparison's "detailed
                // ... damping control". (harmonicNum - 1) rather than harmonicNum
                // deliberately leaves the fundamental's own Q (and so its decay
                // time, alongside `decay`) completely untouched by this control --
                // damping only ever redistributes ring time away from upper modes
                // relative to a fixed fundamental, not a shared baseline that also
                // shifts. kDampingPerMode chosen so damping=1 meaningfully shortens
                // the highest mode's ring without silencing it outright.
                constexpr float kDampingPerMode = 0.35f;
                const float q = std::max(baseQ / (1.0f + damping * (harmonicNum - 1.0f) * kDampingPerMode), 0.6f);
                modes_[idx].setBandpass(modeHz, q, sampleRate_);

                const float amp = 1.0f / harmonicNum; // natural falloff, same neutral tilt Additive defaults to.
                modeAmplitude_[idx] = amp;
                weightSum += amp;
            }
            weightNorm_ = weightSum > 1.0e-6f ? 1.0f / weightSum : 0.0f;

            lastCarrierHz_ = frequencyHz_;
            lastStructure_ = structure;
            lastDecay_ = decayParam;
            lastDamping_ = damping;
            lastModeCount_ = count;
        }

        [[nodiscard]] static float modeRatio(float harmonicNum, float structure) noexcept
        {
            // Same piano-string-style inharmonicity shape oscillator::AdditiveOscillator
            // uses for its `stretch` field, but one-sided (structure is 0..1, not
            // -1..1 -- a resonator's modes only ever detune sharp, matching how a
            // struck real-world object's inharmonicity behaves).
            constexpr float kStructureCoeff = 0.015f;
            const float inside = 1.0f + structure * kStructureCoeff * harmonicNum * harmonicNum;
            return harmonicNum * std::sqrt(inside);
        }

        double sampleRate_ = 48000.0;
        float frequencyHz_ = 440.0f;

        dsp::DeterministicRng rng_;
        float exciterEnvelope_ = 0.0f;
        float exciterDecayCoeff_ = 0.999f;
        float brightState_ = 0.0f;

        float lastCarrierHz_ = -1.0f;
        float lastStructure_ = -1.0f;
        float lastDecay_ = -1.0f;
        float lastDamping_ = -1.0f;
        int lastModeCount_ = -1;
        float weightNorm_ = 0.0f;

        std::array<dsp::Biquad, kMaxModes> modes_{};
        std::array<float, kMaxModes> modeAmplitude_{};
    };

} // namespace pw8::oscillator
