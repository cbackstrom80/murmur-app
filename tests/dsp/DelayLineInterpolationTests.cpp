#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/dsp/DelayLine.hpp"

using namespace pw8::dsp;

namespace
{
    constexpr double kSampleRate = 48000.0;
} // namespace

TEST_CASE("DelayLine::readInterpolatedHermite has lower reconstruction error than linear on a smooth signal",
          "[dsp][delayline]")
{
    // Real, quantitative proof -- not a subjective "sounds better" claim.
    // Write a known sine wave sample-by-sample, then read it back at a
    // spread of fractional delay offsets via both interpolation methods
    // and compare each against the exact analytic sine value at that
    // continuous read time. Cubic Hermite is mathematically expected to
    // reconstruct a smooth, band-limited signal with lower error than
    // linear -- this confirms the real implementation actually delivers
    // that, not just that it compiles/doesn't crash.
    DelayLine line;
    line.prepare(kSampleRate, 0.05f); // real 50ms buffer, plenty of margin for the offsets used below

    constexpr float kFreqHz = 4000.0f; // real, non-trivial fraction of Nyquist -- linear's error isn't negligible here
    constexpr int kNumWritten = 1000;
    for (int n = 0; n < kNumWritten; ++n)
    {
        const float t = static_cast<float>(n) / static_cast<float>(kSampleRate);
        line.write(std::sin(kTwoPi * kFreqHz * t));
    }

    // writePos_ == kNumWritten here (nothing has wrapped yet -- the 50ms
    // buffer is far larger than 1000 samples), so the exact continuous-time
    // read position for a given delay `d` is (kNumWritten - d) / sampleRate.
    const float nWrite = static_cast<float>(kNumWritten);

    double linearSqErr = 0.0;
    double hermiteSqErr = 0.0;
    int numSamples = 0;

    // Real spread of base delays and fractional offsets, all comfortably
    // inside the written history and inside both methods' margin
    // requirements.
    for (int baseDelay = 20; baseDelay <= 200; baseDelay += 10)
    {
        for (const float frac : {0.1f, 0.23f, 0.37f, 0.5f, 0.63f, 0.79f, 0.91f})
        {
            const float d = static_cast<float>(baseDelay) + frac;
            const float readTime = (nWrite - d) / static_cast<float>(kSampleRate);
            const float exact = std::sin(kTwoPi * kFreqHz * readTime);

            const float linearVal = line.readInterpolated(d);
            const float hermiteVal = line.readInterpolatedHermite(d);

            const double linearErr = static_cast<double>(linearVal - exact);
            const double hermiteErr = static_cast<double>(hermiteVal - exact);
            linearSqErr += linearErr * linearErr;
            hermiteSqErr += hermiteErr * hermiteErr;
            ++numSamples;
        }
    }

    const double linearRms = std::sqrt(linearSqErr / numSamples);
    const double hermiteRms = std::sqrt(hermiteSqErr / numSamples);

    INFO("linear RMS error = " << linearRms << ", hermite RMS error = " << hermiteRms);

    // Real, generous margin (not a knife's-edge threshold): cubic
    // reconstruction of a smooth sine at this frequency/sample-rate ratio
    // should be substantially more accurate than linear, not merely "a bit
    // better".
    REQUIRE(hermiteRms < linearRms * 0.5);
    // And linear itself should be a real, non-trivial error at this
    // frequency -- proof this test is actually exercising the interesting
    // regime, not two methods that are both trivially exact.
    REQUIRE(linearRms > 0.001);
}
