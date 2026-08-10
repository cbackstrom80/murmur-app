#pragma once

#include <array>
#include <cmath>

#include "pw8/dsp/Math.hpp"

// Wideband analytic-signal generator and single-sideband frequency shifter.
//
// Technique: a finite-length, linear-phase FIR approximation of the ideal
// discrete Hilbert transform kernel h[n] = 2/(pi*n) for odd n, 0 for even n (the
// standard textbook derivation -- see e.g. Oppenheim & Schafer, "Discrete-Time
// Signal Processing," the Hilbert transformer section), tapered with a Hamming
// window to control truncation ripple. Because the kernel is antisymmetric about
// its center tap (a "Type III" linear-phase FIR), its group delay is exactly
// `kCenter` samples by construction -- provable directly from the filter's
// symmetry, not tuned by ear or borrowed from an unverified coefficient table.
// The companion "real" (in-phase) branch is simply the input delayed by that
// same `kCenter` samples, so the two branches stay exactly time-aligned. This is
// the standard real-time approach analog Bode frequency shifters and their
// digital descendants are built on (as researched in docs/FX_BANK.md's
// ValhallaFreqEcho notes) -- a generic, openly-documented DSP technique, not
// vendored or reverse-engineered code from any commercial plugin, in the same
// spirit as this codebase's PolyBLEP oscillator and TPT state-variable filter
// (both cite their technique's origin rather than a specific product).
namespace pw8::dsp
{
    class HilbertTransformer
    {
    public:
        static constexpr std::size_t kFirLength = 63; // odd, so there's a true center tap.
        static constexpr std::size_t kCenter = kFirLength / 2;

        HilbertTransformer() noexcept { computeTaps(); }

        void reset() noexcept
        {
            history_.fill(0.0f);
            writePos_ = 0;
        }

        /// Feeds one input sample and returns the analytic signal pair: `real` is
        /// the input delayed by the FIR's group delay, `imag` is its (windowed,
        /// finite-length) Hilbert transform -- the two are time-aligned by
        /// construction, so together they approximate the analytic signal
        /// x(t) + j*H{x(t)}.
        void process(float input, float& real, float& imag) noexcept
        {
            history_[writePos_] = input;

            float acc = 0.0f;
            std::size_t idx = writePos_;
            for (std::size_t k = 0; k < kFirLength; ++k)
            {
                acc += taps_[k] * history_[idx];
                idx = (idx == 0) ? (kFirLength - 1) : (idx - 1);
            }
            imag = flushIfNotFinite(acc);

            const std::size_t delayedIdx = (writePos_ + kFirLength - kCenter) % kFirLength;
            real = history_[delayedIdx];

            writePos_ = (writePos_ + 1) % kFirLength;
        }

    private:
        void computeTaps() noexcept
        {
            for (std::size_t k = 0; k < kFirLength; ++k)
            {
                const int n = static_cast<int>(k) - static_cast<int>(kCenter);
                float ideal = 0.0f;
                if (n != 0 && (n % 2) != 0)
                    ideal = 2.0f / (kPi * static_cast<float>(n));

                const float hamming =
                    0.54f - 0.46f * std::cos(kTwoPi * static_cast<float>(k) / static_cast<float>(kFirLength - 1));
                taps_[k] = ideal * hamming;
            }
        }

        std::array<float, kFirLength> taps_{};
        std::array<float, kFirLength> history_{};
        std::size_t writePos_ = 0;
    };

    /// Single-sideband frequency shifter built on `HilbertTransformer`. Shifts
    /// every frequency component of the input by a fixed `shiftHz` (positive
    /// shifts upward, negative downward) -- distinct from pitch shifting, which
    /// scales frequencies proportionally. See docs/FX_BANK.md for why this
    /// differs from a pitch shifter and how ValhallaFreqEcho's research informed
    /// placing this inside a feedback loop (effects::FreqShiftEcho).
    class FrequencyShifter
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        void reset() noexcept
        {
            hilbert_.reset();
            phase_ = 0.0f;
        }

        [[nodiscard]] float process(float input, float shiftHz) noexcept
        {
            float real = 0.0f, imag = 0.0f;
            hilbert_.process(input, real, imag);

            const float phaseInc = kTwoPi * shiftHz / static_cast<float>(sampleRate_);
            phase_ = wrapPhase((phase_ + phaseInc) / kTwoPi) * kTwoPi;

            // Single-sideband modulation: Re{ (real + j*imag) * e^{+j*phase} }
            // = real*cos(phase) - imag*sin(phase). With `real` approximating the
            // input and `imag` approximating its Hilbert transform (analytic
            // signal convention x_a = x + j*H{x}), this shifts the analytic
            // signal's instantaneous frequency *upward* by `shiftHz` when
            // positive -- e.g. a 440Hz tone with shiftHz=+200 becomes 640Hz,
            // verified via an FFT peak measurement in tests/unit/EffectsTests.cpp.
            const float c = std::cos(phase_);
            const float s = std::sin(phase_);
            return flushIfNotFinite(real * c - imag * s);
        }

    private:
        double sampleRate_ = 48000.0;
        HilbertTransformer hilbert_{};
        float phase_ = 0.0f;
    };

} // namespace pw8::dsp
