#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/oscillator/ClassicOscillator.hpp"

using namespace pw8::oscillator;

namespace
{
    // Counts rising zero-crossings over `seconds` of rendered audio to estimate frequency.
    double measureFrequencyHz(ClassicWaveform waveform, float freqHz, double sampleRate, double seconds)
    {
        ClassicOscillator osc;
        osc.prepare(sampleRate);
        osc.setFrequency(freqHz);

        ClassicOscillatorParams params;
        params.waveform = waveform;

        const auto numSamples = static_cast<std::size_t>(seconds * sampleRate);
        float prev = osc.renderSample(params);
        int crossings = 0;
        for (std::size_t i = 1; i < numSamples; ++i)
        {
            const float cur = osc.renderSample(params);
            if (prev < 0.0f && cur >= 0.0f)
                ++crossings;
            prev = cur;
        }
        return static_cast<double>(crossings) / seconds;
    }
} // namespace

TEST_CASE("ClassicOscillator tunes correctly across waveforms", "[oscillator][tuning]")
{
    const double sampleRate = 48000.0;
    const double seconds = 1.0;

    for (auto waveform : {ClassicWaveform::Sine, ClassicWaveform::Saw, ClassicWaveform::Square})
    {
        const double measured = measureFrequencyHz(waveform, 220.0f, sampleRate, seconds);
        REQUIRE(measured == Catch::Approx(220.0).margin(1.0));
    }
}

TEST_CASE("ClassicOscillator output stays bounded and finite", "[oscillator][stability]")
{
    ClassicOscillator osc;
    osc.prepare(48000.0);
    osc.setFrequency(4000.0f); // high frequency stresses PolyBLEP correction terms.

    ClassicOscillatorParams params;
    params.waveform = ClassicWaveform::Square;
    params.pulseWidth = 0.1f;

    for (int i = 0; i < 48000; ++i)
    {
        const float s = osc.renderSample(params);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 1.5f); // small PolyBLEP overshoot tolerance.
    }
}

TEST_CASE("ClassicOscillator morph sweeps continuously without discontinuity blowups", "[oscillator][morph]")
{
    ClassicOscillator osc;
    osc.prepare(48000.0);
    osc.setFrequency(110.0f);

    ClassicOscillatorParams params;
    params.morph = 0.0f;

    float maxAbs = 0.0f;
    for (int i = 0; i < 48000; ++i)
    {
        params.morph = static_cast<float>(i) / 48000.0f;
        const float s = osc.renderSample(params);
        REQUIRE(std::isfinite(s));
        maxAbs = std::max(maxAbs, std::abs(s));
    }
    REQUIRE(maxAbs <= 2.0f);
}

TEST_CASE("ClassicOscillator reset places phase deterministically", "[oscillator]")
{
    ClassicOscillator a, b;
    a.prepare(48000.0);
    b.prepare(48000.0);
    a.setFrequency(330.0f);
    b.setFrequency(330.0f);
    a.reset(0.25f);
    b.reset(0.25f);

    ClassicOscillatorParams params;
    params.waveform = ClassicWaveform::Sine;

    for (int i = 0; i < 1000; ++i)
        REQUIRE(a.renderSample(params) == b.renderSample(params));
}
