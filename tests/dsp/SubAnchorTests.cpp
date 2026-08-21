#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/spatial/SubAnchor.hpp"

using namespace pw8::spatial;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr float kTwoPi = 2.0f * 3.14159265f;
} // namespace

TEST_CASE("SubAnchor monoAmount=0 reconstructs input exactly (backward-compatible default)",
          "[spatial][subanchor]")
{
    // The real proof every existing patch needs: enabling Sub Anchor with the
    // default monoAmount must not change output at all -- decorrelated L/R
    // input, well below AND well above the crossover, both channels.
    SubAnchor anchor;
    anchor.prepare(kSampleRate);

    for (int i = 0; i < 4000; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float inL = std::sin(kTwoPi * 60.0f * t) + 0.5f * std::sin(kTwoPi * 3000.0f * t);
        const float inR = std::sin(kTwoPi * 60.0f * t + 1.7f) + 0.5f * std::sin(kTwoPi * 3200.0f * t);
        float outL = 0.0f, outR = 0.0f;
        anchor.renderSample(inL, inR, 120.0f, 0.0f, outL, outR);
        REQUIRE(outL == Catch::Approx(inL).margin(1.0e-5f));
        REQUIRE(outR == Catch::Approx(inR).margin(1.0e-5f));
    }
}

TEST_CASE("SubAnchor monoAmount=1 substantially reduces L/R difference on decorrelated sub content",
          "[spatial][subanchor]")
{
    // Real correctness proof, framed the way real "bass mono" tools actually
    // are: this is a crossover-based technique, not a brick-wall phase-lock
    // (the recombined "residual" tap necessarily still carries some of each
    // channel's own phase-shifted low-frequency energy near the transition
    // band -- a real, honest property of this approach, not a bug to hide).
    // What must be real and measurable is a substantial *reduction* in L/R
    // difference for genuinely decorrelated sub content, not perfect
    // equality. Compares RMS(outL-outR) against RMS(inL-inR) for the same
    // signal, unprocessed.
    SubAnchor anchor;
    anchor.prepare(kSampleRate);
    const float crossoverHz = 120.0f;

    double sumSqRaw = 0.0;
    double sumSqAnchored = 0.0;
    int counted = 0;
    for (int i = 0; i < 6000; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        // 20Hz: genuinely deep sub content (below most kick-drum fundamentals),
        // real-world representative of what this feature actually targets.
        const float inL = std::sin(kTwoPi * 20.0f * t);
        const float inR = std::sin(kTwoPi * 20.0f * t + 3.0f); // ~172 degrees out of phase
        float outL = 0.0f, outR = 0.0f;
        anchor.renderSample(inL, inR, crossoverHz, 1.0f, outL, outR);
        if (i > 3000) // let filter state settle first
        {
            const double rawDiff = static_cast<double>(inL) - inR;
            const double anchoredDiff = static_cast<double>(outL) - outR;
            sumSqRaw += rawDiff * rawDiff;
            sumSqAnchored += anchoredDiff * anchoredDiff;
            ++counted;
        }
    }
    const double rawRms = std::sqrt(sumSqRaw / counted);
    const double anchoredRms = std::sqrt(sumSqAnchored / counted);
    REQUIRE(rawRms > 0.5); // sanity: the test input really is decorrelated
    REQUIRE(anchoredRms < rawRms * 0.5);
}

TEST_CASE("SubAnchor leaves content well above the crossover untouched at any monoAmount",
          "[spatial][subanchor]")
{
    // The split is real and frequency-selective, not a blanket mono effect --
    // a decorrelated tone well above the crossover must survive un-summed
    // even at monoAmount=1.
    SubAnchor anchor;
    anchor.prepare(kSampleRate);

    float maxAbsDiff = 0.0f;
    for (int i = 0; i < 4000; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float inL = std::sin(kTwoPi * 4000.0f * t);
        const float inR = std::sin(kTwoPi * 4000.0f * t + 2.5f);
        float outL = 0.0f, outR = 0.0f;
        anchor.renderSample(inL, inR, 120.0f, 1.0f, outL, outR);
        if (i > 500)
        {
            maxAbsDiff = std::max(maxAbsDiff, std::abs(outL - inL));
            maxAbsDiff = std::max(maxAbsDiff, std::abs(outR - inR));
        }
    }
    REQUIRE(maxAbsDiff < 0.05f);
}

TEST_CASE("SubAnchor stays finite across a full monoAmount/crossover sweep", "[spatial][subanchor][stability]")
{
    SubAnchor anchor;
    anchor.prepare(kSampleRate);
    for (int i = 0; i < 48000; ++i)
    {
        const float t = static_cast<float>(i) / 48000.0f;
        const float inL = std::sin(kTwoPi * 55.0f * t);
        const float inR = std::sin(kTwoPi * 57.0f * t);
        const float monoAmount = static_cast<float>(i) / 48000.0f; // sweeps 0..1
        const float crossoverHz = 60.0f + 300.0f * monoAmount;     // sweeps 60..360Hz
        float outL = 0.0f, outR = 0.0f;
        anchor.renderSample(inL, inR, crossoverHz, monoAmount, outL, outR);
        REQUIRE(std::isfinite(outL));
        REQUIRE(std::isfinite(outR));
        REQUIRE(std::abs(outL) < 100.0f);
        REQUIRE(std::abs(outR) < 100.0f);
    }
}

TEST_CASE("SubAnchor reset clears filter state", "[spatial][subanchor]")
{
    SubAnchor anchor;
    anchor.prepare(kSampleRate);
    for (int i = 0; i < 1000; ++i)
        (void)([&]() {
            float outL = 0.0f, outR = 0.0f;
            anchor.renderSample(1.0f, -1.0f, 120.0f, 1.0f, outL, outR);
        })();
    anchor.reset();
    float outL = 0.0f, outR = 0.0f;
    anchor.renderSample(0.0f, 0.0f, 120.0f, 1.0f, outL, outR);
    REQUIRE(outL == Catch::Approx(0.0f).margin(0.02f));
    REQUIRE(outR == Catch::Approx(0.0f).margin(0.02f));
}
