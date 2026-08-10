#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <complex>
#include <vector>

#include "pw8/dsp/Fft.hpp"
#include "pw8/dsp/HilbertTransformer.hpp"
#include "pw8/effects/EffectChain.hpp"

using namespace pw8;
using namespace pw8::effects;

namespace
{
    constexpr double kSampleRate = 48000.0;

    struct StereoSample
    {
        float l;
        float r;
    };

    template <typename Processor>
    std::vector<StereoSample> renderImpulseResponse(Processor& proc, const EffectSlotParams& p, int numSamples)
    {
        std::vector<StereoSample> out;
        out.reserve(static_cast<std::size_t>(numSamples));
        for (int i = 0; i < numSamples; ++i)
        {
            const float in = (i == 0) ? 1.0f : 0.0f;
            float outL = 0.0f, outR = 0.0f;
            proc.processStereo(in, in, p, outL, outR);
            out.push_back({outL, outR});
        }
        return out;
    }

    /// Index of the largest-magnitude sample within [from, to).
    std::size_t peakIndex(const std::vector<StereoSample>& samples, std::size_t from, std::size_t to, bool useLeft)
    {
        std::size_t best = from;
        float bestMag = 0.0f;
        for (std::size_t i = from; i < to && i < samples.size(); ++i)
        {
            const float mag = std::abs(useLeft ? samples[i].l : samples[i].r);
            if (mag > bestMag)
            {
                bestMag = mag;
                best = i;
            }
        }
        return best;
    }

    float magnitudeAt(const std::vector<StereoSample>& samples, std::size_t i, bool useLeft)
    {
        if (i >= samples.size())
            return 0.0f;
        return std::abs(useLeft ? samples[i].l : samples[i].r);
    }

    bool allFinite(const std::vector<StereoSample>& samples)
    {
        for (const auto& s : samples)
            if (!std::isfinite(s.l) || !std::isfinite(s.r))
                return false;
        return true;
    }
} // namespace

// ---------------------------------------------------------------------------
// Saturation
// ---------------------------------------------------------------------------

TEST_CASE("Saturation is a transparent passthrough at mix 0", "[effects][saturation]")
{
    SaturationProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Saturation;
    p.mix = 0.0f;
    p.saturationDriveDb = 24.0f;

    float outL, outR;
    proc.processStereo(0.9f, -0.9f, p, outL, outR);
    REQUIRE(outL == Catch::Approx(0.9f));
    REQUIRE(outR == Catch::Approx(-0.9f));
}

TEST_CASE("Saturation compresses a loud signal more than a quiet one", "[effects][saturation]")
{
    SaturationProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Saturation;
    p.mix = 1.0f;
    p.saturationDriveDb = 24.0f; // ~16x linear drive -- well into the tanh knee.

    float outLoudL, outLoudR, outQuietL, outQuietR;
    proc.processStereo(2.0f, 2.0f, p, outLoudL, outLoudR);
    proc.processStereo(0.05f, 0.05f, p, outQuietL, outQuietR);

    // A 40x increase in input (0.05 -> 2.0) should produce far less than a 40x
    // increase in output once saturation is biting -- that's the whole point.
    REQUIRE(outLoudL < 40.0f * outQuietL);
    REQUIRE(outLoudL <= 1.01f); // tanh never exceeds unity.
}

// ---------------------------------------------------------------------------
// Chorus
// ---------------------------------------------------------------------------

TEST_CASE("Chorus is a transparent passthrough at mix 0", "[effects][chorus]")
{
    ChorusProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Chorus;
    p.mix = 0.0f;

    float outL, outR;
    proc.processStereo(0.42f, -0.17f, p, outL, outR);
    REQUIRE(outL == Catch::Approx(0.42f));
    REQUIRE(outR == Catch::Approx(-0.17f));
}

TEST_CASE("Chorus with zero depth delays an impulse by its base delay time", "[effects][chorus]")
{
    ChorusProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Chorus;
    p.mix = 1.0f;
    p.chorusDepthMs = 0.0f; // no LFO modulation -- a fixed delay, exactly like a comb filter.
    p.chorusBaseDelayMs = 10.0f;

    const auto response = renderImpulseResponse(proc, p, 1000);
    const auto expectedSample = static_cast<std::size_t>(10.0 * 0.001 * kSampleRate);
    const auto peak = peakIndex(response, 0, response.size(), true);

    REQUIRE(peak >= expectedSample - 1);
    REQUIRE(peak <= expectedSample + 1);
}

