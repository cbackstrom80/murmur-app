#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/dsp/Biquad.hpp"

using namespace pw8::dsp;

namespace
{
    constexpr double kSampleRate = 48000.0;

    /// Feeds a sine tone through a bandpass-configured Biquad for `seconds`,
    /// discarding the first half (settling time) and returning the RMS of the rest.
    float measureBandpassRms(float centerHz, float q, float toneHz, double seconds)
    {
        Biquad bq;
        bq.setBandpass(centerHz, q, kSampleRate);

        const auto numSamples = static_cast<int>(seconds * kSampleRate);
        double sumSq = 0.0;
        int counted = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
            const float input = std::sin(2.0f * 3.14159265f * toneHz * t);
            const float output = bq.renderSample(input);
            if (i > numSamples / 2)
            {
                sumSq += static_cast<double>(output) * output;
                ++counted;
            }
        }
        return static_cast<float>(std::sqrt(sumSq / counted));
    }
} // namespace

TEST_CASE("Biquad::setBandpass favors its center frequency over a tone an octave away", "[biquad][filter]")
{
    const float atCenter = measureBandpassRms(1000.0f, 8.0f, 1000.0f, 0.05);
    const float anOctaveUp = measureBandpassRms(1000.0f, 8.0f, 2000.0f, 0.05);
    const float anOctaveDown = measureBandpassRms(1000.0f, 8.0f, 500.0f, 0.05);

    REQUIRE(atCenter > anOctaveUp * 3.0f);
    REQUIRE(atCenter > anOctaveDown * 3.0f);
}

TEST_CASE("Biquad::setBandpass actually attenuates far-from-center content, unlike a peaking filter at high gain",
          "[biquad][filter]")
{
    // This is the whole reason setBandpass() exists rather than reusing
    // setPeaking() at a high gain (see the method's own doc comment) -- a tone
    // several octaves away from the center should be attenuated close to
    // silence, not merely "not boosted".
    const float atCenter = measureBandpassRms(1000.0f, 4.0f, 1000.0f, 0.05);
    const float farAway = measureBandpassRms(1000.0f, 4.0f, 8000.0f, 0.05);
    REQUIRE(farAway < atCenter * 0.1f);
}

TEST_CASE("Biquad::setBandpass narrows its passband as Q increases", "[biquad][filter]")
{
    const float offCenterHz = 1300.0f;
    const float lowQResponse = measureBandpassRms(1000.0f, 1.0f, offCenterHz, 0.05);
    const float highQResponse = measureBandpassRms(1000.0f, 16.0f, offCenterHz, 0.05);
    REQUIRE(highQResponse < lowQResponse);
}

TEST_CASE("Biquad::setBandpass stays finite and bounded at extreme Q and frequency", "[biquad][stability]")
{
    for (float freqHz : {20.0f, 1000.0f, 20000.0f})
        for (float q : {0.5f, 1.0f, 50.0f, 500.0f})
        {
            Biquad bq;
            bq.setBandpass(freqHz, q, kSampleRate);
            for (int i = 0; i < 4096; ++i)
            {
                const float input = (i % 7 == 0) ? 1.0f : -0.3f; // a cheap, deterministic non-sinusoidal stress input.
                const float out = bq.renderSample(input);
                REQUIRE(std::isfinite(out));
                REQUIRE(std::abs(out) <= 1000.0f); // generous -- a bandpass with high Q can ring, not a tight ceiling.
            }
        }
}
