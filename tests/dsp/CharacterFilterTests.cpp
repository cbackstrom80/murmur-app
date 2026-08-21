#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/filter/CharacterFilter.hpp"

using namespace pw8::filter;

namespace
{
    constexpr double kSampleRate = 48000.0;

    float measureRms(float cutoffHz, float resonance, float drive, float toneHz, double seconds,
                      float modeMorph = 0.0f)
    {
        CharacterFilter f;
        f.prepare(kSampleRate);

        const auto numSamples = static_cast<int>(seconds * kSampleRate);
        double sumSq = 0.0;
        int counted = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
            const float input = std::sin(2.0f * 3.14159265f * toneHz * t);
            const float output = f.renderSample(input, cutoffHz, resonance, drive, modeMorph);
            if (i > numSamples / 2)
            {
                sumSq += static_cast<double>(output) * output;
                ++counted;
            }
        }
        return static_cast<float>(std::sqrt(sumSq / counted));
    }
} // namespace

TEST_CASE("CharacterFilter lowpass attenuates high frequencies", "[filter][character]")
{
    const float passed = measureRms(500.0f, 0.2f, 0.0f, 100.0f, 0.05);
    const float attenuated = measureRms(500.0f, 0.2f, 0.0f, 8000.0f, 0.05);
    REQUIRE(passed > 0.2f);
    REQUIRE(attenuated < passed * 0.35f);
}

TEST_CASE("CharacterFilter drive increases harmonic energy", "[filter][character]")
{
    const float clean = measureRms(2000.0f, 0.15f, 0.0f, 220.0f, 0.08);
    const float driven = measureRms(2000.0f, 0.15f, 0.85f, 220.0f, 0.08);
    REQUIRE(driven > clean * 1.05f);
}

TEST_CASE("CharacterFilter stays finite at high resonance", "[filter][character][stability]")
{
    CharacterFilter f;
    f.prepare(kSampleRate);
    for (int i = 0; i < 48000; ++i)
    {
        const float t = static_cast<float>(i) / 48000.0f;
        const float input = std::sin(2.0f * 3.14159265f * 110.0f * t);
        const float out = f.renderSample(input, 800.0f, 1.0f, 0.5f);
        REQUIRE(std::isfinite(out));
        REQUIRE(std::abs(out) < 100.0f);
    }
}

TEST_CASE("CharacterFilter reset clears state", "[filter][character]")
{
    CharacterFilter f;
    f.prepare(kSampleRate);
    for (int i = 0; i < 1000; ++i)
        (void)f.renderSample(1.0f, 1000.0f, 0.8f, 0.5f);
    f.reset();
    const float afterReset = f.renderSample(0.0f, 1000.0f, 0.8f, 0.5f);
    REQUIRE(afterReset == Catch::Approx(0.0f).margin(0.02f));
}

// The tests below cover modeMorph, added to give Filter 2 real HP/BP responses from
// its existing 4-stage ladder cascade (see CharacterFilter.hpp's derivation comment)
// instead of the LP-only output it originally shipped with. Mirrors
// StateVariableFilterTests.cpp's own LP/HP/BP direction tests.

TEST_CASE("CharacterFilter default modeMorph (0.0) matches pre-morph lowpass-only behavior", "[filter][character]")
{
    // Two call styles must agree exactly: the 4-arg legacy signature (no modeMorph
    // param at all) and the 5-arg one with modeMorph explicitly 0.0 -- both take the
    // fast path back to the original LP4-only output, so existing patches/presets
    // that never set modeMorph render byte-identical output.
    CharacterFilter legacy;
    CharacterFilter explicit0;
    legacy.prepare(kSampleRate);
    explicit0.prepare(kSampleRate);
    for (int i = 0; i < 2000; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float input = std::sin(2.0f * 3.14159265f * 220.0f * t);
        const float a = legacy.renderSample(input, 1500.0f, 0.4f, 0.2f);
        const float b = explicit0.renderSample(input, 1500.0f, 0.4f, 0.2f, 0.0f);
        REQUIRE(a == b);
    }
}

TEST_CASE("CharacterFilter highpass tap (modeMorph=1) strongly attenuates a tone well below cutoff",
          "[filter][character]")
{
    const float attenuated = measureRms(4000.0f, 0.2f, 0.0f, 100.0f, 0.05, 1.0f);
    const float passed = measureRms(4000.0f, 0.2f, 0.0f, 12000.0f, 0.05, 1.0f);
    REQUIRE(attenuated < passed * 0.35f);
    REQUIRE(passed > 0.1f);
}

TEST_CASE("CharacterFilter bandpass tap (modeMorph=0.5) favors the center frequency", "[filter][character]")
{
    const float atCenter = measureRms(1000.0f, 0.5f, 0.0f, 1000.0f, 0.05, 0.5f);
    const float twoOctavesAway = measureRms(1000.0f, 0.5f, 0.0f, 4000.0f, 0.05, 0.5f);
    REQUIRE(atCenter > twoOctavesAway * 1.5f);
}

TEST_CASE("CharacterFilter modeMorph stays finite across a full sweep at high resonance/drive",
          "[filter][character][stability]")
{
    CharacterFilter f;
    f.prepare(kSampleRate);
    for (int i = 0; i < 48000; ++i)
    {
        const float t = static_cast<float>(i) / 48000.0f;
        const float input = std::sin(2.0f * 3.14159265f * 110.0f * t);
        const float morph = static_cast<float>(i) / 48000.0f; // sweeps 0..1 over the render.
        const float out = f.renderSample(input, 800.0f, 1.0f, 0.9f, morph);
        REQUIRE(std::isfinite(out));
        REQUIRE(std::abs(out) < 100.0f);
    }
}

TEST_CASE("CharacterFilter reset clears state with modeMorph active", "[filter][character]")
{
    CharacterFilter f;
    f.prepare(kSampleRate);
    for (int i = 0; i < 1000; ++i)
        (void)f.renderSample(1.0f, 1000.0f, 0.8f, 0.5f, 1.0f);
    f.reset();
    const float afterReset = f.renderSample(0.0f, 1000.0f, 0.8f, 0.5f, 1.0f);
    REQUIRE(afterReset == Catch::Approx(0.0f).margin(0.02f));
}
