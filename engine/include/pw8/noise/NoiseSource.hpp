#pragma once

#include <cmath>
#include <cstdint>

#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"

// Engine Type 7 -- Noise/Chaos.
//
// Scoped to 7 variants (docs/DSP_ENGINE.md's fuller taxonomy also names metallic/
// crackle/digital/full-chaotic modes -- deliberately deferred for a later pass, not
// silently dropped): White, Pink, Brown, Blue, Sample & Hold, Smooth Random, Dust.
//
// Every variant derives from one pw8::dsp::DeterministicRng stream (mandatory for
// anything touching the signal path -- docs/DSP_ENGINE.md "Determinism"), seeded
// once per note (see Voice::noteOn, which calls reset() with a
// DeterministicRng::deriveSeed()-derived value) rather than reseeded per-buffer or
// drawn from a free-running/time-based source, so the same note on the same seeded
// patch renders byte-identical noise every time.
//
// A self-contained component (prepare/reset/renderSample), same shape as
// oscillator::ClassicOscillator -- OperatorNode.hpp owns one instance per operator
// node and calls it from the NoiseChaos case in OperatorState::render().

namespace pw8::noise
{
    enum class NoiseVariant : std::uint8_t
    {
        White = 0,
        Pink = 1,
        Brown = 2,
        Blue = 3,
        SampleAndHold = 4,
        SmoothRandom = 5,
        Dust = 6,
    };

    struct NoiseSourceParams
    {
        NoiseVariant variant = NoiseVariant::White;
        /// Retarget rate in Hz, meaningful only for SampleAndHold/SmoothRandom/Dust
        /// -- ignored (but still present, matching every other engine's flat-params
        /// convention) by the continuous variants (White/Pink/Brown/Blue).
        float rateHz = 200.0f;
    };

    class NoiseSource
    {
    public:
        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate; }

        /// `seed` should come from dsp::DeterministicRng::deriveSeed() at note-on
        /// (see Voice::noteOn) -- never a free-running/time-based seed.
        void reset(std::uint64_t seed) noexcept
        {
            rng_.reseed(seed);
            for (auto& b : pinkState_)
                b = 0.0f;
            brownState_ = 0.0f;
            lastWhite_ = 0.0f;
            clockPhase_ = 0.0f;
            heldValue_ = 0.0f;
            rampFrom_ = 0.0f;
        }

        [[nodiscard]] float renderSample(const NoiseSourceParams& params) noexcept
        {
            switch (params.variant)
            {
                case NoiseVariant::White:
                    return whiteSample();
                case NoiseVariant::Pink:
                    return pinkSample();
                case NoiseVariant::Brown:
                    return brownSample();
                case NoiseVariant::Blue:
                    return blueSample();
                case NoiseVariant::SampleAndHold:
                    return sampleAndHoldSample(params.rateHz);
                case NoiseVariant::SmoothRandom:
                    return smoothRandomSample(params.rateHz);
                case NoiseVariant::Dust:
                    return dustSample(params.rateHz);
            }
            return 0.0f;
        }

    private:
        [[nodiscard]] float whiteSample() noexcept { return rng_.nextRange(-1.0f, 1.0f); }

        // Paul Kellet's refined pink noise filter (public domain, musicdsp.org) -- a
        // 7-pole IIR approximation of a -3dB/octave (1/f) spectral slope, the
        // standard cheap approach (vs. an FFT-domain 1/sqrt(f) shaping pass, which
        // would need a block buffer this per-sample signal path doesn't have).
        [[nodiscard]] float pinkSample() noexcept
        {
            const float white = whiteSample();
            pinkState_[0] = 0.99886f * pinkState_[0] + white * 0.0555179f;
            pinkState_[1] = 0.99332f * pinkState_[1] + white * 0.0750759f;
            pinkState_[2] = 0.96900f * pinkState_[2] + white * 0.1538520f;
            pinkState_[3] = 0.86650f * pinkState_[3] + white * 0.3104856f;
            pinkState_[4] = 0.55000f * pinkState_[4] + white * 0.5329522f;
            pinkState_[5] = -0.7616f * pinkState_[5] - white * 0.0168980f;
            const float pink = pinkState_[0] + pinkState_[1] + pinkState_[2] + pinkState_[3] + pinkState_[4] +
                                pinkState_[5] + pinkState_[6] + white * 0.5362f;
            pinkState_[6] = white * 0.115926f;
            return dsp::flushIfNotFinite(pink * 0.11f); // Restores roughly +-1 full scale after the summed gains.
        }

