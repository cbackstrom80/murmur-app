#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/operator/OperatorNode.hpp"
#include "pw8/oscillator/GranularOscillator.hpp"
#include "pw8/oscillator/WavetableTable.hpp"

using namespace pw8;
using namespace pw8::oscillator;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr float kGranularRootHz = 261.6256f; // Matches GranularOscillator's own C4 convention.

    /// A single-frame, single-mip source table -- a moderate-amplitude sine, so
    /// every possible grain read position has real (nonzero) content.
    WavetableTable makeSineSourceTable(int samplesPerFrame = 8192)
    {
        WavetableTable table;
        table.numFrames = 1;
        table.samplesPerFrame = samplesPerFrame;

        WavetableTable::MipLevel mip;
        mip.maxHarmonic = 1;
        mip.samples.resize(static_cast<std::size_t>(samplesPerFrame));
        for (int i = 0; i < samplesPerFrame; ++i)
            mip.samples[static_cast<std::size_t>(i)] =
                0.8f * std::sin(2.0f * 3.14159265f * 9.0f * static_cast<float>(i) / static_cast<float>(samplesPerFrame));
        table.mips.push_back(std::move(mip));
        return table;
    }

    WavetableView sourceViewFrom(const WavetableTable& table)
    {
        return WavetableView{table.mips.front().samples.data(), table.numFrames, table.samplesPerFrame};
    }

    std::vector<float> renderGranular(const WavetableView& source, const GranularParams& params, float freqHz,
                                       std::uint64_t seed, std::size_t numSamples)
    {
        GranularOscillator osc;
        osc.prepare(kSampleRate);
        osc.reset(seed);
        osc.setFrequency(freqHz);

        std::vector<float> out(numSamples);
        for (std::size_t i = 0; i < numSamples; ++i)
            out[i] = osc.renderSample(params, source);
        return out;
    }

    /// Counts rising edges above `threshold` in |samples| -- a simple, robust
    /// onset counter valid when grains don't overlap (grainSizeMs well under the
    /// inter-grain period), which the density test below deliberately arranges.
    int countOnsets(const std::vector<float>& samples, float threshold)
    {
        int count = 0;
        bool above = false;
        for (float s : samples)
        {
            const bool isAbove = std::abs(s) > threshold;
            if (isAbove && !above)
                ++count;
            above = isAbove;
        }
        return count;
    }
} // namespace

TEST_CASE("GranularOscillator output stays bounded and finite across the extreme parameter matrix",
          "[granular][stability]")
{
    const auto table = makeSineSourceTable();
    const auto source = sourceViewFrom(table);

    for (float density : {0.5f, 20.0f, 200.0f})
        for (float sizeMs : {1.0f, 60.0f, 500.0f})
            for (float posJitter : {0.0f, 1.0f})
                for (float pitchJitter : {0.0f, 1.0f})
                    for (float freqHz : {20.0f, 440.0f, 8000.0f})
                    {
                        GranularParams params;
                        params.densityHz = density;
                        params.grainSizeMs = sizeMs;
                        params.positionJitter = posJitter;
                        params.pitchJitter = pitchJitter;
                        params.framePosition01 = 0.5f;

                        const auto samples = renderGranular(source, params, freqHz, 0x1111ULL, 4096);
                        for (float s : samples)
                        {
                            REQUIRE(std::isfinite(s));
                            REQUIRE(std::abs(s) <= 4.0f);
                        }
                    }
}

TEST_CASE("GranularOscillator is silent with no source table (invalid view)", "[granular][robustness]")
{
    GranularOscillator osc;
    osc.prepare(kSampleRate);
    osc.reset(0x42ULL);
    osc.setFrequency(440.0f);

    GranularParams params;
    for (int i = 0; i < 4096; ++i)
        REQUIRE(osc.renderSample(params, WavetableView{}) == 0.0f);
}

