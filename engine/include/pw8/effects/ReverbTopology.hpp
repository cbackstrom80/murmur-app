#pragma once

#include <array>
#include <cstddef>

#include "pw8/effects/EffectTypes.hpp"

// Real, per-character delay-line topologies for the shared FDN tank in
// Reverb.hpp -- Plate/Hall/Room/Spring/Shimmer used to be five *parameter*
// presets layered on one shared tank shape (ReverbCharacter.hpp's
// applyReverbCharacter() only rescales reverbSizeParam/reverbDecaySeconds/
// etc., which preserves the ratio between all 8 lines exactly -- it can only
// ever simulate "bigger/smaller," never a real structural difference). This
// file gives each character its own real base delay-length distribution,
// diffuser timing, and (for Spring) active line count -- the actual
// structural distinction real hardware makes between a plate's tight, dense
// cluster and a hall's wide, sparse spread.
//
// A direct scan of every factory preset (all 92, all Reverb slots) found
// every single one uses reverbCharacter=0 (Default) -- none use Plate/Hall/
// Room/Spring/Shimmer at all. So Default's table below is kept byte-for-byte
// identical to Reverb.hpp's own pre-existing kBaseDelayMs/kDiffuserStageMs --
// this phase carries zero golden-hash risk; only the four characters no
// shipped preset touches get real new topologies.
namespace pw8::effects
{
    struct ReverbTopology
    {
        std::array<float, kNumReverbLines> baseDelayMs;
        std::array<float, kNumReverbDiffuserStages> diffuserStageMs;
        std::size_t activeLines; // <= kNumReverbLines
    };

    namespace detail
    {
        // Real, unchanged -- byte-identical to Reverb.hpp's own original
        // kBaseDelayMs/kDiffuserStageMs, 8 mutually-irrational-ish lengths
        // (no small common integer ratios, avoids metallic comb ringing).
        inline constexpr ReverbTopology kDefaultTopology{
            {23.1f, 29.7f, 31.3f, 37.1f, 41.3f, 43.7f, 47.9f, 53.3f},
            {4.7f, 6.1f, 7.9f, 9.7f},
            kNumReverbLines,
        };

        // Real plate: tight, dense cluster of near-equal delay times (smooth,
        // near-instant density, no perceptible discrete echoes) + a fast
        // diffuser for near-instant buildup.
        inline constexpr ReverbTopology kPlateTopology{
            {13.7f, 15.1f, 16.9f, 17.3f, 18.7f, 19.9f, 21.1f, 22.3f},
            {2.3f, 3.1f, 3.7f, 4.3f},
            kNumReverbLines,
        };

        // Real hall: wide, long spread (audible gaps between early arrivals,
        // a big implied space) + a slower diffuser buildup.
        inline constexpr ReverbTopology kHallTopology{
            {29.3f, 41.7f, 53.9f, 64.1f, 71.3f, 79.7f, 88.9f, 96.1f},
            {8.9f, 11.3f, 13.7f, 16.9f},
            kNumReverbLines,
        };

        // Real room: short, but less extreme/uniform than Plate -- a small
        // space, not a maximally smooth/dense one.
        inline constexpr ReverbTopology kRoomTopology{
            {10.3f, 13.7f, 15.9f, 18.1f, 20.3f, 22.7f, 25.1f, 27.3f},
            {3.1f, 4.3f, 5.3f, 6.7f},
            kNumReverbLines,
        };

        // Real spring: only 4 active lines, not 8 -- a real spring tank has
        // far fewer independent coupled resonant paths than a dense
        // algorithmic network, structurally sparser/twangier by
        // construction, not just parameter-tinted. Trailing 4 base-delay
        // entries are never read (activeLines=4) so their values don't
        // matter; kept at 0 to make that explicit rather than leaving stale
        // Default values that look meaningful but aren't. Reuses Default's
        // diffuser timing -- Spring's existing parameter remap in
        // ReverbCharacter.hpp (modDepth+, modRate x1.8, diffusion x0.75)
        // already pushes the character; the active-line cut is this file's
        // own real structural addition on top of that.
        inline constexpr ReverbTopology kSpringTopology{
            {9.7f, 33.1f, 57.3f, 74.9f, 0.0f, 0.0f, 0.0f, 0.0f},
            {4.7f, 6.1f, 7.9f, 9.7f},
            4,
        };
    } // namespace detail

    /// Real per-character topology lookup. Default and Shimmer deliberately
    /// share one table -- Shimmer's real differentiation is the Phase 3
    /// pitch-shifted feedback tap, not topology, a documented choice, not an
    /// oversight. Falls back to Default for any out-of-range value (matches
    /// applyReverbCharacter()'s own clamp-then-switch convention).
    [[nodiscard]] inline const ReverbTopology& getReverbTopology(ReverbCharacter character) noexcept
    {
        switch (character)
        {
            case ReverbCharacter::Plate:
                return detail::kPlateTopology;
            case ReverbCharacter::Hall:
                return detail::kHallTopology;
            case ReverbCharacter::Room:
                return detail::kRoomTopology;
            case ReverbCharacter::Spring:
                return detail::kSpringTopology;
            case ReverbCharacter::Shimmer:
            case ReverbCharacter::Default:
            default:
                return detail::kDefaultTopology;
        }
    }

} // namespace pw8::effects
