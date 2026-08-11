#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/noise/NoiseSource.hpp"
#include "pw8/operator/OperatorNode.hpp"

using namespace pw8;
using namespace pw8::noise;

namespace
{
    std::vector<float> renderNoise(NoiseVariant variant, float rateHz, std::uint64_t seed, double sampleRate,
                                    std::size_t numSamples)
    {
        NoiseSource src;
        src.prepare(sampleRate);
        src.reset(seed);

        NoiseSourceParams params;
        params.variant = variant;
        params.rateHz = rateHz;

        std::vector<float> out(numSamples);
        for (std::size_t i = 0; i < numSamples; ++i)
            out[i] = src.renderSample(params);
        return out;
    }

    /// Sum of squared FFT magnitude in a low band (bins [1, band]) divided by the
    /// same sum in a high band near Nyquist (bins [half-band, half-1]) -- a coarse,
    /// comparative measure of spectral tilt. Not an absolute dB/octave figure (the
    /// two bands aren't a clean single octave apart), but ordinally correct for
    /// comparing how tilted one signal's spectrum is relative to another's, which is
    /// all the property tests below need.
    double lowToHighEnergyRatio(const std::vector<float>& samples)
    {
        const std::size_t n = samples.size();
        std::vector<std::complex<float>> data(n);
        for (std::size_t i = 0; i < n; ++i)
            data[i] = std::complex<float>(samples[i], 0.0f);
        dsp::fft(data, false);

        const std::size_t half = n / 2;
        const std::size_t band = half / 8;
        double lowEnergy = 0.0;
        double highEnergy = 0.0;
        for (std::size_t k = 1; k <= band; ++k)
            lowEnergy += std::norm(data[k]);
        for (std::size_t k = half - band; k < half; ++k)
            highEnergy += std::norm(data[k]);
        return lowEnergy / std::max(highEnergy, 1.0e-9);
    }
} // namespace

TEST_CASE("NoiseSource: white/pink/brown/blue stay bounded and finite", "[noise][stability]")
{
    const double sampleRate = 48000.0;
    for (auto variant : {NoiseVariant::White, NoiseVariant::Pink, NoiseVariant::Brown, NoiseVariant::Blue,
                          NoiseVariant::SampleAndHold, NoiseVariant::SmoothRandom, NoiseVariant::Dust})
    {
        for (float rate : {0.5f, 200.0f, 2000.0f})
        {
            const auto samples = renderNoise(variant, rate, 0x1234ABCDULL, sampleRate, 96000);
            for (float s : samples)
            {
                REQUIRE(std::isfinite(s));
                REQUIRE(std::abs(s) <= 1.0001f);
            }
        }
    }
}

TEST_CASE("NoiseSource: reset() with the same seed reproduces the exact same stream", "[noise][determinism]")
{
    const auto a = renderNoise(NoiseVariant::Pink, 200.0f, 0xC0FFEEULL, 48000.0, 4096);
    const auto b = renderNoise(NoiseVariant::Pink, 200.0f, 0xC0FFEEULL, 48000.0, 4096);
    REQUIRE(a == b);

    const auto c = renderNoise(NoiseVariant::Pink, 200.0f, 0xDEADBEEFULL, 48000.0, 4096);
    REQUIRE(a != c); // different seed -- vanishingly unlikely to collide by chance.
}

TEST_CASE("NoiseSource: pink noise's low/high energy ratio measurably exceeds white noise's flat response",
          "[noise][spectral][measured]")
{
    // 65536 samples gives enough low-frequency FFT resolution for the pink filter's
    // slope to show up clearly against white's flat spectrum.
    constexpr std::size_t n = 65536;
    const auto white = renderNoise(NoiseVariant::White, 200.0f, 0x1111ULL, 48000.0, n);
    const auto pink = renderNoise(NoiseVariant::Pink, 200.0f, 0x1111ULL, 48000.0, n);

    const double whiteRatio = lowToHighEnergyRatio(white);
    const double pinkRatio = lowToHighEnergyRatio(pink);

    // White noise is flat by construction -- its measured ratio should sit near 1
    // (loose bound: FFT bin variance on a finite noise realization is real, this
    // isn't asserting an exact 1.0). Pink noise's -3dB/octave slope should measure
    // as a substantially larger low/high ratio.
    REQUIRE(whiteRatio < 3.0);
    REQUIRE(pinkRatio > whiteRatio * 5.0);
}