// ---------------------------------------------------------------------------
// TapeDelay
// ---------------------------------------------------------------------------

TEST_CASE("TapeDelay Static mode produces decaying, evenly-spaced echoes", "[effects][tapedelay]")
{
    TapeDelayProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::TapeDelay;
    p.mix = 1.0f;
    p.tapeDelayMs = 20.0f;
    p.tapeFeedback = 0.5f;
    p.tapeDriveDb = 0.0f;
    p.tapeDriftDepthMs = 0.0f; // isolate spacing from wow/flutter for this test.
    p.tapePanMode = DelayPanMode::Static;

    const auto response = renderImpulseResponse(proc, p, 6000);
    const auto delaySamples = static_cast<std::size_t>(20.0 * 0.001 * kSampleRate);

    const auto echo1 = peakIndex(response, delaySamples / 2, delaySamples + delaySamples / 2, true);
    const auto echo2 = peakIndex(response, echo1 + delaySamples / 2, echo1 + delaySamples + delaySamples / 2, true);

    REQUIRE(echo1 >= delaySamples - 2);
    REQUIRE(echo1 <= delaySamples + 2);
    // Second echo lands roughly one more delay period after the first.
    REQUIRE(echo2 >= echo1 + delaySamples - 4);
    REQUIRE(echo2 <= echo1 + delaySamples + 4);
    // Feedback decay: each repeat is quieter than the last.
    REQUIRE(magnitudeAt(response, echo2, true) < magnitudeAt(response, echo1, true));
}

TEST_CASE("TapeDelay PingPong mode alternates the echo between channels", "[effects][tapedelay][pingpong]")
{
    TapeDelayProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::TapeDelay;
    p.mix = 1.0f;
    p.tapeDelayMs = 15.0f;
    p.tapeFeedback = 0.6f;
    p.tapeDriveDb = 0.0f;
    p.tapeDriftDepthMs = 0.0f;
    p.tapePanMode = DelayPanMode::PingPong;

    const auto response = renderImpulseResponse(proc, p, 4000);
    const auto delaySamples = static_cast<std::size_t>(15.0 * 0.001 * kSampleRate);

    // First repeat: left dominant. Second repeat (one delay period later): right dominant.
    const auto firstL = magnitudeAt(response, delaySamples, true);
    const auto firstR = magnitudeAt(response, delaySamples, false);
    const auto secondL = magnitudeAt(response, 2 * delaySamples, true);
    const auto secondR = magnitudeAt(response, 2 * delaySamples, false);

    REQUIRE(firstL > firstR);
    REQUIRE(secondR > secondL);
}

// ---------------------------------------------------------------------------
// NodeDelay
// ---------------------------------------------------------------------------

TEST_CASE("NodeDelay chains a child node's echo after its parent's", "[effects][nodedelay]")
{
    NodeDelayProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::NodeDelay;
    p.mix = 1.0f;
    p.nodeInsanity = 0.0f;

    for (auto& n : p.nodes)
        n.enabled = false;

    p.nodes[0] = DelayNodeParams{true, -1, 10.0f, 0.0f, 0.0f, 0.0f, 1.0f};  // root tap, 10ms, no self-feedback.
    p.nodes[1] = DelayNodeParams{true, 0, 15.0f, 0.0f, 0.0f, 0.0f, 1.0f};   // child of node 0, +15ms more.

    const auto response = renderImpulseResponse(proc, p, 3000);

    const auto d0 = static_cast<std::size_t>(10.0 * 0.001 * kSampleRate);
    const auto d1 = static_cast<std::size_t>(15.0 * 0.001 * kSampleRate);

    const auto echoAtNode0 = peakIndex(response, d0 - 5, d0 + 5, true);
    const auto echoAtNode1 = peakIndex(response, d0 + d1 - 5, d0 + d1 + 5, true);

    REQUIRE(magnitudeAt(response, echoAtNode0, true) > 0.05f);
    REQUIRE(magnitudeAt(response, echoAtNode1, true) > 0.05f); // proves node 1 heard node 0's output, not just the input.
}

