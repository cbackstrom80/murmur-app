#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>

#include "pw8/dsp/Fft.hpp"
#include "pw8/oscillator/WavetableTable.hpp"

using namespace pw8::oscillator;

namespace
{
    constexpr int kSamplesPerFrame = 2048;

    /// Builds a harmonically rich (sawtooth-like, amplitude 1/k) single frame,
    /// truncated to `maxHarmonic`, without going through the builder tool's FFT path
    /// (this is a from-scratch synthesis, used only to set up test fixtures).
    std::vector<float> synthesizeFrame(int maxHarmonic)
    {
        std::vector<float> frame(kSamplesPerFrame, 0.0f);
        for (int k = 1; k <= maxHarmonic; ++k)
        {
            const float amp = 1.0f / static_cast<float>(k);
            for (int i = 0; i < kSamplesPerFrame; ++i)
                frame[static_cast<std::size_t>(i)] +=
                    amp * std::sin(2.0f * 3.14159265f * static_cast<float>(k) * static_cast<float>(i) / kSamplesPerFrame);
        }
        float peak = 1.0e-9f;
        for (float s : frame)
            peak = std::max(peak, std::abs(s));
        for (float& s : frame)
            s /= peak;
        return frame;
    }

    WavetableTable makeMultiMipSawTable()
    {
        WavetableTable table;
        table.numFrames = 1;
        table.samplesPerFrame = kSamplesPerFrame;

        for (int maxHarmonic : {1023, 511, 255, 127, 63, 31, 15, 7, 3, 1})
        {
            WavetableTable::MipLevel mip;
            mip.maxHarmonic = maxHarmonic;
            mip.samples = synthesizeFrame(maxHarmonic);
            table.mips.push_back(std::move(mip));
        }
        return table;
    }

    /// Renders `numSamples` of the oscillator reading `view` at `freqHz`/`sampleRate`
    /// (a minimal, direct table read -- not going through WavetableOscillator, to
    /// keep this test independent of that class) and returns the FFT magnitude
    /// spectrum of the result.
    std::vector<float> renderSpectrum(const WavetableView& view, float freqHz, double sampleRate, int numSamples)
    {
        std::vector<std::complex<float>> signal(static_cast<std::size_t>(numSamples));
        float phase = 0.0f;
        const float dt = static_cast<float>(freqHz / sampleRate);
        for (int i = 0; i < numSamples; ++i)
        {
            const float sampleF = phase * static_cast<float>(view.samplesPerFrame);
            const int idx0 = static_cast<int>(sampleF) % view.samplesPerFrame;
            signal[static_cast<std::size_t>(i)] = std::complex<float>(view.sampleAt(0, idx0), 0.0f);
            phase += dt;
            if (phase >= 1.0f) phase -= 1.0f;
        }
        pw8::dsp::fft(signal, false);
        std::vector<float> mag(static_cast<std::size_t>(numSamples) / 2);
        for (std::size_t i = 0; i < mag.size(); ++i)
            mag[i] = std::abs(signal[i]);
        return mag;
    }

    /// Fraction of total spectral energy that falls OUTSIDE a small window around the
    /// fundamental's bin -- a proxy for aliasing/spectral-smearing energy.
    float outOfBandEnergyRatio(const std::vector<float>& spectrum, int fundamentalBin)
    {
        double total = 0.0, inBand = 0.0;
        for (std::size_t bin = 0; bin < spectrum.size(); ++bin)
        {
            const double e = static_cast<double>(spectrum[bin]) * spectrum[bin];
            total += e;
            if (std::abs(static_cast<int>(bin) - fundamentalBin) <= 2)
                inBand += e;
        }
        return total > 1.0e-12 ? static_cast<float>(1.0 - inBand / total) : 0.0f;
    }
} // namespace

TEST_CASE("WavetableTable::viewForFrequency picks a more band-limited mip at higher note frequencies", "[wavetable][mipmap]")
{
    const auto table = makeMultiMipSawTable();
    const double sampleRate = 48000.0;

    const auto lowNoteView = table.viewForFrequency(110.0f, sampleRate);   // A2 -- should get a high/full-harmonic mip.
    const auto highNoteView = table.viewForFrequency(8000.0f, sampleRate); // very high -- should get a heavily truncated mip.

    // Identify which mip each view came from by matching the data pointer.
    auto findMaxHarmonic = [&](const WavetableView& v) {
        for (const auto& mip : table.mips)
            if (mip.samples.data() == v.samples)
                return mip.maxHarmonic;
        return -1;
    };

    const int lowNoteHarmonic = findMaxHarmonic(lowNoteView);
    const int highNoteHarmonic = findMaxHarmonic(highNoteView);

    REQUIRE(lowNoteHarmonic > 0);
    REQUIRE(highNoteHarmonic > 0);
    REQUIRE(highNoteHarmonic < lowNoteHarmonic);
}

TEST_CASE("WavetableTable mip selection measurably reduces aliasing energy vs. always using the full-bandwidth mip",
          "[wavetable][mipmap][aliasing]")
{
    const auto table = makeMultiMipSawTable();
    const double sampleRate = 48000.0;
    const float highFreq = 9000.0f; // aggressive: harmonic 2 alone (18kHz) is near Nyquist*0.9.
    constexpr int kFftSize = 4096;

    const WavetableView fullBandwidthView{table.mips.front().samples.data(), 1, kSamplesPerFrame};
    const WavetableView mipSelectedView = table.viewForFrequency(highFreq, sampleRate);

    const int fundamentalBin = static_cast<int>(std::lround(highFreq / (sampleRate / kFftSize)));

    const auto spectrumNoMip = renderSpectrum(fullBandwidthView, highFreq, sampleRate, kFftSize);
    const auto spectrumMipped = renderSpectrum(mipSelectedView, highFreq, sampleRate, kFftSize);

    const float aliasingNoMip = outOfBandEnergyRatio(spectrumNoMip, fundamentalBin);
    const float aliasingMipped = outOfBandEnergyRatio(spectrumMipped, fundamentalBin);

    INFO("out-of-band energy ratio without mip selection: " << aliasingNoMip);
    INFO("out-of-band energy ratio with mip selection: " << aliasingMipped);
    REQUIRE(aliasingMipped < aliasingNoMip * 0.5f); // at least 2x cleaner -- mip-mapping is doing real work.
}