        // Leaky-integrated white noise (public domain, musicdsp.org "brown noise",
        // also Paul Kellet) -- a true (non-leaky) random-walk integral of white noise
        // is unbounded, so the leak term is what keeps this finite on an indefinitely
        // held note, not an afterthought clamp bolted on top.
        [[nodiscard]] float brownSample() noexcept
        {
            const float white = whiteSample();
            brownState_ = (brownState_ + (0.02f * white)) / 1.02f;
            return dsp::clamp(brownState_ * 3.5f, -1.0f, 1.0f); // 3.5x restores full-scale amplitude after the leak.
        }

        // First difference of white noise -- a single-zero highpass, +3dB/octave,
        // the mirror-image complement of the brown/red leaky-integrator lowpass above.
        [[nodiscard]] float blueSample() noexcept
        {
            const float white = whiteSample();
            const float blue = (white - lastWhite_) * 0.5f;
            lastWhite_ = white;
            return blue;
        }

        /// Advances the shared retarget clock; returns true exactly once per
        /// 1/rateHz seconds (phase-accumulator pattern, same technique lfo::Lfo
        /// already uses for its own free-running phase).
        [[nodiscard]] bool advanceClock(float rateHz) noexcept
        {
            const float rate = dsp::clamp(rateHz, 0.01f, 20000.0f);
            clockPhase_ += rate / static_cast<float>(sampleRate_);
            if (clockPhase_ >= 1.0f)
            {
                clockPhase_ -= std::floor(clockPhase_);
                return true;
            }
            return false;
        }

        [[nodiscard]] float sampleAndHoldSample(float rateHz) noexcept
        {
            if (advanceClock(rateHz))
                heldValue_ = whiteSample();
            return heldValue_;
        }

        [[nodiscard]] float smoothRandomSample(float rateHz) noexcept
        {
            if (advanceClock(rateHz))
            {
                rampFrom_ = heldValue_;
                heldValue_ = whiteSample();
            }
            // clockPhase_ is already the 0..1 fraction through the current ramp
            // segment (the wrapped remainder advanceClock() carries over), so it
            // doubles as the interpolation fraction with no separate counter needed.
            return rampFrom_ + (heldValue_ - rampFrom_) * clockPhase_;
        }

        // "Dust": sparse random impulses. Per-sample Bernoulli trial with
        // probability rateHz/sampleRate gives an expected impulse rate of rateHz/sec
        // without needing a phase accumulator -- the standard "dust" opcode design
        // (e.g. Csound's `dust`), simpler than a clock-based retarget since an
        // impulse train has no "current value" to hold between events.
        [[nodiscard]] float dustSample(float rateHz) noexcept
        {
            const float rate = dsp::clamp(rateHz, 0.01f, 20000.0f);
            const float probability = rate / static_cast<float>(sampleRate_);
            if (rng_.nextFloat() < probability)
                return whiteSample();
            return 0.0f;
        }

        dsp::DeterministicRng rng_;
        double sampleRate_ = 48000.0;

        float pinkState_[7] = {};
        float brownState_ = 0.0f;
        float lastWhite_ = 0.0f;

        float clockPhase_ = 0.0f;
        float heldValue_ = 0.0f;
        float rampFrom_ = 0.0f;
    };

} // namespace pw8::noise