TEST_CASE("NodeDelay a disabled node contributes nothing to the mix", "[effects][nodedelay]")
{
    EffectSlotParams p;
    p.type = EffectType::NodeDelay;
    p.mix = 1.0f;
    for (auto& n : p.nodes)
        n.enabled = false;
    p.nodes[0] = DelayNodeParams{false, -1, 10.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    NodeDelayProcessor proc;
    proc.prepare(kSampleRate);
    const auto response = renderImpulseResponse(proc, p, 2000);

    // Disabled means "excluded from the wet mix," not "silent output" -- the dry
    // signal (the impulse itself, and only it, since mix has nothing wet to add)
    // still passes straight through unchanged.
    for (std::size_t i = 0; i < response.size(); ++i)
    {
        const float expectedDry = (i == 0) ? 1.0f : 0.0f;
        REQUIRE(response[i].l == Catch::Approx(expectedDry).margin(1.0e-6f));
        REQUIRE(response[i].r == Catch::Approx(expectedDry).margin(1.0e-6f));
    }
}

// ---------------------------------------------------------------------------
// FrequencyShifter (the DSP primitive behind FreqShiftEcho)
// ---------------------------------------------------------------------------

/// Finds the dominant frequency in a windowed block via FFT magnitude peak --
/// robust to the residual image-band leakage an approximate (finite-order)
/// Hilbert transformer leaves behind, unlike a zero-crossing count, which that
/// leakage can visibly skew.
float estimateFrequencyByFftPeak(const std::vector<float>& signal, double sampleRate)
{
    const std::size_t n = signal.size();
    std::vector<std::complex<float>> data(n);
    for (std::size_t i = 0; i < n; ++i)
    {
        // Hann window to tighten the FFT peak (reduces spectral leakage).
        const float w = 0.5f - 0.5f * std::cos(2.0f * dsp::kPi * static_cast<float>(i) / static_cast<float>(n - 1));
        data[i] = std::complex<float>(signal[i] * w, 0.0f);
    }
    dsp::fft(data, false);

    std::size_t peakBin = 1;
    float peakMag = 0.0f;
    for (std::size_t bin = 1; bin < n / 2; ++bin)
    {
        const float mag = std::abs(data[bin]);
        if (mag > peakMag)
        {
            peakMag = mag;
            peakBin = bin;
        }
    }
    return static_cast<float>(peakBin) * static_cast<float>(sampleRate) / static_cast<float>(n);
}

TEST_CASE("FrequencyShifter shifts a pure tone's frequency by the requested amount", "[effects][freqshift]")
{
    dsp::FrequencyShifter shifter;
    shifter.prepare(kSampleRate);

    constexpr float kInputHz = 440.0f;
    constexpr float kShiftHz = 200.0f;
    constexpr int kNumSamples = 16384; // power of two, for the FFT.
    constexpr int kSettleSamples = 4000;

    std::vector<float> output;
    output.reserve(kSettleSamples + kNumSamples);
    for (int i = 0; i < kSettleSamples + kNumSamples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        const float input = std::sin(2.0f * dsp::kPi * kInputHz * t);
        output.push_back(shifter.process(input, kShiftHz));
    }

    // Discard the allpass cascade's settling time before analyzing.
    std::vector<float> settled(output.begin() + kSettleSamples, output.end());
    const float measuredHz = estimateFrequencyByFftPeak(settled, kSampleRate);

    // FFT bin resolution at 48kHz/16384 samples is ~2.9Hz; the Niemitalo wideband
    // design's phase error is small but nonzero, so a generous-but-meaningful
    // margin confirms a genuine +200Hz shift, not just "some peak moved."
    REQUIRE(measuredHz == Catch::Approx(kInputHz + kShiftHz).margin(20.0f));
}

TEST_CASE("FreqShiftEcho produces finite, bounded output from an impulse", "[effects][freqshiftecho]")
{
    FreqShiftEchoProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::FreqShiftEcho;
    p.mix = 1.0f;
    p.freqShiftHz = 30.0f;
    p.freqShiftFeedback = 0.7f;
    p.freqShiftDelayMs = 20.0f; // short delay so the impulse response fits a short test window.

    const auto delaySamples = static_cast<std::size_t>(20.0 * 0.001 * kSampleRate);
    const auto response = renderImpulseResponse(proc, p, static_cast<int>(delaySamples * 4));
    REQUIRE(allFinite(response));

    bool anyNonZeroAfterDelay = false;
    for (std::size_t i = delaySamples / 2; i < response.size(); ++i)
        if (std::abs(response[i].l) > 1.0e-4f)
            anyNonZeroAfterDelay = true;
    REQUIRE(anyNonZeroAfterDelay);
}

// ---------------------------------------------------------------------------
// FractalEcho (the invented algorithm)
// ---------------------------------------------------------------------------

TEST_CASE("FractalEcho topology generation is deterministic for a given seed", "[effects][fractalecho][determinism]")
{
    EffectSlotParams p;
    p.fractalSeedA = 777;
    p.fractalSeedB = 888;
    p.fractalBaseDelayMs = 180.0f;
    p.fractalRatio = 0.6f;
    p.fractalSpreadMs = 10.0f;

    FractalEchoProcessor a, b;
    a.prepare(kSampleRate);
    b.prepare(kSampleRate);
    a.regenerateForTest(p);
    b.regenerateForTest(p);

    for (std::size_t i = 0; i < kMaxDelayNodes; ++i)
    {
        REQUIRE(a.topologyA()[i].delayMs == Catch::Approx(b.topologyA()[i].delayMs));
        REQUIRE(a.topologyA()[i].feedback == Catch::Approx(b.topologyA()[i].feedback));
        REQUIRE(a.topologyB()[i].pan == Catch::Approx(b.topologyB()[i].pan));
    }
}

TEST_CASE("FractalEcho's two seeds generate different topologies", "[effects][fractalecho]")
{
    EffectSlotParams p;
    p.fractalSeedA = 1;
    p.fractalSeedB = 2;
    p.fractalBaseDelayMs = 180.0f;
    p.fractalRatio = 0.6f;
    p.fractalSpreadMs = 10.0f;

    FractalEchoProcessor proc;
    proc.prepare(kSampleRate);
    proc.regenerateForTest(p);

    bool anyDifferent = false;
    for (std::size_t i = 0; i < kMaxDelayNodes; ++i)
        if (std::abs(proc.topologyA()[i].delayMs - proc.topologyB()[i].delayMs) > 0.01f)
            anyDifferent = true;
    REQUIRE(anyDifferent);
}

TEST_CASE("FractalEcho's self-similar depth scaling makes deeper nodes' base delay shorter", "[effects][fractalecho]")
{
    EffectSlotParams p;
    p.fractalSeedA = 42;
    p.fractalSeedB = 43;
    p.fractalBaseDelayMs = 200.0f;
    p.fractalRatio = 0.5f; // each depth halves the base delay.
    p.fractalSpreadMs = 0.0f; // no jitter -- isolate the depth-scaling rule.

    FractalEchoProcessor proc;
    proc.prepare(kSampleRate);
    proc.regenerateForTest(p);

    // Node 0 = depth 0 (base delay), node 1 = depth 1 (child), node 3 = depth 2 (grandchild).
    REQUIRE(proc.topologyA()[0].delayMs == Catch::Approx(200.0f).margin(0.5f));
    REQUIRE(proc.topologyA()[1].delayMs == Catch::Approx(100.0f).margin(0.5f));
    REQUIRE(proc.topologyA()[3].delayMs == Catch::Approx(50.0f).margin(0.5f));
}

TEST_CASE("FractalEcho stays finite and bounded while morph sweeps continuously from 0 to 1", "[effects][fractalecho]")
{
    FractalEchoProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::FractalEcho;
    p.mix = 1.0f;
    p.fractalSeedA = 5;
    p.fractalSeedB = 999;

    constexpr int kNumSamples = 20000;
    for (int i = 0; i < kNumSamples; ++i)
    {
        p.fractalMorph = static_cast<float>(i) / static_cast<float>(kNumSamples - 1);
        const float in = (i % 4800 == 0) ? 0.8f : 0.0f; // periodic impulses to keep the tree fed.
        float outL, outR;
        proc.processStereo(in, in, p, outL, outR);
        REQUIRE(std::isfinite(outL));
        REQUIRE(std::isfinite(outR));
        REQUIRE(std::abs(outL) < 10.0f);
        REQUIRE(std::abs(outR) < 10.0f);
    }
}

// ---------------------------------------------------------------------------
// EffectChain
// ---------------------------------------------------------------------------

TEST_CASE("EffectChain with every slot Bypass is a transparent passthrough", "[effects][chain]")
{
    EffectChain<3> chain;
    chain.prepare(kSampleRate);

    std::array<EffectSlotParams, 3> params{}; // all default-constructed Bypass.
    float l = 0.37f, r = -0.81f;
    chain.process(params, l, r);

    REQUIRE(l == Catch::Approx(0.37f));
    REQUIRE(r == Catch::Approx(-0.81f));
}

TEST_CASE("EffectChain runs slots in series", "[effects][chain]")
{
    EffectChain<2> chain;
    chain.prepare(kSampleRate);

    std::array<EffectSlotParams, 2> params{};
    params[0].type = EffectType::Saturation;
    params[0].mix = 1.0f;
    params[0].saturationDriveDb = 12.0f;
    params[1].type = EffectType::Bypass;

    float l = 1.5f, r = 1.5f;
    chain.process(params, l, r);

    // Slot 1 (Bypass) shouldn't undo slot 0's saturation -- output must still be compressed.
    REQUIRE(l < 1.5f);
    REQUIRE(l > 0.0f);
}

// ---------------------------------------------------------------------------
// Reverb / Eq / Compressor / Limiter (GATE 10 -- docs/ROADMAP.md)
// ---------------------------------------------------------------------------

namespace
{
    /// RMS of a steady-state sine tone through `proc`, discarding the first half
    /// (settling time) -- the same measurement pattern StateVariableFilterTests.cpp
    /// already established for a per-sample DSP block.
    template <typename Processor>
    float measureToneRms(Processor& proc, const EffectSlotParams& p, float toneHz, float inputAmplitude,
                          double seconds)
    {
        const auto numSamples = static_cast<int>(seconds * kSampleRate);
        double sumSq = 0.0;
        int counted = 0;
        for (int i = 0; i < numSamples; ++i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
            const float in = inputAmplitude * std::sin(2.0f * dsp::kPi * toneHz * t);
            float outL = 0.0f, outR = 0.0f;
            proc.processStereo(in, in, p, outL, outR);
            if (i > numSamples / 2)
            {
                sumSq += static_cast<double>(outL) * outL;
                ++counted;
            }
        }
        return static_cast<float>(std::sqrt(sumSq / counted));
    }
} // namespace

TEST_CASE("Eq low shelf boost measurably raises RMS of a tone below the shelf frequency", "[effects][eq]")
{
    EqProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams flat;
    flat.type = EffectType::Eq;
    flat.mix = 1.0f;
    flat.eqLowFreqHz = 200.0f;
    flat.eqLowGainDb = 0.0f;

    EqProcessor proc2;
    proc2.prepare(kSampleRate);
    EffectSlotParams boosted = flat;
    boosted.eqLowGainDb = 12.0f;

    const float rmsFlat = measureToneRms(proc, flat, 60.0f, 0.3f, 0.05);
    const float rmsBoosted = measureToneRms(proc2, boosted, 60.0f, 0.3f, 0.05);

    // +12dB is a factor of ~3.98x in amplitude/RMS.
    REQUIRE(rmsBoosted > rmsFlat * 2.5f);
}

TEST_CASE("Eq mid peaking cut measurably lowers RMS of a tone at the peak frequency", "[effects][eq]")
{
    EqProcessor flatProc, cutProc;
    flatProc.prepare(kSampleRate);
    cutProc.prepare(kSampleRate);

    EffectSlotParams flat;
    flat.type = EffectType::Eq;
    flat.mix = 1.0f;
    flat.eqMidFreqHz = 1000.0f;
    flat.eqMidGainDb = 0.0f;
    flat.eqMidQ = 1.0f;

    EffectSlotParams cut = flat;
    cut.eqMidGainDb = -18.0f;

    const float rmsFlat = measureToneRms(flatProc, flat, 1000.0f, 0.3f, 0.05);
    const float rmsCut = measureToneRms(cutProc, cut, 1000.0f, 0.3f, 0.05);

    REQUIRE(rmsCut < rmsFlat * 0.3f);
}

TEST_CASE("Eq is a transparent passthrough at mix 0", "[effects][eq]")
{
    EqProcessor proc;
    proc.prepare(kSampleRate);
    EffectSlotParams p;
    p.type = EffectType::Eq;
    p.mix = 0.0f;
    p.eqLowGainDb = 12.0f;
    p.eqMidGainDb = -18.0f;
    p.eqHighGainDb = 12.0f;

    float outL, outR;
    proc.processStereo(0.5f, -0.3f, p, outL, outR);
    REQUIRE(outL == Catch::Approx(0.5f));
    REQUIRE(outR == Catch::Approx(-0.3f));
}

TEST_CASE("Compressor reduces a loud tone's level by roughly the expected amount above threshold",
          "[effects][compressor]")
{
    CompressorProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Compressor;
    p.mix = 1.0f;
    p.compThresholdDb = -18.0f;
    p.compRatio = 4.0f;
    p.compAttackMs = 1.0f;
    p.compReleaseMs = 50.0f;
    p.compKneeDb = 0.0f; // hard knee -- exact expected reduction is simple to compute.
    p.compMakeupDb = 0.0f;

    // A full-scale (amplitude 1.0) sine's peak is 0dBFS -- 18dB above the threshold.
    // Hard-knee 4:1 above threshold: reduction = (1/4 - 1) * 18 = -13.5dB.
    const float rmsCompressed = measureToneRms(proc, p, 200.0f, 1.0f, 0.3);

    CompressorProcessor bypassProc;
    bypassProc.prepare(kSampleRate);
    EffectSlotParams bypassParams = p;
    bypassParams.mix = 0.0f;
    const float rmsUncompressed = measureToneRms(bypassProc, bypassParams, 200.0f, 1.0f, 0.3);

    const float measuredReductionDb = dsp::gainToDb(rmsCompressed / rmsUncompressed);
    REQUIRE(measuredReductionDb == Catch::Approx(-13.5f).margin(1.5f));
}

TEST_CASE("Compressor leaves a quiet tone below threshold essentially untouched", "[effects][compressor]")
{
    CompressorProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Compressor;
    p.mix = 1.0f;
    p.compThresholdDb = -6.0f;
    p.compRatio = 4.0f;
    p.compKneeDb = 0.0f;

    const float rmsCompressed = measureToneRms(proc, p, 200.0f, 0.05f, 0.1); // well below -6dBFS.

    CompressorProcessor bypassProc;
    bypassProc.prepare(kSampleRate);
    EffectSlotParams bypassParams = p;
    bypassParams.mix = 0.0f;
    const float rmsUncompressed = measureToneRms(bypassProc, bypassParams, 200.0f, 0.05f, 0.1);

    REQUIRE(rmsCompressed == Catch::Approx(rmsUncompressed).margin(0.002f));
}

TEST_CASE("Limiter never lets a sustained loud tone's output exceed its ceiling", "[effects][limiter]")
{
    LimiterProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Limiter;
    p.mix = 1.0f;
    p.limiterCeilingDb = -1.0f;
    p.limiterLookaheadMs = 5.0f;
    p.limiterReleaseMs = 50.0f;

    const float ceilingLinear = dsp::dbToGain(p.limiterCeilingDb);

    constexpr int kNumSamples = 24000; // 0.5s.
    float maxOutputAbs = 0.0f;
    for (int i = 0; i < kNumSamples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        // A tone well above the ceiling, plus a sharp transient partway through --
        // exactly what a limiter's lookahead exists to catch without overshoot.
        float in = 2.5f * std::sin(2.0f * dsp::kPi * 220.0f * t);
        if (i == kNumSamples / 2)
            in = 8.0f;
        float outL = 0.0f, outR = 0.0f;
        proc.processStereo(in, in, p, outL, outR);
        // Skip the very first lookahead window -- the ring buffer hasn't seen a
        // full window of real signal yet, a known, documented startup edge case
        // (see LimiterProcessor's header comment), not a violation of the
        // steady-state no-overshoot guarantee this test targets.
        if (i > 1000)
            maxOutputAbs = std::max(maxOutputAbs, std::abs(outL));
    }

    REQUIRE(maxOutputAbs <= ceilingLinear * 1.01f); // 1% numerical-precision margin.
}

TEST_CASE("Limiter at mix 0 outputs exactly the delayed input, undistorted, regardless of gain reduction",
          "[effects][limiter]")
{
    // mix=0 means outL = lerp(delayedL, delayedL*gain, 0) == delayedL exactly, even
    // while a loud signal elsewhere is driving real gain reduction -- proving mix
    // actually gates the limiting rather than just being cosmetically wired.
    LimiterProcessor proc;
    proc.prepare(kSampleRate);
    EffectSlotParams p;
    p.type = EffectType::Limiter;
    p.mix = 0.0f;
    p.limiterCeilingDb = -6.0f; // a low ceiling -- if mix were doing nothing, this would obviously clip the loud tone.

    constexpr int kNumSamples = 4800;
    std::vector<float> inputs(kNumSamples);
    std::vector<float> outputs(kNumSamples);
    for (int i = 0; i < kNumSamples; ++i)
    {
        const float t = static_cast<float>(i) / static_cast<float>(kSampleRate);
        inputs[static_cast<std::size_t>(i)] = 3.0f * std::sin(2.0f * dsp::kPi * 440.0f * t); // well above the ceiling.
        float outL = 0.0f, outR = 0.0f;
        proc.processStereo(inputs[static_cast<std::size_t>(i)], inputs[static_cast<std::size_t>(i)], p, outL, outR);
        outputs[static_cast<std::size_t>(i)] = outL;
    }

    // The output stream is just the input stream shifted by the lookahead delay --
    // find that shift empirically (cross-correlation would be overkill; a direct
    // search for the best-matching lag is enough here) and confirm the match is
    // exact, not attenuated.
    int bestLag = -1;
    float bestError = 1.0e9f;
    for (int lag = 1; lag < 1000; ++lag)
    {
        float errorSum = 0.0f;
        for (int i = lag; i < lag + 200 && i < kNumSamples; ++i)
            errorSum += std::abs(outputs[static_cast<std::size_t>(i)] - inputs[static_cast<std::size_t>(i - lag)]);
        if (errorSum < bestError)
        {
            bestError = errorSum;
            bestLag = lag;
        }
    }
    REQUIRE(bestLag > 0);
    REQUIRE(bestError < 0.01f); // near-zero cumulative error across 200 samples -- an exact, undistorted match.
}

TEST_CASE("Reverb produces a decaying tail after an impulse, not silence or a static hold", "[effects][reverb]")
{
    ReverbProcessor proc;
    proc.prepare(kSampleRate);

    EffectSlotParams p;
    p.type = EffectType::Reverb;
    p.mix = 1.0f;
    p.reverbSizeParam = 1.0f;
    p.reverbDecaySeconds = 1.0f;
    p.reverbDampingHz = 6000.0f;
    p.reverbPreDelayMs = 0.0f;

    const auto response = renderImpulseResponse(proc, p, static_cast<int>(1.5 * kSampleRate));
    REQUIRE(allFinite(response));

    auto windowedRms = [&](std::size_t from, std::size_t to) {
        double sumSq = 0.0;
        for (std::size_t i = from; i < to && i < response.size(); ++i)
            sumSq += static_cast<double>(response[i].l) * response[i].l;
        return std::sqrt(sumSq / static_cast<double>(to - from));
    };

    const double early = windowedRms(static_cast<std::size_t>(0.05 * kSampleRate), static_cast<std::size_t>(0.15 * kSampleRate));
    const double late = windowedRms(static_cast<std::size_t>(1.0 * kSampleRate), static_cast<std::size_t>(1.4 * kSampleRate));

    REQUIRE(early > 0.0001); // a real tail exists shortly after the impulse.
    REQUIRE(late < early);   // and it decays -- not a static hold or runaway feedback.
    REQUIRE(late > 0.0);     // but hasn't already gone completely silent by 1s into a 1s-decay reverb.
}

TEST_CASE("Reverb is a transparent passthrough at mix 0", "[effects][reverb]")
{
    ReverbProcessor proc;
    proc.prepare(kSampleRate);
    EffectSlotParams p;
    p.type = EffectType::Reverb;
    p.mix = 0.0f;

    float outL, outR;
    proc.processStereo(0.4f, -0.2f, p, outL, outR);
    REQUIRE(outL == Catch::Approx(0.4f));
    REQUIRE(outR == Catch::Approx(-0.2f));
}
