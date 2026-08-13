#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <vector>

#include "pw8/envelope/DahdsrEnvelope.hpp"

using namespace pw8::envelope;

TEST_CASE("DahdsrEnvelope retargetParams adjusts attack mid-ramp", "[envelope][live]")
{
    DahdsrEnvelope env;
    env.prepare(48000.0);

    DahdsrParams slow;
    slow.attackSeconds = 0.5f;
    slow.decaySeconds = 0.2f;
    slow.sustainLevel = 1.0f;
    env.noteOn(slow);

    float levelMid = 0.0f;
    for (int i = 0; i < 6000; ++i)
        levelMid = env.renderSample();

    REQUIRE(levelMid > 0.2f);
    REQUIRE(levelMid < 0.75f);

    DahdsrParams fast = slow;
    fast.attackSeconds = 0.01f;
    env.retargetParams(fast);

    float levelAfter = levelMid;
    for (int i = 0; i < 480; ++i)
        levelAfter = env.renderSample();

    REQUIRE(levelAfter > levelMid);
}
