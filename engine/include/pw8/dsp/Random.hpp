#pragma once

#include <cstdint>

// Deterministic, seedable RNG used everywhere randomness touches the signal path
// (voice variation, noise engines, humanize, grain position, step probability).
// Reproducibility is a hard requirement: the same seed + inputs must render
// byte-for-byte identical control decisions across platforms.
//
// Implementation: splitmix64 (public domain, Vigna/Steele) -- small, fast,
// well-distributed, trivial to reason about and to reimplement in Python for
// cross-checking Patchwork-side generation against engine-side rendering.

namespace pw8::dsp
{
    class DeterministicRng
    {
    public:
        constexpr DeterministicRng() noexcept = default;
        constexpr explicit DeterministicRng(std::uint64_t seed) noexcept : state_(seed) {}

        constexpr void reseed(std::uint64_t seed) noexcept { state_ = seed; }

        [[nodiscard]] constexpr std::uint64_t nextU64() noexcept
        {
            std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
            return z ^ (z >> 31);
        }

        /// Uniform float in [0, 1).
        [[nodiscard]] constexpr float nextFloat() noexcept
        {
            // Take the top 24 bits for a well-distributed float mantissa.
            const std::uint32_t bits = static_cast<std::uint32_t>(nextU64() >> 40);
            return static_cast<float>(bits) / static_cast<float>(1u << 24);
        }

        /// Uniform float in [lo, hi).
        [[nodiscard]] constexpr float nextRange(float lo, float hi) noexcept
        {
            return lo + nextFloat() * (hi - lo);
        }

        /// Derive a stable per-voice/per-note sub-seed from a base patch seed and IDs,
        /// so voice variation is deterministic but decorrelated across voices/notes
        /// without sharing a single running stream (which would make voice N's
        /// output depend on how many notes were generated before it).
        [[nodiscard]] static constexpr std::uint64_t deriveSeed(std::uint64_t patchSeed,
                                                                  std::uint32_t voiceId,
                                                                  std::uint64_t noteGenerationId) noexcept
        {
            std::uint64_t h = patchSeed ^ (0x9E3779B97F4A7C15ULL + voiceId);
            h ^= noteGenerationId + 0x9E3779B97F4A7C15ULL + (h << 6) + (h >> 2);
            // One splitmix64 round to mix the combined seed thoroughly.
            h = (h ^ (h >> 30)) * 0xBF58476D1CE4E5B9ULL;
            h = (h ^ (h >> 27)) * 0x94D049BB133111EBULL;
            return h ^ (h >> 31);
        }

    private:
        std::uint64_t state_ = 0x853C49E6748FEA9BULL;
    };

} // namespace pw8::dsp
