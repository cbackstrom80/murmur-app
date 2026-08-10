#pragma once

#include <cmath>

#include "pw8/dsp/Math.hpp"

// Filter 1 -- clean multimode synthesis filter (docs/DSP_ENGINE.md "Filter System").
//
// Topology: a trapezoidal-integrator state-variable filter (the "TPT SVF" family
// described by Vadim Zavalishin in "The Art of VA Filter Design" and, in the
// per-sample form used here, by Andrew Simper's widely-published SVF technical
// note). This is a well-established, openly-documented digital filter *technique*,
// not a specific product's circuit model or vendored code -- the implementation
// below is original, written from the standard trapezoidal-integrator derivation.
//
// Why this topology for Filter 1 specifically:
//   - It's unconditionally stable across the full cutoff range (no naive
//     integrator blow-up near Nyquist), which matters because cutoff will be
//     modulated per-sample by the mod matrix and can swing quickly.
//   - Low/high/band/notch/peak all fall out of the same two integrator states in
//     one pass, so switching mode costs nothing extra at runtime.
//   - It needs no oversampling to stay well-behaved at audio rates, unlike naive
//     one-pole-cascade designs pushed to high resonance.
//
// Nonlinear "character" filters (ladder-style, OTA-style, diode-style, saturated
// cascade -- Filter 2) are a separate, PLANNED topology family; see DSP_ENGINE.md.

namespace pw8::filter
{
    enum class FilterMode : std::uint8_t
    {
        Lowpass = 0,
        Highpass,
        Bandpass,
        Notch,
        Peak,
    };

    struct FilterParams
    {
        bool enabled = false;
        FilterMode mode = FilterMode::Lowpass;
        float cutoffHz = 8000.0f;
        /// 0 (no resonance, Butterworth-ish) .. 1 (near self-oscillation).
        float resonance = 0.2f;
        /// How much the cutoff tracks the voice's key-tracked note frequency, -1..1
        /// (0 = no tracking, 1 = fully tracks 1:1 with pitch).
        float keyTrack = 0.0f;
    };

    class StateVariableFilter
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        }

        void reset() noexcept
        {
            ic1eq_ = 0.0f;
            ic2eq_ = 0.0f;
        }

        /// Renders one sample. `cutoffHz` and `resonance01` may vary every call
        /// (mod-matrix-modulated); coefficients are recomputed each sample -- cheap
        /// (a handful of multiplies plus one tan()) relative to what a per-sample
        /// cutoff sweep costs anywhere else in the graph, and avoids zipper noise
        /// from stale coefficients far more reliably than a smoothed-parameter cache.
        [[nodiscard]] float renderSample(float input, FilterMode mode, float cutoffHz, float resonance01) noexcept
        {
            const float nyquist = static_cast<float>(sampleRate_) * 0.5f;
            const float fc = dsp::clamp(cutoffHz, 10.0f, nyquist * 0.98f);
            const float g = std::tan(dsp::kPi * fc / static_cast<float>(sampleRate_));

            // resonance01 in [0,1] -> twoR (twice the damping) in [1.9, 0.05]:
            // low resonance01 = heavily damped, high resonance01 = near self-oscillation.
            // Clamped away from 0 to keep the filter unconditionally stable.
            const float twoR = dsp::clamp(1.9f - 1.85f * dsp::clamp(resonance01, 0.0f, 1.0f), 0.05f, 1.9f);

            const float a1 = 1.0f / (1.0f + g * (g + twoR));
            const float a2 = g * a1;
            const float a3 = g * a2;

            const float v3 = input - ic2eq_;
            const float v1 = a1 * ic1eq_ + a2 * v3;
            const float v2 = ic2eq_ + a2 * ic1eq_ + a3 * v3;
            ic1eq_ = dsp::flushIfNotFinite(2.0f * v1 - ic1eq_);
            ic2eq_ = dsp::flushIfNotFinite(2.0f * v2 - ic2eq_);

            const float lowpass = v2;
            const float bandpass = v1;
            const float highpass = input - twoR * v1 - v2;
            const float notch = input - twoR * v1;
            const float peak = lowpass - highpass; // aka "bell"-ish band-emphasis response.

            switch (mode)
            {
                case FilterMode::Lowpass: return lowpass;
                case FilterMode::Highpass: return highpass;
                case FilterMode::Bandpass: return bandpass;
                case FilterMode::Notch: return notch;
                case FilterMode::Peak: return peak;
            }
            return lowpass;
        }

    private:
        double sampleRate_ = 48000.0;
        float ic1eq_ = 0.0f;
        float ic2eq_ = 0.0f;
    };

} // namespace pw8::filter
