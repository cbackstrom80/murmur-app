#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/lfo/Lfo.hpp"

using namespace pw8::lfo;

namespace
{
    constexpr double kSampleRate = 48000.0;
}

TEST_CASE("Lfo sine wave stays in [-1, 1] and runs at the configured rate", "[lfo]")
{
    Lfo l;
    l.prepare(kSampleRate);
    LfoParams params;
    params.waveform = LfoWaveform::Sine;
    params.mode = LfoMode::Free;
    params.rateHz = 4.0f;
    l.noteOn(params, 1);

    // Render a touch over 1 second: at exactly 4 Hz, the 4th rising zero-crossing
    // lands exactly on sample 48000, so a hard 48000-sample window can miss it by a
    // rounding hair. A small margin makes the count robust without weakening what's
    // being checked (the margin is far shorter than a further cycle).
    int crossings = 0;
    float prev = l.renderSample(params, 120.0f);
    for (int i = 1; i < 48100; ++i)
    {
        const float v = l.renderSample(params, 120.0f);
        REQUIRE(v >= -1.0001f);
        REQUIRE(v <= 1.0001f);
        if (prev < 0.0f && v >= 0.0f)
            ++crossings;
        prev = v;
    }
    REQUIRE(crossings == 4); // 4 Hz over ~1 second == 4 rising zero-crossings.
}

TEST_CASE("Lfo square wave only ever outputs +-1", "[lfo]")
{
    Lfo l;
    l.prepare(kSampleRate);
    LfoParams params;
    params.waveform = LfoWaveform::Square;
    params.rateHz = 5.0f;
    l.noteOn(params, 1);

    for (int i = 0; i < 10000; ++i)
    {
        const float v = l.renderSample(params, 120.0f);
        REQUIRE((v == 1.0f || v == -1.0f));
    }
}

TEST_CASE("Lfo Retrigger mode resets phase deterministically on noteOn", "[lfo]")
{
    Lfo a, b;
    a.prepare(kSampleRate);
    b.prepare(kSampleRate);
    LfoParams params;
    params.waveform = LfoWaveform::Triangle;
    params.mode = LfoMode::Retrigger;
    params.rateHz = 3.0f;

    a.noteOn(params, 1);
    for (int i = 0; i < 500; ++i)
        (void)a.renderSample(params, 120.0f);

    a.noteOn(params, 1); // retrigger -- should behave identically to a fresh instance.
    b.noteOn(params, 1);

    for (int i = 0; i < 1000; ++i)
        REQUIRE(a.renderSample(params, 120.0f) == b.renderSample(params, 120.0f));
}

TEST_CASE("Lfo OneShot mode stops advancing after one cycle", "[lfo]")
{
    Lfo l;
    l.prepare(kSampleRate);
    LfoParams params;
    params.waveform = LfoWaveform::Saw;
    params.mode = LfoMode::OneShot;
    params.rateHz = 100.0f; // fast, so one cycle (480 samples at 48kHz) completes well within 600.
    l.noteOn(params, 1);

    for (int i = 0; i < 600; ++i)
        (void)l.renderSample(params, 120.0f);

    const float heldValue = l.renderSample(params, 120.0f);
    for (int i = 0; i < 10000; ++i)
        REQUIRE(l.renderSample(params, 120.0f) == heldValue);
}

TEST_CASE("Lfo TempoSync computes rate from BPM and division correctly", "[lfo][tempo]")
{
    Lfo l;
    l.prepare(kSampleRate);
    LfoParams params;
    params.waveform = LfoWaveform::Sine;
    params.mode = LfoMode::TempoSync;
    params.syncDivisionIndex = 4; // 1 quarter-note-per-cycle -> at 120 BPM, 2 Hz.
    l.noteOn(params, 1);

    // Same margin rationale as the Free-mode rate test above.
    int crossings = 0;
    float prev = l.renderSample(params, 120.0f);
    for (int i = 1; i < 48100; ++i)
    {
        const float v = l.renderSample(params, 120.0f);
        if (prev < 0.0f && v >= 0.0f)
            ++crossings;
        prev = v;
    }
    REQUIRE(crossings == 2);
}

TEST_CASE("Lfo SampleHold is deterministic for a given seed and changes only at cycle boundaries", "[lfo][determinism]")
{
    Lfo a, b;
    a.prepare(kSampleRate);
    b.prepare(kSampleRate);
    LfoParams params;
    params.waveform = LfoWaveform::SampleHold;
    params.rateHz = 2.0f;
    a.noteOn(params, 42);
    b.noteOn(params, 42);

    float lastValue = a.renderSample(params, 120.0f);
    REQUIRE(lastValue == b.renderSample(params, 120.0f));

    int changes = 0;
    for (int i = 1; i < 48000; ++i)
    {
        const float av = a.renderSample(params, 120.0f);
        REQUIRE(av == b.renderSample(params, 120.0f)); // same seed -> identical sequence.
        if (av != lastValue)
        {
            ++changes;
            lastValue = av;
        }
    }
    // At 2 Hz over 1 second, the held value should change approximately twice (once per wrap).
    REQUIRE(changes >= 1);
    REQUIRE(changes <= 3);
}
