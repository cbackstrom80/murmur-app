#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Renderer.hpp"

using namespace pw8;

namespace
{
    midi::MidiSequence singleNoteSequence(double onTime, double offTime, int note = 60, int velocity = 100)
    {
        midi::MidiSequence seq;
        seq.events.push_back(midi::MidiEvent{onTime, midi::EventType::NoteOn, 0, note, velocity, 0, 0});
        seq.events.push_back(midi::MidiEvent{offTime, midi::EventType::NoteOff, 0, note, 0, 0, 0});
        return seq;
    }
} // namespace

TEST_CASE("Renderer produces finite, non-silent audio for INIT SINE", "[render][regression]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].engine = algorithm::EngineType::Classic;
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.ampEnvelope.attackSeconds = 0.005f;
    p.layerA.ampEnvelope.decaySeconds = 0.05f;
    p.layerA.ampEnvelope.sustainLevel = 0.8f;
    p.layerA.ampEnvelope.releaseSeconds = 0.1f;

    auto midiSeq = singleNoteSequence(0.0, 1.0);

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 1.5;

    const auto result = render::render(p, midiSeq, options);

    REQUIRE(result.ok);
    REQUIRE_FALSE(result.interleavedStereo.empty());
    REQUIRE_FALSE(result.metrics.containsNaNOrInf);
    REQUIRE(result.metrics.peak > 0.01f);
    REQUIRE(result.metrics.peak <= 16.0f);
    REQUIRE(result.metrics.durationSeconds == Catch::Approx(1.5).margin(0.01));
}

TEST_CASE("Renderer stays silent with no MIDI input", "[render][regression]")
{
    patch::Patch p = patch::Patch::makeInit();
    midi::MidiSequence empty;

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 0.5;

    const auto result = render::render(p, empty, options);
    REQUIRE(result.ok);
    REQUIRE(result.metrics.peak == 0.0f);
}

TEST_CASE("Renderer handles polyphonic overlapping notes without NaN", "[render][regression][polyphony]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    p.voiceSettings.polyphony = 8;

    midi::MidiSequence seq;
    for (int i = 0; i < 8; ++i)
    {
        const double t = static_cast<double>(i) * 0.05;
        seq.events.push_back(midi::MidiEvent{t, midi::EventType::NoteOn, 0, 48 + i, 100, 0, 0});
        seq.events.push_back(midi::MidiEvent{t + 0.4, midi::EventType::NoteOff, 0, 48 + i, 0, 0, 0});
    }

    render::RenderOptions options;
    options.sampleRate = 44100.0;
    options.durationSecondsOverride = 1.0;

    const auto result = render::render(p, seq, options);
    REQUIRE(result.ok);
    REQUIRE_FALSE(result.metrics.containsNaNOrInf);
}

TEST_CASE("Renderer rejects an out-of-range sample rate", "[render][robustness]")
{
    patch::Patch p = patch::Patch::makeInit();
    midi::MidiSequence seq;

    render::RenderOptions options;
    options.sampleRate = 1.0; // absurd -- must be rejected, not crash.

    const auto result = render::render(p, seq, options);
    REQUIRE_FALSE(result.ok);
}

TEST_CASE("Renderer output is finite even with an FM-style self-feedback algorithm", "[render][regression][feedback]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.algorithm = algorithm::AlgorithmGraphDefinition::makeDefaultParallel8();
    p.layerA.algorithm.edges.push_back(
        algorithm::AlgorithmEdge{core::NodeId(0), core::NodeId(0), algorithm::EdgeType::Feedback, 1.5f});

    auto midiSeq = singleNoteSequence(0.0, 0.5);

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 1.0;

    const auto result = render::render(p, midiSeq, options);
    REQUIRE(result.ok);
    REQUIRE_FALSE(result.metrics.containsNaNOrInf);
    REQUIRE(result.metrics.peak <= 16.0f);
}
