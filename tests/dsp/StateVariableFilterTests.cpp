#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/filter/StateVariableFilter.hpp"

using namespace pw8::filter;

namespace
{
    constexpr double kSampleRate = 48000.0;

    /// Feeds a sine tone through the filter for `seconds`, discarding the first half
    /// (settling time) and returning the RMS of the rest.
    float measureRms(FilterMode mode, float cutoffHz, float resonance, float toneHz, double seconds)
    {
        StateVariableFilter f;
        f.prepare(kSampleRate);

        const auto numSamples = static_cast<int>(seconds * kSampleRate);
        double sumSq = 0.0;
        int counted = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
            const float input = std::sin(2.0f * 3.14159265f * toneHz * t);
            const float output = f.renderSample(input, mode, cutoffHz, resonance);
            if (i > numSamples / 2)
            {
                sumSq += static_cast<double>(output) * output;
                ++counted;
            }
        }
        return static_cast<float>(std::sqrt(sumSq / counted));
    }

    float measureRmsMorph(float modeMorph, float cutoffHz, float resonance, float toneHz, double seconds)
    {
        StateVariableFilter f;
        f.prepare(kSampleRate);

        const auto numSamples = static_cast<int>(seconds * kSampleRate);
        double sumSq = 0.0;
        int counted = 0;

        for (int i = 0; i < numSamples; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
            const float input = std::sin(2.0f * 3.14159265f * toneHz * t);
            const float output =
                f.renderSample(input, FilterMode::Lowpass, modeMorph, cutoffHz, resonance);
            if (i > numSamples / 2)
            {
                sumSq += static_cast<double>(output) * output;
                ++counted;
            }
        }
        return static_cast<float>(std::sqrt(sumSq / counted));
    }
} // namespace

TEST_CASE("StateVariableFilter lowpass strongly attenuates a tone well above cutoff", "[filter]")
{
    const float passed = measureRms(FilterMode::Lowpass, 200.0f, 0.2f, 100.0f, 0.05);
    const float attenuated = measureRms(FilterMode::Lowpass, 200.0f, 0.2f, 8000.0f, 0.05);
    REQUIRE(passed > 0.3f);
    REQUIRE(attenuated < 0.05f);
}

TEST_CASE("StateVariableFilter highpass strongly attenuates a tone well below cutoff", "[filter]")
{
    const float attenuated = measureRms(FilterMode::Highpass, 4000.0f, 0.2f, 100.0f, 0.05);
    const float passed = measureRms(FilterMode::Highpass, 4000.0f, 0.2f, 12000.0f, 0.05);
    REQUIRE(attenuated < 0.05f);
    REQUIRE(passed > 0.3f);
}

TEST_CASE("StateVariableFilter bandpass favors the center frequency", "[filter]")
{
    const float atCenter = measureRms(FilterMode::Bandpass, 1000.0f, 0.5f, 1000.0f, 0.05);
    const float twoOctavesAway = measureRms(FilterMode::Bandpass, 1000.0f, 0.5f, 4000.0f, 0.05);
    REQUIRE(atCenter > twoOctavesAway * 2.0f);
}

TEST_CASE("StateVariableFilter modeMorph endpoints match discrete LP/BP/HP", "[filter][blades]")
{
    const float lp = measureRmsMorph(0.0f, 200.0f, 0.2f, 100.0f, 0.05);
    const float bp = measureRmsMorph(0.5f, 1000.0f, 0.5f, 1000.0f, 0.05);
    const float hp = measureRmsMorph(1.0f, 4000.0f, 0.2f, 12000.0f, 0.05);

    REQUIRE(lp == Catch::Approx(measureRms(FilterMode::Lowpass, 200.0f, 0.2f, 100.0f, 0.05)).margin(1.0e-4f));
    REQUIRE(bp == Catch::Approx(measureRms(FilterMode::Bandpass, 1000.0f, 0.5f, 1000.0f, 0.05)).margin(1.0e-4f));
    REQUIRE(hp == Catch::Approx(measureRms(FilterMode::Highpass, 4000.0f, 0.2f, 12000.0f, 0.05)).margin(1.0e-4f));
}

TEST_CASE("StateVariableFilter modeMorph interpolates between LP and BP", "[filter][blades]")
{
    const float lp = measureRmsMorph(0.0f, 500.0f, 0.3f, 2000.0f, 0.05);
    const float quarter = measureRmsMorph(0.25f, 500.0f, 0.3f, 2000.0f, 0.05);
    const float bp = measureRmsMorph(0.5f, 500.0f, 0.3f, 2000.0f, 0.05);

    REQUIRE(quarter > lp * 0.5f);
    REQUIRE(quarter < bp * 1.5f);
    REQUIRE(std::abs(quarter - lp) > 1.0e-4f);
}

TEST_CASE("StateVariableFilter modeMorph sweep stays finite", "[filter][blades][stability]")
{
    StateVariableFilter f;
    f.prepare(kSampleRate);
    for (int i = 0; i < 4800; ++i)
    {
        const float morph = static_cast<float>(i % 4800) / 4799.0f;
        const float t = static_cast<float>(i) / 48000.0f;
        const float input = std::sin(2.0f * 3.14159265f * 440.0f * t);
        const float out = f.renderSample(input, FilterMode::Lowpass, morph, 1200.0f, 0.85f);
        REQUIRE(std::isfinite(out));
        REQUIRE(std::abs(out) < 100.0f);
    }
}

TEST_CASE("StateVariableFilter stays finite and bounded at high resonance across the cutoff range", "[filter][stability]")
{
    for (float cutoff : {30.0f, 200.0f, 1000.0f, 8000.0f, 19000.0f})
    {
        StateVariableFilter f;
        f.prepare(kSampleRate);
        for (int i = 0; i < 48000; ++i)
        {
            const float t = static_cast<float>(i) / 48000.0f;
            const float input = std::sin(2.0f * 3.14159265f * 220.0f * t);
            const float out = f.renderSample(input, FilterMode::Lowpass, cutoff, 1.0f); // max resonance.
            REQUIRE(std::isfinite(out));
            REQUIRE(std::abs(out) < 100.0f);
        }
    }
}

TEST_CASE("StateVariableFilter reset() clears internal state", "[filter]")
{
    StateVariableFilter f;
    f.prepare(kSampleRate);
    for (int i = 0; i < 1000; ++i)
        (void)f.renderSample(1.0f, FilterMode::Lowpass, 500.0f, 0.8f);

    f.reset();
    // Immediately after reset, a single sample of silence in should produce a small
    // (near-zero, filter-coefficient-scaled) output rather than carrying over energy.
    const float out = f.renderSample(0.0f, FilterMode::Lowpass, 500.0f, 0.8f);
    REQUIRE(std::abs(out) < 1.0e-6f);
}
