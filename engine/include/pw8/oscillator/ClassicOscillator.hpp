#pragma once

#include <cstdint>

#include "pw8/dsp/Math.hpp"

// Engine Type 1 -- Classic.
//
// Band-limiting strategy: PolyBLEP (polynomial band-limited step).
//
// Rationale (see docs/DSP_ENGINE.md for the full writeup):
//   - Naive discontinuous saw/square generation aliases badly at anything above a
//     few hundred Hz and is unacceptable for a flagship product.
//   - True minBLEP/BLIT tables give slightly better high-frequency behaviour but
//     require table storage, wrap-around convolution bookkeeping and are harder to
//     keep numerically simple under heavy pitch modulation (FM/PM feeding this
//     oscillator's frequency every sample).
//   - PolyBLEP is a small, allocation-free, per-sample-cost, well-understood
//     technique that only needs the current phase and phase increment -- which
//     makes it trivial to keep correct under aggressive per-sample pitch modulation
//     from the algorithm graph. That per-sample robustness under modulation is the
///     deciding factor for engine type 1.
//   - Triangle is derived by leaky-integrating a PolyBLEP square wave. This is a
//     standard trick: integrating a band-limited square removes the value-discontinuity
//     aliasing entirely (the slope discontinuities that remain are far less audible
//     and are not further corrected in this pass -- a polyBLAMP slope correction is
//     tracked as a documented future refinement, not required for MVP quality bar).
//
// This oscillator is used both standalone (waveform-only patches) and as the
// building block inside the 8-node algorithm graph, where its frequency and phase
// may be modulated per-sample by other nodes (PM/FM/sync edges).

namespace pw8::oscillator
{
    enum class ClassicWaveform : std::uint8_t
    {
        Sine = 0,
        Triangle = 1,
        Saw = 2,
        Square = 3,
    };

    /// PolyBLEP correction, applied at a discontinuity crossed at normalized phase `t`
    /// with phase increment `dt` (both in cycles, i.e. dt = freq / sampleRate).
    [[nodiscard]] inline float polyBlep(float t, float dt) noexcept
    {
        if (dt <= 0.0f)
            return 0.0f;

        if (t < dt)
        {
            const float x = t / dt;
            return x + x - x * x - 1.0f;
        }
        if (t > 1.0f - dt)
        {
            const float x = (t - 1.0f) / dt;
            return x * x + x + x + 1.0f;
        }
        return 0.0f;
    }

    struct ClassicOscillatorParams
    {
        ClassicWaveform waveform = ClassicWaveform::Saw;
        /// Continuous morph across the ordered shape sequence [Sine, Triangle, Saw, Square],
        /// 0..1 across the whole sequence. When morph > 0, this overrides `waveform` and
        /// crossfades between the two neighbouring shapes. Set morph < 0 to disable and use
        /// `waveform` verbatim (the common case for a simple non-morphing patch).
        float morph = -1.0f;
        /// Pulse width for Square/Pulse shapes, 0..1 (0.5 = symmetric square).
        float pulseWidth = 0.5f;
    };

    class ClassicOscillator
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        /// Reset phase (e.g. on voice start, or when a sync edge fires in the algorithm graph).
        void reset(float initialPhase = 0.0f) noexcept
        {
            phase_ = dsp::wrapPhase(initialPhase);
            triangleIntegratorState_ = 0.0f;
        }

        void setFrequency(float hz) noexcept
        {
            frequencyHz_ = hz;
        }

        /// Advance one sample and return the oscillator output in [-1, 1].
        /// `externalPhaseModulation` is an additional phase offset in cycles, applied
        /// this sample only (used for PHASE_MOD algorithm-graph edges); pass 0 for
        /// unmodulated use.
        [[nodiscard]] float renderSample(const ClassicOscillatorParams& params,
                                          float externalPhaseModulation = 0.0f) noexcept
        {
            const float dt = static_cast<float>(frequencyHz_ / sampleRate_);
            const float readPhase = dsp::wrapPhase(phase_ + externalPhaseModulation);

            float out = (params.morph >= 0.0f)
                            ? renderMorphed(readPhase, dt, params)
                            : renderShape(params.waveform, readPhase, dt, params.pulseWidth);

            const float phaseBefore = phase_;
            phase_ = dsp::wrapPhase(phase_ + dt);
            didWrapThisSample_ = phase_ < phaseBefore;
            return out;
        }

        [[nodiscard]] float getPhase() const noexcept { return phase_; }
        [[nodiscard]] bool didWrapThisSample() const noexcept { return didWrapThisSample_; }

