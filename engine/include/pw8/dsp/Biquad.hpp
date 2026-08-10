#pragma once

#include <cmath>

#include "pw8/dsp/Math.hpp"

// A standard Direct-Form-I biquad, with coefficient formulas for the three shapes
// effects::Eq needs (low shelf, peaking/bell, high shelf) taken from Robert
// Bristow-Johnson's "Audio EQ Cookbook" -- the standard, openly-published
// reference every audio engineer reaches for (same citation category as this
// codebase's PolyBLEP oscillator and TPT state-variable filter: a generic,
// widely-documented technique, not vendored or reverse-engineered product code).
namespace pw8::dsp
{
    class Biquad
    {
    public:
        void reset() noexcept { x1_ = x2_ = y1_ = y2_ = 0.0f; }

        void setCoefficients(float b0, float b1, float b2, float a0, float a1, float a2) noexcept
        {
            const float invA0 = 1.0f / (std::abs(a0) > 1.0e-9f ? a0 : 1.0e-9f);
            b0_ = b0 * invA0;
            b1_ = b1 * invA0;
            b2_ = b2 * invA0;
            a1_ = a1 * invA0;
            a2_ = a2 * invA0;
        }

        [[nodiscard]] float renderSample(float x) noexcept
        {
            const float y = b0_ * x + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
            x2_ = x1_;
            x1_ = x;
            y2_ = y1_;
            y1_ = flushIfNotFinite(y);
            return y1_;
        }

        /// RBJ Audio EQ Cookbook "lowShelf": boosts/cuts everything below `freqHz`
        /// by `gainDb`, shelving flat above it. `slope` (the cookbook's `S`) fixed
        /// at 1.0 (the cookbook's "as steep as it can be without overshoot" case)
        /// -- effects::Eq doesn't expose shelf steepness as a separate parameter.
        void setLowShelf(float freqHz, float gainDb, double sampleRate) noexcept
        {
            const float a = std::pow(10.0f, gainDb / 40.0f);
            const float w0 = kTwoPi * freqHz / static_cast<float>(sampleRate);
            const float cosW0 = std::cos(w0);
            const float sinW0 = std::sin(w0);
            // alpha = (sin(w0)/2) * sqrt((A + 1/A)*(1/S - 1) + 2), with shelf slope S
            // fixed at 1 (the cookbook's steepest-without-overshoot case) -- the
            // (1/S - 1) term is then exactly 0, leaving sqrt(2).
            const float alpha = (sinW0 / 2.0f) * 1.41421356f; // sqrt(2).
            const float twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;

            const float b0 = a * ((a + 1.0f) - (a - 1.0f) * cosW0 + twoSqrtAAlpha);
            const float b1 = 2.0f * a * ((a - 1.0f) - (a + 1.0f) * cosW0);
            const float b2 = a * ((a + 1.0f) - (a - 1.0f) * cosW0 - twoSqrtAAlpha);
            const float a0 = (a + 1.0f) + (a - 1.0f) * cosW0 + twoSqrtAAlpha;
            const float a1 = -2.0f * ((a - 1.0f) + (a + 1.0f) * cosW0);
            const float a2 = (a + 1.0f) + (a - 1.0f) * cosW0 - twoSqrtAAlpha;
            setCoefficients(b0, b1, b2, a0, a1, a2);
        }

        /// RBJ Audio EQ Cookbook "highShelf" -- the low-shelf formula's mirror image.
        void setHighShelf(float freqHz, float gainDb, double sampleRate) noexcept
        {
            const float a = std::pow(10.0f, gainDb / 40.0f);
            const float w0 = kTwoPi * freqHz / static_cast<float>(sampleRate);
            const float cosW0 = std::cos(w0);
            const float sinW0 = std::sin(w0);
            // alpha = (sin(w0)/2) * sqrt((A + 1/A)*(1/S - 1) + 2), with shelf slope S
            // fixed at 1 (the cookbook's steepest-without-overshoot case) -- the
            // (1/S - 1) term is then exactly 0, leaving sqrt(2).
            const float alpha = (sinW0 / 2.0f) * 1.41421356f; // sqrt(2).
            const float twoSqrtAAlpha = 2.0f * std::sqrt(a) * alpha;

            const float b0 = a * ((a + 1.0f) + (a - 1.0f) * cosW0 + twoSqrtAAlpha);
            const float b1 = -2.0f * a * ((a - 1.0f) + (a + 1.0f) * cosW0);
            const float b2 = a * ((a + 1.0f) + (a - 1.0f) * cosW0 - twoSqrtAAlpha);
            const float a0 = (a + 1.0f) - (a - 1.0f) * cosW0 + twoSqrtAAlpha;
            const float a1 = 2.0f * ((a - 1.0f) - (a + 1.0f) * cosW0);
            const float a2 = (a + 1.0f) - (a - 1.0f) * cosW0 - twoSqrtAAlpha;
            setCoefficients(b0, b1, b2, a0, a1, a2);
        }

        /// RBJ Audio EQ Cookbook "peakingEQ" -- a symmetric bell boost/cut around
        /// `freqHz`, bandwidth controlled by `q`.
        void setPeaking(float freqHz, float gainDb, float q, double sampleRate) noexcept
        {
            const float a = std::pow(10.0f, gainDb / 40.0f);
            const float w0 = kTwoPi * freqHz / static_cast<float>(sampleRate);
            const float cosW0 = std::cos(w0);
            const float sinW0 = std::sin(w0);
            const float alpha = sinW0 / (2.0f * std::max(q, 0.01f));

            const float b0 = 1.0f + alpha * a;
            const float b1 = -2.0f * cosW0;
            const float b2 = 1.0f - alpha * a;
            const float a0 = 1.0f + alpha / a;
            const float a1 = -2.0f * cosW0;
            const float a2 = 1.0f - alpha / a;
            setCoefficients(b0, b1, b2, a0, a1, a2);
        }

    private:
        float b0_ = 1.0f, b1_ = 0.0f, b2_ = 0.0f, a1_ = 0.0f, a2_ = 0.0f;
        float x1_ = 0.0f, x2_ = 0.0f, y1_ = 0.0f, y2_ = 0.0f;
    };

} // namespace pw8::dsp
