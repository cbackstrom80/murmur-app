#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "AcousticMeasurements.hpp"
#include "pw8/effects/Reverb.hpp"

using namespace pw8::effects;

// Real, quantitative regression tests for the shared reverb tank's decay
// behavior -- numerically verifying that `reverbDecaySeconds` and the
// low/high RT60 ratio/crossover parameters actually produce the acoustic
// behavior their doc comments claim, not just "doesn't crash" or "hash
// matches" (golden-hash tests catch *any* change, including a legitimate
// one like Phase 4's Hermite interpolation upgrade -- they don't independently
// confirm the underlying physics is still correct). Measurement technique
// (Schroeder backward-integration EDC + fit-window regression) is in
// AcousticMeasurements.hpp, validated there against synthetic ground truth
// before being trusted here.
namespace
{
    constexpr double kSampleRate = 48000.0;

    std::vector<float> renderReverbTail(const EffectSlotParams& params, int totalSamples)
    {
        ReverbProcessor reverb;
        reverb.prepare(kSampleRate);
        std::vector<float> out(static_cast<std::size_t>(totalSamples));
        for (int i = 0; i < totalSamples; ++i)
        {
            const float in = (i == 0) ? 1.0f : 0.0f; // real unit impulse, then silence
            float outL = 0.0f, outR = 0.0f;
            reverb.processStereo(in, in, params, outL, outR);
            out[static_cast<std::size_t>(i)] = 0.5f * (outL + outR);
        }
        return out;
    }

    EffectSlotParams baseReverbParams(float decaySeconds)
    {
        EffectSlotParams p{};
        p.type = EffectType::Reverb;
        p.mix = 1.0f;
        p.reverbSizeParam = 1.0f;
        p.reverbDecaySeconds = decaySeconds;
        p.reverbDiffusion = 0.65f;
        p.reverbDensity = 0.85f;
        p.reverbModDepth = 0.35f;
        p.reverbEarlyLevel = 0.0f; // isolate the late tank -- cleaner decay curve to measure
        p.reverbLateLevel = 1.0f;
        return p;
    }
} // namespace

TEST_CASE("Reverb's measured broadband RT60 tracks reverbDecaySeconds", "[regression][reverb][acoustic]")
{
    // Flat low/high ratios (1.0) -- the mid-band target *is* the broadband
    // target here, so a direct RT60 measurement on the raw tail should track
    // reverbDecaySeconds itself across a real spread of values.
    for (const float targetRT60 : {0.5f, 1.0f, 2.0f, 4.0f})
    {
        EffectSlotParams params = baseReverbParams(targetRT60);
        params.reverbLowRatio = 1.0f;
        params.reverbHighRatio = 1.0f;
        params.reverbRollOffHz = 20000.0f;

        const int totalSamples = static_cast<int>(kSampleRate * static_cast<double>(targetRT60) * 2.5);
        const auto tail = renderReverbTail(params, totalSamples);
        const double measured = pw8::test::measureRT60(tail, kSampleRate);

        INFO("target=" << targetRT60 << "s measured=" << measured << "s");
        REQUIRE(measured > 0.0); // fit actually converged
        // Real, generous margin (validated worst-case error was ~4.4% across
        // this same target range) -- 20% catches a genuine regression
        // without being sensitive to the engine's normal small variation.
        REQUIRE(measured == Catch::Approx(static_cast<double>(targetRT60)).epsilon(0.20));
    }
}

TEST_CASE("Reverb's low/high RT60 ratio and crossover parameters produce correspondingly different per-band decay",
          "[regression][reverb][acoustic]")
{
    // Real M7-modeled defaults (EffectTypes.hpp): LF rings *longer* than mid
    // (ratio 1.3), HF decays *faster* (ratio 0.6) -- exactly the frequency-
    // dependent decay this file's own doc comment in Reverb.hpp claims to
    // implement (Jot's "absorptive filter" technique). Verify it's real.
    constexpr float rtMid = 2.0f;
    constexpr float lowRatio = 1.3f;
    constexpr float highRatio = 0.6f;
    constexpr float lowXoverHz = 400.0f;
    constexpr float highXoverHz = 4500.0f;

    EffectSlotParams params = baseReverbParams(rtMid);
    params.reverbLowRatio = lowRatio;
    params.reverbHighRatio = highRatio;
    params.reverbLowCrossoverHz = lowXoverHz;
    params.reverbHighCrossoverHz = highXoverHz;
    params.reverbRollOffHz = 20000.0f;

    const double rtLowExpected = static_cast<double>(rtMid) * lowRatio;
    const double rtHighExpected = static_cast<double>(rtMid) * highRatio;

    // Render long enough for the *slowest* band (low, 2.6s here) to decay a
    // full 60dB.
    const int totalSamples = static_cast<int>(kSampleRate * rtLowExpected * 2.5);
    const auto raw = renderReverbTail(params, totalSamples);

    const auto low = pw8::test::lowBand(raw, kSampleRate, lowXoverHz);
    const auto mid = pw8::test::midBand(raw, kSampleRate, lowXoverHz, highXoverHz);
    const auto high = pw8::test::highBand(raw, kSampleRate, highXoverHz);

    const double measuredLow = pw8::test::measureRT60(low, kSampleRate);
    const double measuredMid = pw8::test::measureRT60(mid, kSampleRate);
    const double measuredHigh = pw8::test::measureRT60(high, kSampleRate);

    INFO("low: expected=" << rtLowExpected << " measured=" << measuredLow);
    INFO("mid: expected=" << rtMid << " measured=" << measuredMid);
    INFO("high: expected=" << rtHighExpected << " measured=" << measuredHigh);

    REQUIRE(measuredLow > 0.0);
    REQUIRE(measuredMid > 0.0);
    REQUIRE(measuredHigh > 0.0);

    // Absolute match against the real target values (validated worst-case
    // per-band error was ~4.5%; 20% margin same rationale as the broadband
    // test above).
    REQUIRE(measuredLow == Catch::Approx(rtLowExpected).epsilon(0.20));
    REQUIRE(measuredMid == Catch::Approx(static_cast<double>(rtMid)).epsilon(0.20));
    REQUIRE(measuredHigh == Catch::Approx(rtHighExpected).epsilon(0.20));

    // Independent, cheaper-to-stay-true sanity check: regardless of exact
    // percentages, the real *ordering* low > mid > high must hold given
    // these ratios -- a second signal that survives even if the engine's
    // internals change in ways that shift absolute measurement accuracy.
    REQUIRE(measuredLow > measuredMid);
    REQUIRE(measuredMid > measuredHigh);
}
