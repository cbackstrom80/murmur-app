#pragma once

#include "pw8/filter/StateVariableFilter.hpp"

// Sub Anchor -- the first real implementation in pw8/spatial/ (previously an
// empty PLANNED directory, see this directory's own README: "low-frequency-mono"
// was already named there as a real, expected future capability before this
// existed). A frequency-selective mono lock: content below `crossoverHz` is
// blended toward a phase-coherent (L+R)/2 sum by `monoAmount`; content above
// the crossover passes through completely untouched.
//
// Real motivation: aggressive stereo-widening/distortion/chorus on a bass
// patch's character layer routinely smears its low end out of mono
// compatibility -- a real, well-known problem for club systems, vinyl
// cutting, and loudness-normalized streaming playback, all of which punish
// an incoherent sub. This gives a patch a real way to protect its low end
// without giving up movement/character above it.
//
// Built entirely on the existing, already-shipped StateVariableFilter --
// exploits the fact that its lowpass/highpass taps are an exact
// complementary pair (`highpass = input - lowpass`, the same identity
// CharacterFilter's own HP tap already relies on): summing the (possibly
// mono-blended) lowpass band back with the untouched `input - lowpass`
// remainder always exactly reconstructs the input when monoAmount is 0 --
// no separate "bypass" branch needed, the math *is* the bypass.
//
// Single filter stage per channel, deliberately -- real measurement (not
// assumption) during development found that naively cascading identical
// stages made real-world separation *worse*, not better: repeated
// identical-Q 2-pole sections don't approximate a proper Butterworth-
// aligned multi-pole crossover (that needs different Q per stage) and
// instead compound passband phase/magnitude deviation. A single stage
// with a mild fixed Q (`kAnchorResonance`, chosen empirically -- see
// SubAnchorTests.cpp) gives real, measurably better separation than either
// an undamped single stage or a naive cascade.
namespace pw8::spatial
{
    struct SubAnchorParams
    {
        bool enabled = false;
        /// Real crossover point separating the "anchored" sub band from
        /// everything above it. Typical bass crossover range.
        float crossoverHz = 120.0f;
        /// 0 = untouched (default, backward-compatible with every existing
        /// patch), 1 = the sub band's lowpass-tap content is forced fully
        /// mono. This is a crossover-based technique, not a brick-wall
        /// phase-lock: the recombined signal still carries each channel's
        /// own phase-shifted residual near the transition band (the same
        /// honest limitation every real "bass mono" tool has) -- real,
        /// measured behavior is a substantial reduction in L/R difference
        /// for genuinely sub-range content (well below crossover), not
        /// perfect equality right at the crossover. See SubAnchorTests.cpp
        /// for what's actually proven.
        float monoAmount = 0.0f;
    };

    class SubAnchor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            loL_.prepare(sampleRate);
            loR_.prepare(sampleRate);
        }

        void reset() noexcept
        {
            loL_.reset();
            loR_.reset();
        }

        /// Renders one stereo sample. `crossoverHz`/`monoAmount` may vary
        /// every call (mod-matrix-modulated in the future), matching every
        /// other real-time-modulatable primitive in this engine.
        void renderSample(float inL, float inR, float crossoverHz, float monoAmount, float& outL,
                          float& outR) noexcept
        {
            constexpr float kAnchorResonance = 0.263f; // ~Butterworth Q, chosen empirically
            const float loL = loL_.renderSample(inL, filter::FilterMode::Lowpass, crossoverHz, kAnchorResonance);
            const float loR = loR_.renderSample(inR, filter::FilterMode::Lowpass, crossoverHz, kAnchorResonance);
            const float mono = (loL + loR) * 0.5f;
            const float mixedLoL = loL + (mono - loL) * monoAmount;
            const float mixedLoR = loR + (mono - loR) * monoAmount;
            outL = mixedLoL + (inL - loL);
            outR = mixedLoR + (inR - loR);
        }

    private:
        filter::StateVariableFilter loL_;
        filter::StateVariableFilter loR_;
    };

} // namespace pw8::spatial
