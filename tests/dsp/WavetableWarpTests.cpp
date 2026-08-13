#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/oscillator/WavetableWarp.hpp"
#include "pw8/oscillator/WavetableOscillator.hpp"

using namespace pw8;
using namespace pw8::oscillator;

namespace
{
    double nonFundamentalEnergyRatio(const std::vector<float>& samples, std::size_t fundamentalBin)
    {
        const std::size_t n = samples.size();
        std::vector<std::complex<float>> data(n);
        for (std::size_t i = 0; i < n; ++i)
            data[i] = std::complex<float>(samples[i], 0.0f);
        dsp::fft(data, false);

        const std::size_t half = n / 2;
        const double fundamentalEnergy = std::norm(data[fundamentalBin]);
        double totalEnergy = 0.0;
        for (std::size_t k = 1; k < half; ++k)
            totalEnergy += std::norm(data[k]);
        const double nonFundamental = totalEnergy - fundamentalEnergy;
        return nonFundamental / std::max(fundamentalEnergy, 1.0e-9);
    }

    std::vector<float> renderSineTableWithWarp(float bend, float asymmetry, double sampleRate, float freqHz,
                                               std::size_t numSamples)
    {
        constexpr int kTableSize = 256;
        std::vector<float> table(static_cast<std::size_t>(kTableSize));
        for (int i = 0; i < kTableSize; ++i)
            table[static_cast<std::size_t>(i)] =
                std::sin(dsp::kTwoPi * static_cast<float>(i) / static_cast<float>(kTableSize));

        WtWarpParams params;
        params.bend = bend;
        params.asymmetry = asymmetry;

        std::vector<float> out(numSamples);
        float phase = 0.0f;
        const float dt = freqHz / static_cast<float>(sampleRate);
        for (std::size_t i = 0; i < numSamples; ++i)
        {
            const float readPhase = warpReadPhase(phase, params);
            const float sampleF = readPhase * static_cast<float>(kTableSize);
            const int idx0 = static_cast<int>(sampleF) % kTableSize;
            const int idx1 = (idx0 + 1) % kTableSize;
            const float frac = sampleF - std::floor(sampleF);
            out[i] = dsp::lerp(table[static_cast<std::size_t>(idx0)], table[static_cast<std::size_t>(idx1)], frac);
            phase = dsp::wrapPhase(phase + dt);
        }
        return out;
    }
} // namespace

TEST_CASE("WavetableWarp identity at zero bend and asymmetry", "[wavetable][warp]")
{
    WtWarpParams params;
    REQUIRE(warpReadPhase(0.25f, params) == Catch::Approx(0.25f));
    REQUIRE(warpReadPhase(0.75f, params) == Catch::Approx(0.75f));
}

TEST_CASE("WavetableWarp bend displaces read phase", "[wavetable][warp]")
{
    WtWarpParams neutral;
    WtWarpParams bent;
    bent.bend = 0.8f;

    REQUIRE(warpReadPhase(0.25f, bent) != Catch::Approx(warpReadPhase(0.25f, neutral)).margin(1.0e-4f));
}

TEST_CASE("WavetableWarp bend increases harmonics", "[wavetable][warp]")
{
    constexpr std::size_t kNumSamples = 8192;
    constexpr std::size_t kWarmup = 512;
    constexpr double kSampleRate = 48000.0;
    constexpr std::size_t kFundamentalBin = 40;
    constexpr float kFreqHz = static_cast<float>(kFundamentalBin) * static_cast<float>(kSampleRate) /
                              static_cast<float>(kNumSamples);

    const auto flat = renderSineTableWithWarp(0.0f, 0.0f, kSampleRate, kFreqHz, kWarmup + kNumSamples);
    const auto warped = renderSineTableWithWarp(0.75f, 0.0f, kSampleRate, kFreqHz, kWarmup + kNumSamples);

    const auto flatTail = std::vector<float>(flat.begin() + static_cast<std::ptrdiff_t>(kWarmup), flat.end());
    const auto warpedTail = std::vector<float>(warped.begin() + static_cast<std::ptrdiff_t>(kWarmup), warped.end());

    const double flatRatio = nonFundamentalEnergyRatio(flatTail, kFundamentalBin);
    const double warpedRatio = nonFundamentalEnergyRatio(warpedTail, kFundamentalBin);

    REQUIRE(warpedRatio > flatRatio * 1.05);
}

