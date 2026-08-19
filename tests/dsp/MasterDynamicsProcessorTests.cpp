#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/dynamics/MasterDynamicsProcessor.hpp"

using namespace pw8::dynamics;

TEST_CASE("MasterDynamicsProcessor follower ducks on sidechain", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = true;
    params.mode = MasterDynamicsMode::Follower;
    params.sidechainGain = 1.0f;
    params.attackMs = 1.0f;
    params.releaseMs = 20.0f;
    params.mix = 1.0f;

    float left = 0.8f;
    float right = 0.8f;
    for (int i = 0; i < 4800; ++i)
    {
        left = 0.8f;
        right = 0.8f;
        proc.processSample(left, right, 0.9f, 0.9f, false, params);
    }

    REQUIRE(left < 0.8f);
    REQUIRE(proc.getGainReductionDb() < -0.1f);
    REQUIRE(proc.getSidechainEnvelope() > 0.1f);
}

TEST_CASE("MasterDynamicsProcessor envelope swells on gate trigger", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = true;
    params.mode = MasterDynamicsMode::Envelope;
    params.attackMs = 2.0f;
    params.releaseMs = 800.0f;
    params.mix = 1.0f;

    float left = 0.5f;
    float right = 0.5f;
    proc.processSample(left, right, 0.0f, 0.0f, true, params);
    for (int i = 0; i < 80; ++i)
    {
        left = 0.5f;
        right = 0.5f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }

    REQUIRE(left > 0.2f);
}

TEST_CASE("MasterDynamicsProcessor bypass when disabled", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = false;
    params.mode = MasterDynamicsMode::Compressor;

    float left = 0.42f;
    float right = -0.33f;
    proc.processSample(left, right, 1.0f, 1.0f, true, params);

    REQUIRE(left == Catch::Approx(0.42f));
    REQUIRE(right == Catch::Approx(-0.33f));
    REQUIRE(proc.getGainReductionDb() == Catch::Approx(0.0f));
}

TEST_CASE("MasterDynamicsProcessor vactrol slow release after gate", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = true;
    params.mode = MasterDynamicsMode::Vactrol;
    params.attackMs = 2.0f;
    params.vactrolSlewMs = 200.0f;
    params.mix = 1.0f;

    float left = 0.5f;
    float right = 0.5f;
    proc.processSample(left, right, 0.0f, 0.0f, true, params);
    for (int i = 0; i < 120; ++i)
    {
        left = 0.5f;
        right = 0.5f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }
    const float peakGain = left;

    for (int i = 0; i < 2400; ++i)
    {
        left = 0.5f;
        right = 0.5f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }

    REQUIRE(peakGain > 0.4f);
    REQUIRE(left > 0.05f);
    REQUIRE(left < peakGain);
}

TEST_CASE("MasterDynamicsProcessor compressor reduces hot peaks", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = true;
    params.mode = MasterDynamicsMode::Compressor;
    params.thresholdDb = -12.0f;
    params.ratio = 4.0f;
    params.attackMs = 0.5f;
    params.releaseMs = 50.0f;
    params.makeupDb = 0.0f;
    params.mix = 1.0f;

    float left = 0.95f;
    float right = 0.95f;
    for (int i = 0; i < 4800; ++i)
    {
        left = 0.95f;
        right = 0.95f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }

    REQUIRE(left < 0.95f);
    REQUIRE(proc.getGainReductionDb() < -0.5f);
}

TEST_CASE("MasterDynamicsProcessor vactrol swells with slower opto release", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = true;
    params.mode = MasterDynamicsMode::Vactrol;
    params.attackMs = 2.0f;
    params.vactrolSlewMs = 400.0f;
    params.mix = 1.0f;

    float left = 0.4f;
    float right = 0.4f;
    proc.processSample(left, right, 0.0f, 0.0f, true, params);
    for (int i = 0; i < 80; ++i)
    {
        left = 0.4f;
        right = 0.4f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }
    REQUIRE(left > 0.15f);

    for (int i = 0; i < 200; ++i)
    {
        left = 0.4f;
        right = 0.4f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }
    REQUIRE(left > 0.05f);
}

TEST_CASE("MasterDynamicsProcessor compressor reduces hot program material", "[dsp][dynamics]")
{
    MasterDynamicsProcessor proc;
    proc.prepare(48000.0);

    MasterDynamicsParams params{};
    params.enabled = true;
    params.mode = MasterDynamicsMode::Compressor;
    params.thresholdDb = -20.0f;
    params.ratio = 4.0f;
    params.attackMs = 1.0f;
    params.releaseMs = 40.0f;
    params.makeupDb = 0.0f;
    params.mix = 1.0f;

    float left = 0.9f;
    float right = 0.9f;
    for (int i = 0; i < 2400; ++i)
    {
        left = 0.9f;
        right = 0.9f;
        proc.processSample(left, right, 0.0f, 0.0f, false, params);
    }

    REQUIRE(left < 0.9f);
    REQUIRE(proc.getGainReductionDb() < -1.0f);
}
