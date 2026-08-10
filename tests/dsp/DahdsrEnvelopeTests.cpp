#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "pw8/envelope/DahdsrEnvelope.hpp"

using namespace pw8::envelope;

TEST_CASE("DahdsrEnvelope reaches sustain and holds it", "[envelope]")
{
    DahdsrEnvelope env;
    env.prepare(48000.0);

    DahdsrParams params;
    params.attackSeconds = 0.01f;
    params.decaySeconds = 0.01f;
    params.sustainLevel = 0.5f;
    params.releaseSeconds = 0.1f;

    env.noteOn(params);

    float last = 0.0f;
    for (int i = 0; i < 48000; ++i) // 1 second: well past attack+decay.
        last = env.renderSample();

    REQUIRE(last == Catch::Approx(0.5f).margin(0.01f));
    REQUIRE(env.getStage() == Stage::Sustain);
}

TEST_CASE("DahdsrEnvelope release returns fully to zero and goes idle", "[envelope]")
{
    DahdsrEnvelope env;
    env.prepare(48000.0);

    DahdsrParams params;
    params.attackSeconds = 0.001f;
    params.decaySeconds = 0.001f;
    params.sustainLevel = 1.0f;
    params.releaseSeconds = 0.05f;

    env.noteOn(params);
    for (int i = 0; i < 1000; ++i)
        (void)env.renderSample();

    env.noteOff();
    float last = 0.0f;
    for (int i = 0; i < 48000; ++i) // 1 second, release is 50ms -- plenty of margin.
        last = env.renderSample();

    REQUIRE(last == Catch::Approx(0.0f).margin(1.0e-4f));
    REQUIRE(env.getStage() == Stage::Idle);
    REQUIRE_FALSE(env.isActive());
}

TEST_CASE("DahdsrEnvelope attack takes approximately the configured duration", "[envelope][timing]")
{
    DahdsrEnvelope env;
    const double sampleRate = 48000.0;
    env.prepare(sampleRate);

    DahdsrParams params;
    params.attackSeconds = 0.1f;
    params.decaySeconds = 5.0f; // long decay so we don't leave Attack early.
    params.sustainLevel = 0.0f;
    params.curveShape = 0.0f; // linear, so crossing 1.0 happens right at the boundary.

    env.noteOn(params);

    int samplesUntilPeak = 0;
    for (; samplesUntilPeak < static_cast<int>(sampleRate); ++samplesUntilPeak)
    {
        if (env.renderSample() >= 0.999f)
            break;
    }

    const double measuredSeconds = static_cast<double>(samplesUntilPeak) / sampleRate;
    REQUIRE(measuredSeconds == Catch::Approx(0.1).margin(0.005));
}

TEST_CASE("DahdsrEnvelope reset silences immediately", "[envelope]")
{
    DahdsrEnvelope env;
    env.prepare(48000.0);
    DahdsrParams params;
    params.attackSeconds = 0.001f;
    params.sustainLevel = 1.0f;
    env.noteOn(params);
    for (int i = 0; i < 100; ++i)
        (void)env.renderSample();

    env.reset();
    REQUIRE(env.getCurrentLevel() == 0.0f);
    REQUIRE(env.getStage() == Stage::Idle);
}