TEST_CASE("WavetableWarp asymmetry changes harmonic content when bend is active", "[wavetable][warp]")
{
    constexpr std::size_t kNumSamples = 8192;
    constexpr std::size_t kWarmup = 512;
    constexpr double kSampleRate = 48000.0;
    constexpr std::size_t kFundamentalBin = 40;
    constexpr float kFreqHz = static_cast<float>(kFundamentalBin) * static_cast<float>(kSampleRate) /
                              static_cast<float>(kNumSamples);

    const auto bendOnly = renderSineTableWithWarp(0.55f, 0.0f, kSampleRate, kFreqHz, kWarmup + kNumSamples);
    const auto bendAndAsym =
        renderSineTableWithWarp(0.55f, 0.75f, kSampleRate, kFreqHz, kWarmup + kNumSamples);

    const auto bendTail = std::vector<float>(bendOnly.begin() + static_cast<std::ptrdiff_t>(kWarmup), bendOnly.end());
    const auto asymTail =
        std::vector<float>(bendAndAsym.begin() + static_cast<std::ptrdiff_t>(kWarmup), bendAndAsym.end());

    const double bendRatio = nonFundamentalEnergyRatio(bendTail, kFundamentalBin);
    const double asymRatio = nonFundamentalEnergyRatio(asymTail, kFundamentalBin);

    REQUIRE(bendRatio > 0.05);
    REQUIRE(asymRatio != Catch::Approx(bendRatio).margin(0.005));
}

TEST_CASE("WavetableWarp asymmetry modulates bend displacement", "[wavetable][warp]")
{
    WtWarpParams bendOnly;
    bendOnly.bend = 0.6f;

    WtWarpParams bendAndAsym;
    bendAndAsym.bend = 0.6f;
    bendAndAsym.asymmetry = 0.8f;

    REQUIRE(warpReadPhase(0.125f, bendAndAsym) != Catch::Approx(warpReadPhase(0.125f, bendOnly)).margin(1.0e-4f));
}

TEST_CASE("WavetableWarp sync identity at zero amount", "[wavetable][warp]")
{
    WtWarpParams params;
    params.syncRatio = 4.0f;
    params.syncAmount = 0.0f;
    REQUIRE(warpReadPhase(0.25f, params) == Catch::Approx(0.25f));
}

TEST_CASE("WavetableWarp sync hard blend changes read phase", "[wavetable][warp]")
{
    WtWarpParams neutral;
    WtWarpParams synced;
    synced.syncRatio = 2.0f;
    synced.syncAmount = 1.0f;

    REQUIRE(warpReadPhase(0.25f, synced) != Catch::Approx(warpReadPhase(0.25f, neutral)).margin(1.0e-4f));
    REQUIRE(warpReadPhase(0.25f, synced) == Catch::Approx(0.5f).margin(1.0e-4f));
}

