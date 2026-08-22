#include <catch2/catch_test_macros.hpp>

#include "pw8/effects/ReverbEarlyPattern.hpp"

using namespace pw8::effects;

// Real, direct tests of getReverbEarlyPattern() -- the table-level companion
// to ReverbTopologyTests.cpp, same rationale: prove the per-character early-
// reflection patterns are genuinely distinct (not copy-paste placeholders),
// that Default matches what Reverb.hpp used to hardcode directly, and that
// every real value stays inside the engine's actual buffer ceiling.
namespace
{
    bool tapsEqual(const ReverbEarlyPattern& a, const ReverbEarlyPattern& b)
    {
        for (std::size_t i = 0; i < kNumReverbEarlyTaps; ++i)
            if (a.tapMs[i] != b.tapMs[i] || a.tapGain[i] != b.tapGain[i])
                return false;
        return true;
    }
} // namespace

TEST_CASE("Default early-reflection pattern matches the engine's original hardcoded taps", "[reverb][early]")
{
    const auto& p = getReverbEarlyPattern(ReverbCharacter::Default);
    REQUIRE(p.activeTaps == kNumReverbEarlyTaps);
    const std::array<float, kNumReverbEarlyTaps> expectedMs = {6.1f, 9.7f, 13.3f, 17.9f, 21.1f, 26.3f, 31.7f, 38.9f};
    const std::array<float, kNumReverbEarlyTaps> expectedGain = {0.62f, 0.53f, 0.45f, 0.38f, 0.33f, 0.28f, 0.24f,
                                                                   0.20f};
    for (std::size_t i = 0; i < kNumReverbEarlyTaps; ++i)
    {
        REQUIRE(p.tapMs[i] == expectedMs[i]);
        REQUIRE(p.tapGain[i] == expectedGain[i]);
    }
}

TEST_CASE("Shimmer deliberately reuses Default's early-reflection pattern", "[reverb][early]")
{
    REQUIRE(tapsEqual(getReverbEarlyPattern(ReverbCharacter::Default), getReverbEarlyPattern(ReverbCharacter::Shimmer)));
}

TEST_CASE("Plate/Hall/Room/Spring have genuinely distinct early-reflection patterns", "[reverb][early]")
{
    const ReverbEarlyPattern patterns[] = {
        getReverbEarlyPattern(ReverbCharacter::Default), getReverbEarlyPattern(ReverbCharacter::Plate),
        getReverbEarlyPattern(ReverbCharacter::Hall),    getReverbEarlyPattern(ReverbCharacter::Room),
        getReverbEarlyPattern(ReverbCharacter::Spring),
    };
    for (std::size_t a = 0; a < 5; ++a)
        for (std::size_t b = a + 1; b < 5; ++b)
            REQUIRE_FALSE(tapsEqual(patterns[a], patterns[b]));
}

TEST_CASE("Every character's active tap count is even (keeps the L/R alternating assignment balanced) and in range",
          "[reverb][early]")
{
    for (const auto character : {ReverbCharacter::Default, ReverbCharacter::Plate, ReverbCharacter::Hall,
                                  ReverbCharacter::Room, ReverbCharacter::Spring, ReverbCharacter::Shimmer})
    {
        const auto& p = getReverbEarlyPattern(character);
        REQUIRE(p.activeTaps > 0);
        REQUIRE(p.activeTaps <= kNumReverbEarlyTaps);
        REQUIRE(p.activeTaps % 2 == 0);
    }
}

TEST_CASE("Every real early-tap value stays inside the engine's actual early-line buffer ceiling", "[reverb][early]")
{
    constexpr float kMaxMs = kMaxReverbEarlyLineSeconds * 1000.0f;
    for (const auto character : {ReverbCharacter::Default, ReverbCharacter::Plate, ReverbCharacter::Hall,
                                  ReverbCharacter::Room, ReverbCharacter::Spring, ReverbCharacter::Shimmer})
    {
        const auto& p = getReverbEarlyPattern(character);
        for (std::size_t i = 0; i < p.activeTaps; ++i)
        {
            REQUIRE(p.tapMs[i] > 0.0f);
            REQUIRE(p.tapMs[i] < kMaxMs);
            REQUIRE(p.tapGain[i] > 0.0f);
            REQUIRE(p.tapGain[i] <= 1.0f);
        }
    }
}
