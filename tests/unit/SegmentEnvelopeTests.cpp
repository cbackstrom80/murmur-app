#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "pw8/envelope/SegmentEnvelope.hpp"

using Catch::Matchers::WithinAbs;
using namespace pw8::envelope;

namespace
{
    SegmentEnvelopeChain makeSixSegmentChain()
    {
        SegmentEnvelopeChain chain;
        chain.segments[0] = {SegmentType::Ramp, 10.0f, 0.25f, "linear"};
        chain.segments[1] = {SegmentType::Ramp, 10.0f, 0.5f, "linear"};
        chain.segments[2] = {SegmentType::Hold, 10.0f, 0.5f, ""};
        chain.segments[3] = {SegmentType::Ramp, 10.0f, 0.75f, "smooth"};
        chain.segments[4] = {SegmentType::Step, 10.0f, 1.0f, ""};
        chain.segments[5] = {SegmentType::Ramp, 10.0f, 0.0f, "linear"};
        chain.segmentCount = 6;
        chain.loopStart = 0;
        chain.loopEnd = chain.segmentCount; ///< disabled loop (loopEnd must be < count to activate)
        return chain;
    }

    void renderForMs(SegmentEnvelope& env, double sampleRate, float ms)
    {
        const auto samples = static_cast<std::int64_t>(sampleRate * static_cast<double>(ms) * 0.001);
        for (std::int64_t i = 0; i < samples; ++i)
            env.renderSample();
    }
} // namespace

TEST_CASE("SegmentEnvelope six-segment chain reaches target levels", "[envelope][segment]")
{
    SegmentEnvelope env;
    env.prepare(48000.0);
    const auto chain = makeSixSegmentChain();
    env.noteOn(chain, 0.3f);

    renderForMs(env, 48000.0, 5.0f);
    REQUIRE_THAT(env.getCurrentLevel(), WithinAbs(0.125f, 0.05f));

    renderForMs(env, 48000.0, 6.0f);
    REQUIRE_THAT(env.getCurrentLevel(), WithinAbs(0.275f, 0.09f));

    renderForMs(env, 48000.0, 10.0f);
    REQUIRE_THAT(env.getCurrentLevel(), WithinAbs(0.5f, 0.05f));

    renderForMs(env, 48000.0, 10.0f);
    REQUIRE_THAT(env.getCurrentLevel(), WithinAbs(0.5f, 0.05f));

    renderForMs(env, 48000.0, 10.0f);
    REQUIRE(env.getCurrentLevel() > 0.55f);

    renderForMs(env, 48000.0, 10.0f);
    REQUIRE(env.getSegmentIndex() >= 4);
    REQUIRE(env.getCurrentLevel() > 0.9f);
}

TEST_CASE("SegmentEnvelope noteOff enters release", "[envelope][segment]")
{
    SegmentEnvelope env;
    env.prepare(48000.0);
    SegmentEnvelopeChain chain;
    chain.segments[0] = {SegmentType::Ramp, 50.0f, 1.0f, "linear"};
    chain.segmentCount = 1;
    env.noteOn(chain, 0.05f);

    renderForMs(env, 48000.0, 40.0f);
    REQUIRE(env.getCurrentLevel() > 0.5f);

    env.noteOff();
    renderForMs(env, 48000.0, 60.0f);
    REQUIRE(env.getCurrentLevel() == Catch::Approx(0.0f).margin(0.01f));
    REQUIRE_FALSE(env.isActive());
}

TEST_CASE("SegmentEnvelope loop markers repeat sub-chain while gated", "[envelope][segment]")
{
    SegmentEnvelope env;
    env.prepare(48000.0);
    auto chain = makeSixSegmentChain();
    chain.loopStart = 0;
    chain.loopEnd = 2;
    env.noteOn(chain, 0.3f);

    renderForMs(env, 48000.0, 35.0f);
    REQUIRE(env.getSegmentIndex() <= 2);
    REQUIRE(env.isActive());
}

TEST_CASE("SegmentEnvelope empty chain stays idle", "[envelope][segment]")
{
    SegmentEnvelope env;
    env.prepare(48000.0);
    SegmentEnvelopeChain chain;
    env.noteOn(chain, 0.3f);
    REQUIRE(env.renderSample() == Catch::Approx(0.0f));
    REQUIRE_FALSE(env.isActive());
}