TEST_CASE("WavetableWarp sync increases sideband energy at ratio 2", "[wavetable][warp]")
{
    constexpr std::size_t kNumSamples = 8192;
    constexpr std::size_t kWarmup = 512;
    constexpr double kSampleRate = 48000.0;
    constexpr std::size_t kFundamentalBin = 40;
    constexpr float kFreqHz = static_cast<float>(kFundamentalBin) * static_cast<float>(kSampleRate) /
                              static_cast<float>(kNumSamples);

    const auto flat = renderSineTableWithWarp(0.0f, 0.0f, kSampleRate, kFreqHz, kWarmup + kNumSamples);

    WtWarpParams syncParams;
    syncParams.syncRatio = 2.0f;
    syncParams.syncAmount = 1.0f;

    std::vector<float> synced(kWarmup + kNumSamples);
    constexpr int kTableSize = 256;
    std::vector<float> table(static_cast<std::size_t>(kTableSize));
    for (int i = 0; i < kTableSize; ++i)
        table[static_cast<std::size_t>(i)] =
            std::sin(dsp::kTwoPi * static_cast<float>(i) / static_cast<float>(kTableSize));

    float phase = 0.0f;
    const float dt = kFreqHz / static_cast<float>(kSampleRate);
    for (std::size_t i = 0; i < synced.size(); ++i)
    {
        const float readPhase = warpReadPhase(phase, syncParams);
        const float sampleF = readPhase * static_cast<float>(kTableSize);
        const int idx0 = static_cast<int>(sampleF) % kTableSize;
        const int idx1 = (idx0 + 1) % kTableSize;
        const float frac = sampleF - std::floor(sampleF);
        synced[i] = dsp::lerp(table[static_cast<std::size_t>(idx0)], table[static_cast<std::size_t>(idx1)], frac);
        phase = dsp::wrapPhase(phase + dt);
    }

    const auto flatTail = std::vector<float>(flat.begin() + static_cast<std::ptrdiff_t>(kWarmup), flat.end());
    const auto syncedTail = std::vector<float>(synced.begin() + static_cast<std::ptrdiff_t>(kWarmup), synced.end());

    const double flatRatio = nonFundamentalEnergyRatio(flatTail, kFundamentalBin);
    const double syncedRatio = nonFundamentalEnergyRatio(syncedTail, kFundamentalBin);

    REQUIRE(syncedRatio > flatRatio * 1.05);
}

TEST_CASE("WavetableOscillator warp changes output vs identity", "[wavetable][warp]")
{
    constexpr int kTableSize = 256;
    std::vector<float> samples(static_cast<std::size_t>(kTableSize));
    for (int i = 0; i < kTableSize; ++i)
        samples[static_cast<std::size_t>(i)] =
            std::sin(dsp::kTwoPi * static_cast<float>(i) / static_cast<float>(kTableSize));

    WavetableView view;
    view.samples = samples.data();
    view.numFrames = 1;
    view.samplesPerFrame = kTableSize;

    WavetableOscillator osc;
    osc.prepare(48000.0);
    osc.setFrequency(440.0f);
    osc.reset(0.25f);

    WtWarpParams neutral;
    WtWarpParams bent;
    bent.bend = 0.75f;

    const float flat = osc.renderSample(view, 0.0f, 0.0f, neutral);
    osc.reset(0.25f);
    const float warped = osc.renderSample(view, 0.0f, 0.0f, bent);

    REQUIRE(flat != Catch::Approx(0.0f).margin(1.0e-4f));
    REQUIRE(warped != Catch::Approx(flat).margin(1.0e-4f));
}

TEST_CASE("WavetableOscillator asymmetry changes output when bend is active", "[wavetable][warp]")
{
    constexpr int kTableSize = 256;
    std::vector<float> samples(static_cast<std::size_t>(kTableSize));
    for (int i = 0; i < kTableSize; ++i)
        samples[static_cast<std::size_t>(i)] =
            std::sin(dsp::kTwoPi * static_cast<float>(i) / static_cast<float>(kTableSize));

    WavetableView view;
    view.samples = samples.data();
    view.numFrames = 1;
    view.samplesPerFrame = kTableSize;

    WavetableOscillator osc;
    osc.prepare(48000.0);
    osc.setFrequency(440.0f);
    osc.reset(0.125f);

    WtWarpParams bent;
    bent.bend = 0.75f;
    WtWarpParams bentAsym;
    bentAsym.bend = 0.75f;
    bentAsym.asymmetry = 0.75f;

    const float bendOnly = osc.renderSample(view, 0.0f, 0.0f, bent);
    osc.reset(0.125f);
    const float withAsym = osc.renderSample(view, 0.0f, 0.0f, bentAsym);

    REQUIRE(withAsym != Catch::Approx(bendOnly).margin(1.0e-4f));
}
