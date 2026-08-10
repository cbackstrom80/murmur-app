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

TEST_CASE("Renderer: enabling Filter 1 audibly changes a bright saw's spectrum (RMS drops)", "[render][regression][filter]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    p.layerA.ampEnvelope.attackSeconds = 0.005f;
    p.layerA.ampEnvelope.sustainLevel = 1.0f;

    auto midiSeq = singleNoteSequence(0.0, 1.0, 48); // low note -> saw has strong high harmonics to remove.

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 1.2;

    const auto unfiltered = render::render(p, midiSeq, options);
    REQUIRE(unfiltered.ok);
    REQUIRE_FALSE(unfiltered.metrics.containsNaNOrInf);

    p.layerA.filter1.enabled = true;
    p.layerA.filter1.mode = filter::FilterMode::Lowpass;
    p.layerA.filter1.cutoffHz = 150.0f; // well below the saw's upper harmonics.
    p.layerA.filter1.resonance = 0.1f;

    const auto filtered = render::render(p, midiSeq, options);
    REQUIRE(filtered.ok);
    REQUIRE_FALSE(filtered.metrics.containsNaNOrInf);

    REQUIRE(filtered.metrics.rms < unfiltered.metrics.rms * 0.9f);
}

TEST_CASE("Renderer: mod matrix (LFO -> filter cutoff, velocity -> operator level) renders finite audio", "[render][regression][modulation]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;

    p.layerA.filter1.enabled = true;
    p.layerA.filter1.cutoffHz = 1500.0f;
    p.layerA.filter1.resonance = 0.4f;

    p.layerA.lfo1.waveform = lfo::LfoWaveform::Sine;
    p.layerA.lfo1.mode = lfo::LfoMode::Free;
    p.layerA.lfo1.rateHz = 6.0f;

    p.layerA.modRoutes.push_back(
        modulation::ModRoute{modulation::ModSource::Lfo1, modulation::ModDestination::FilterCutoff, 0, 24.0f, modulation::ModScope::Voice});
    p.layerA.modRoutes.push_back(modulation::ModRoute{modulation::ModSource::Velocity, modulation::ModDestination::OperatorLevel,
                                                        0, 0.5f, modulation::ModScope::Voice});
    p.layerA.modRoutes.push_back(
        modulation::ModRoute{modulation::ModSource::ChannelPressure, modulation::ModDestination::Pan, 0, 0.3f, modulation::ModScope::Voice});

    auto midiSeq = singleNoteSequence(0.0, 1.0, 55, 90);

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 1.2;

    const auto result = render::render(p, midiSeq, options);
    REQUIRE(result.ok);
    REQUIRE_FALSE(result.metrics.containsNaNOrInf);
    REQUIRE(result.metrics.peak > 0.0f);
    REQUIRE(result.metrics.peak <= 16.0f);
}

TEST_CASE("Renderer: tempo-synced LFO's rate actually tracks --bpm end to end", "[render][regression][lfo][tempo]")
{
    // A tempo-synced LFO modulating operator level (tremolo) at the SAME sync
    // division but very different BPMs must produce audibly different results over
    // a fixed 1-second window -- if RenderOptions::bpm never reached the LFO (e.g.
    // Engine::setTempo() wasn't wired, or Renderer forgot to call it), both renders
    // would be identical (both silently falling back to the 120 BPM default).
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    p.layerA.ampEnvelope.sustainLevel = 1.0f;
    p.layerA.lfo1.mode = lfo::LfoMode::TempoSync;
    p.layerA.lfo1.syncDivisionIndex = 4; // 1/4 note -- rate scales directly with BPM.
    p.layerA.modRoutes.push_back(modulation::ModRoute{modulation::ModSource::Lfo1, modulation::ModDestination::OperatorLevel,
                                                        0, 0.9f, modulation::ModScope::Voice});

    auto midiSeq = singleNoteSequence(0.0, 1.0);
    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 1.0;

    options.bpm = 40.0; // slow: well under one full tremolo cycle in 1 second.
    const auto slow = render::render(p, midiSeq, options);

    options.bpm = 600.0; // fast: many tremolo cycles in 1 second.
    const auto fast = render::render(p, midiSeq, options);

    REQUIRE(slow.ok);
    REQUIRE(fast.ok);
    REQUIRE_FALSE(slow.metrics.containsNaNOrInf);
    REQUIRE_FALSE(fast.metrics.containsNaNOrInf);
    // Different BPM -> different tremolo rate -> the two renders must differ
    // (a bug that ignores bpm would make these numerically identical).
    REQUIRE(std::abs(slow.metrics.rms - fast.metrics.rms) > 0.01f);
}