TEST_CASE("NoiseSource: brown noise tilts even further toward low frequencies than pink, blue the opposite way",
          "[noise][spectral][measured]")
{
    constexpr std::size_t n = 65536;
    const auto pink = renderNoise(NoiseVariant::Pink, 200.0f, 0x2222ULL, 48000.0, n);
    const auto brown = renderNoise(NoiseVariant::Brown, 200.0f, 0x2222ULL, 48000.0, n);
    const auto white = renderNoise(NoiseVariant::White, 200.0f, 0x2222ULL, 48000.0, n);
    const auto blue = renderNoise(NoiseVariant::Blue, 200.0f, 0x2222ULL, 48000.0, n);

    // Brown/red is a steeper -6dB/octave lowpass slope vs. pink's -3dB/octave --
    // measurably more low-frequency-heavy.
    REQUIRE(lowToHighEnergyRatio(brown) > lowToHighEnergyRatio(pink));
    // Blue is white's highpass mirror image (+3dB/octave) -- measurably LESS
    // low-frequency-heavy than flat white noise.
    REQUIRE(lowToHighEnergyRatio(blue) < lowToHighEnergyRatio(white));
}

TEST_CASE("NoiseSource: Sample & Hold retargets at approximately the configured rate", "[noise][rate]")
{
    const double sampleRate = 48000.0;
    const float rateHz = 100.0f;
    const double seconds = 4.0;
    const auto samples = renderNoise(NoiseVariant::SampleAndHold, rateHz, 0x3333ULL, sampleRate, static_cast<std::size_t>(sampleRate * seconds));

    int changes = 0;
    for (std::size_t i = 1; i < samples.size(); ++i)
        if (samples[i] != samples[i - 1])
            ++changes;

    const double expected = rateHz * seconds;
    REQUIRE(static_cast<double>(changes) == Catch::Approx(expected).margin(expected * 0.15));
}

TEST_CASE("NoiseSource: Dust impulse density tracks the configured rate", "[noise][rate]")
{
    const double sampleRate = 48000.0;
    const float rateHz = 300.0f;
    const double seconds = 4.0;
    const auto samples = renderNoise(NoiseVariant::Dust, rateHz, 0x4444ULL, sampleRate, static_cast<std::size_t>(sampleRate * seconds));

    int impulses = 0;
    for (float s : samples)
        if (s != 0.0f)
            ++impulses;

    const double expected = rateHz * seconds;
    REQUIRE(static_cast<double>(impulses) == Catch::Approx(expected).margin(expected * 0.25));
}

TEST_CASE("OperatorState renders NoiseChaos through the full operator path, ignoring pitch", "[noise][operator]")
{
    op::OperatorParams params;
    params.engine = algorithm::EngineType::NoiseChaos;
    params.noiseVariant = static_cast<float>(NoiseVariant::White);
    params.noiseRate = 200.0f;
    params.level = 1.0f;

    const auto seed = dsp::DeterministicRng::deriveSeed(42, 0, 0);

    op::OperatorState constantPitch;
    constantPitch.prepare(48000.0);
    constantPitch.seedNoise(seed);

    op::OperatorState wildPitch;
    wildPitch.prepare(48000.0);
    wildPitch.seedNoise(seed);

    bool sawNonZero = false;
    for (int i = 0; i < 4096; ++i)
    {
        const float a = constantPitch.render(params, nullptr, 440.0f, 0.0f, 0.0f);
        // baseFrequencyHz varies wildly per sample here -- NoiseChaos's output must
        // not depend on it (see the render() case's own comment on why carrierHz is
        // computed but unused for this engine).
        const float wildHz = 20.0f + static_cast<float>(i % 50) * 731.0f;
        const float b = wildPitch.render(params, nullptr, wildHz, 0.0f, 0.0f);
        REQUIRE(std::isfinite(a));
        REQUIRE(a == b);
        if (a != 0.0f)
            sawNonZero = true;
    }
    REQUIRE(sawNonZero);
}