TEST_CASE("GranularOscillator: same seed reproduces the exact same output stream", "[granular][determinism]")
{
    const auto table = makeSineSourceTable();
    const auto source = sourceViewFrom(table);
    GranularParams params;
    params.positionJitter = 0.5f;
    params.pitchJitter = 0.5f;

    const auto a = renderGranular(source, params, kGranularRootHz, 0xC0FFEEULL, 8192);
    const auto b = renderGranular(source, params, kGranularRootHz, 0xC0FFEEULL, 8192);
    REQUIRE(a == b);

    const auto c = renderGranular(source, params, kGranularRootHz, 0xDEADBEEFULL, 8192);
    REQUIRE(a != c);
}

TEST_CASE("GranularOscillator: grain onset rate tracks grainDensity roughly linearly", "[granular][measured]")
{
    const auto table = makeSineSourceTable();
    const auto source = sourceViewFrom(table);

    // No jitter, and grain size kept well under each config's inter-grain period
    // so successive grains don't overlap -- every grain's Hann envelope returns
    // to (near) zero between onsets, making a simple threshold-crossing counter
    // an accurate, robust proxy for the actual trigger rate.
    GranularParams lowDensity;
    lowDensity.densityHz = 20.0f;    // period ~50ms.
    lowDensity.grainSizeMs = 8.0f;   // well under 50ms.
    lowDensity.positionJitter = 0.0f;
    lowDensity.pitchJitter = 0.0f;

    GranularParams highDensity = lowDensity;
    highDensity.densityHz = 60.0f;   // period ~16.7ms.
    highDensity.grainSizeMs = 5.0f;  // well under 16.7ms.

    constexpr double seconds = 2.0;
    const auto lowSamples =
        renderGranular(source, lowDensity, kGranularRootHz, 0xAAAAULL, static_cast<std::size_t>(seconds * kSampleRate));
    const auto highSamples =
        renderGranular(source, highDensity, kGranularRootHz, 0xAAAAULL, static_cast<std::size_t>(seconds * kSampleRate));

    const int lowCount = countOnsets(lowSamples, 0.05f);
    const int highCount = countOnsets(highSamples, 0.05f);

    const double expectedLow = lowDensity.densityHz * seconds;
    const double expectedHigh = highDensity.densityHz * seconds;

    REQUIRE(static_cast<double>(lowCount) == Catch::Approx(expectedLow).margin(expectedLow * 0.2));
    REQUIRE(static_cast<double>(highCount) == Catch::Approx(expectedHigh).margin(expectedHigh * 0.2));
    // The core claim: onset rate scales with density, not a coincidence of the
    // specific numbers above.
    REQUIRE(highCount > lowCount);
}

TEST_CASE("OperatorState renders Granular through the full operator path", "[granular][operator]")
{
    const auto table = makeSineSourceTable();

    op::OperatorState state;
    state.prepare(48000.0);
    state.seedGranular(dsp::DeterministicRng::deriveSeed(42, 0, 0));

    op::OperatorParams params;
    params.engine = algorithm::EngineType::Granular;
    params.frequencyRatio = 1.0f;
    params.level = 1.0f;
    params.wavetableFramePosition = 0.5f;
    params.grainDensity = 30.0f;
    params.grainSizeMs = 40.0f;
    params.grainPositionJitter = 0.1f;
    params.grainPitchJitter = 0.1f;

    bool sawNonZero = false;
    for (int i = 0; i < 16384; ++i)
    {
        const float s = state.render(params, &table, kGranularRootHz, 0.0f, 0.0f);
        REQUIRE(std::isfinite(s));
        REQUIRE(std::abs(s) <= 4.0f);
        if (s != 0.0f)
            sawNonZero = true;
    }
    REQUIRE(sawNonZero);
}

TEST_CASE("OperatorState renders Granular as silence when no wavetable is loaded", "[granular][operator]")
{
    op::OperatorState state;
    state.prepare(48000.0);
    state.seedGranular(dsp::DeterministicRng::deriveSeed(42, 0, 0));

    op::OperatorParams params;
    params.engine = algorithm::EngineType::Granular;
    params.level = 1.0f;

    for (int i = 0; i < 4096; ++i)
        REQUIRE(state.render(params, nullptr, 220.0f, 0.0f, 0.0f) == 0.0f);
}
