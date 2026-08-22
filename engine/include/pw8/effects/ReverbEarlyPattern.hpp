#pragma once

#include <array>
#include <cstddef>

#include "pw8/effects/EffectTypes.hpp"

// Real, per-character early-reflection patterns for the algorithmic reverb's
// discrete early-tap cluster (the "Early Select" idea from Fathom's own
// backlog: the algorithmic engine's early reflections, distinct from Hybrid
// mode's IR-based early reflections). Companion to ReverbTopology.hpp, which
// did the same real per-character differentiation for the *late* tank --
// this file does it for the early cluster, keyed off the same
// `reverbCharacter` selector rather than adding a new param, so a character
// choice shapes the space's whole early+late signature together instead of
// only its tail.
//
// Real acoustic motivation per character (a room's early-reflection pattern
// is one of the strongest real perceptual cues for its size/type, distinct
// from its late decay time):
//   - A big hall: longer first-reflection delay (sound travels further to
//     the first wall) and a sparser, more widely time-spaced pattern.
//   - A small room: short first-reflection delay but a *denser* flurry of
//     close reflections (many nearby boundaries).
//   - A plate: not a real 3D space at all -- minimal/near-absent discrete
//     early reflections, blending almost immediately into the dense tank
//     (which ReverbTopology.hpp already made fast/dense for Plate).
//   - A spring tank: essentially no discrete "room" early cluster -- the
//     signal goes nearly straight into the sparse resonant tail.
//
// Same real, load-bearing precedent as ReverbTopology.hpp: every factory
// preset's Reverb slots use reverbCharacter=0 (Default), so Default's table
// here is kept byte-identical to Reverb.hpp's original kEarlyTapMs/
// kEarlyTapGain -- zero golden-hash risk, only the four other characters
// (untouched by any shipped preset) get real new patterns.
namespace pw8::effects
{
    struct ReverbEarlyPattern
    {
        std::array<float, kNumReverbEarlyTaps> tapMs;
        std::array<float, kNumReverbEarlyTaps> tapGain;
        std::size_t activeTaps; // <= kNumReverbEarlyTaps, kept even so the L/R alternating tap assignment in
                                 // Reverb.hpp stays balanced (i%2==0 -> L, else -> R)
    };

    namespace detail
    {
        // Real, unchanged -- byte-identical to Reverb.hpp's own original
        // kEarlyTapMs/kEarlyTapGain.
        inline constexpr ReverbEarlyPattern kDefaultEarlyPattern{
            {6.1f, 9.7f, 13.3f, 17.9f, 21.1f, 26.3f, 31.7f, 38.9f},
            {0.62f, 0.53f, 0.45f, 0.38f, 0.33f, 0.28f, 0.24f, 0.20f},
            kNumReverbEarlyTaps,
        };

        // Real plate: not a room at all -- a minimal, fast cluster that
        // blends almost immediately into the (already fast/dense, per
        // ReverbTopology.hpp) tank rather than a real spatial early field.
        inline constexpr ReverbEarlyPattern kPlateEarlyPattern{
            {2.3f, 4.7f, 7.1f, 9.7f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.40f, 0.32f, 0.24f, 0.16f, 0.0f, 0.0f, 0.0f, 0.0f},
            4,
        };

        // Real hall: longer first-reflection delay + a sparser, more widely
        // time-spaced pattern (a real big-room signature).
        inline constexpr ReverbEarlyPattern kHallEarlyPattern{
            {14.3f, 27.1f, 41.7f, 58.3f, 76.9f, 97.1f, 118.3f, 142.7f},
            {0.55f, 0.48f, 0.42f, 0.36f, 0.30f, 0.25f, 0.20f, 0.16f},
            kNumReverbEarlyTaps,
        };

        // Real room: short first-reflection delay, but denser (all 8 taps
        // active, tightly packed) -- a real small-room signature: many
        // nearby boundaries reflecting back quickly.
        inline constexpr ReverbEarlyPattern kRoomEarlyPattern{
            {3.1f, 5.3f, 7.1f, 8.9f, 10.7f, 12.9f, 15.1f, 17.3f},
            {0.58f, 0.52f, 0.47f, 0.42f, 0.37f, 0.32f, 0.27f, 0.22f},
            kNumReverbEarlyTaps,
        };

        // Real spring: essentially no discrete "room" early cluster -- just
        // a couple of very quiet, fast taps before the sparse resonant tail
        // (ReverbTopology.hpp's own 4-active-line reduction) takes over.
        inline constexpr ReverbEarlyPattern kSpringEarlyPattern{
            {4.3f, 11.7f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.20f, 0.12f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            2,
        };
    } // namespace detail

    /// Real per-character early-reflection pattern lookup. Default and
    /// Shimmer deliberately share one pattern -- same real reasoning as
    /// ReverbTopology.hpp's own Default/Shimmer sharing. Falls back to
    /// Default for any out-of-range value.
    [[nodiscard]] inline const ReverbEarlyPattern& getReverbEarlyPattern(ReverbCharacter character) noexcept
    {
        switch (character)
        {
            case ReverbCharacter::Plate:
                return detail::kPlateEarlyPattern;
            case ReverbCharacter::Hall:
                return detail::kHallEarlyPattern;
            case ReverbCharacter::Room:
                return detail::kRoomEarlyPattern;
            case ReverbCharacter::Spring:
                return detail::kSpringEarlyPattern;
            case ReverbCharacter::Shimmer:
            case ReverbCharacter::Default:
            default:
                return detail::kDefaultEarlyPattern;
        }
    }

} // namespace pw8::effects