    private:
        [[nodiscard]] float renderShape(ClassicWaveform shape, float t, float dt, float pulseWidth) noexcept
        {
            switch (shape)
            {
                case ClassicWaveform::Sine:
                    return renderSine(t);
                case ClassicWaveform::Triangle:
                    return renderTriangle(t, dt);
                case ClassicWaveform::Saw:
                    return renderSaw(t, dt);
                case ClassicWaveform::Square:
                    return renderSquare(t, dt, pulseWidth);
            }
            return 0.0f;
        }

        [[nodiscard]] float renderMorphed(float t, float dt, const ClassicOscillatorParams& params) noexcept
        {
            // Ordered sequence: Sine(0) -> Triangle(1/3) -> Saw(2/3) -> Square(1).
            const float m = dsp::clamp(params.morph, 0.0f, 1.0f) * 3.0f;
            const int segment = static_cast<int>(m);
            const float frac = m - static_cast<float>(segment);

            const ClassicWaveform shapes[4] = {
                ClassicWaveform::Sine, ClassicWaveform::Triangle, ClassicWaveform::Saw, ClassicWaveform::Square};

            const int a = dsp::clamp(segment, 0, 3);
            const int b = dsp::clamp(segment + 1, 0, 3);

            if (a == b)
                return renderShape(shapes[a], t, dt, params.pulseWidth);

            // NOTE: this calls renderShape twice per sample for the triangle integrator's
            // sake, which duplicates internal integrator state advance. To keep the
            // integrator consistent we only advance it once (in the "primary" shape call)
            // and treat the secondary call as a stateless probe when it's Triangle.
            const float sa = renderShapeStateless(shapes[a], t, dt, params.pulseWidth);
            const float sb = renderShapeStateless(shapes[b], t, dt, params.pulseWidth);
            return dsp::lerp(sa, sb, frac);
        }

        // Stateless variants used only for morph blending, so the triangle leaky
        // integrator (which has real per-sample memory) isn't double-advanced.
        [[nodiscard]] float renderShapeStateless(ClassicWaveform shape, float t, float dt, float pulseWidth) noexcept
        {
            switch (shape)
            {
                case ClassicWaveform::Sine:
                    return renderSine(t);
                case ClassicWaveform::Triangle:
                    return approximateTriangleStateless(t);
                case ClassicWaveform::Saw:
                    return renderSaw(t, dt);
                case ClassicWaveform::Square:
                    return renderSquare(t, dt, pulseWidth);
            }
            return 0.0f;
        }

        [[nodiscard]] static float renderSine(float t) noexcept
        {
            return std::sin(dsp::kTwoPi * t);
        }

        [[nodiscard]] static float renderSaw(float t, float dt) noexcept
        {
            float v = 2.0f * t - 1.0f;
            v -= polyBlep(t, dt);
            return v;
        }

        [[nodiscard]] static float renderSquare(float t, float dt, float pulseWidth) noexcept
        {
            const float pw = dsp::clamp(pulseWidth, 0.01f, 0.99f);
            float v = (t < pw) ? 1.0f : -1.0f;
            v += polyBlep(t, dt);
            const float t2 = dsp::wrapPhase(t - pw + 1.0f);
            v -= polyBlep(t2, dt);
            return v;
        }

        // Leaky-integrated PolyBLEP square, stateful version used in normal playback.
        [[nodiscard]] float renderTriangle(float t, float dt) noexcept
        {
            const float square = renderSquare(t, dt, 0.5f);
            // Scale so the integrated triangle reaches roughly +/-1: the integration
            // gain depends on frequency, so normalize by dt (a standard leaky-integrator
            // triangle trick -- higher frequency means faster leak-corrected settling).
            const float scaled = square * dt * 4.0f;
            triangleIntegratorState_ = triangleIntegratorState_ * kTriangleLeak + scaled;
            return triangleIntegratorState_;
        }

        // Approximate closed-form triangle (analytic, no integrator state) used only
        // as one side of a morph blend so we don't perturb the real integrator state.
        [[nodiscard]] static float approximateTriangleStateless(float t) noexcept
        {
            return 4.0f * std::abs(t - 0.5f) - 1.0f;
        }

        static constexpr float kTriangleLeak = 0.9995f;

        double sampleRate_ = 48000.0;
        float frequencyHz_ = 440.0f;
        float phase_ = 0.0f;
        float triangleIntegratorState_ = 0.0f;
        bool didWrapThisSample_ = false;
    };

} // namespace pw8::oscillator
