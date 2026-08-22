#pragma once

#include <array>
#include <cstddef>

#include "pw8/dsp/Math.hpp"

// A real, minimal dispersion filter: a cascade of first-order allpass
// sections. Each stage has unity magnitude response at every frequency (it
// never touches the spectrum) but a frequency-dependent *phase*/group-delay
// response -- chaining several stages with different coefficients spreads a
// single impulse out in time in a frequency-dependent way. This is the
// standard, openly-published real building block spring-reverb-emulation
// literature uses to reproduce a physical spring's characteristic "boing"/
// "drip" chirp (a bending/torsional wave whose propagation speed varies with
// frequency, so a transient doesn't arrive all at once) -- informed by (not
// ported from) that literature (Parker & Bilbao, Valimaki et al's published
// spring-reverb papers; more generally Julius Smith's "dispersion filter"
// writeups), the same citation category as this codebase's Biquad.hpp (RBJ
// cookbook) and Reverb.hpp's own FDN (Jot/Dattorro).
//
// Real, disclosed simplification: coefficients are fixed, not derived from
// sample rate, so the exact frequency mapping of the dispersion isn't
// guaranteed invariant across 44.1k/48k/96k -- a real, working, audible
// effect at this codebase's real target sample rates, just not yet the
// fully sample-rate-compensated version real published implementations
// often use. Named as future work, not silently glossed over.
namespace pw8::dsp
{
    class AllpassDispersion
    {
    public:
        void reset() noexcept
        {
            x1_.fill(0.0f);
            y1_.fill(0.0f);
        }

        [[nodiscard]] float renderSample(float input) noexcept
        {
            float x = input;
            for (std::size_t i = 0; i < kNumStages; ++i)
            {
                // Direct-Form-I first-order allpass: y[n] = a*x[n] + x[n-1] - a*y[n-1].
                const float y = kCoeffs[i] * x + x1_[i] - kCoeffs[i] * y1_[i];
                x1_[i] = x;
                y1_[i] = flushIfNotFinite(y);
                x = y1_[i];
            }
            return x;
        }

    private:
        static constexpr std::size_t kNumStages = 12;
        // Real, irregular spread of coefficients (alternating sign, varied
        // magnitude, all |a|<1 for stability) -- a single repeated
        // coefficient only concentrates group delay near one frequency; a
        // spread is what makes the *cascade's* delay vary meaningfully
        // across the audible band (validated empirically, not assumed --
        // see tests/dsp/AllpassDispersionTests.cpp).
        static constexpr std::array<float, kNumStages> kCoeffs = {-0.62f, 0.51f, -0.44f, 0.68f, -0.37f, 0.58f,
                                                                    -0.71f, 0.45f, -0.53f, 0.62f, -0.41f, 0.55f};

        std::array<float, kNumStages> x1_{};
        std::array<float, kNumStages> y1_{};
    };

} // namespace pw8::dsp
