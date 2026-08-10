#include <algorithm>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

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

    midi::MidiSequence heldChordSequence(double onTime, double offTime, std::initializer_list<int> notes, int velocity = 100)
    {
        midi::MidiSequence seq;
        for (int note : notes)
            seq.events.push_back(midi::MidiEvent{onTime, midi::EventType::NoteOn, 0, note, velocity, 0, 0});
        for (int note : notes)
            seq.events.push_back(midi::MidiEvent{offTime, midi::EventType::NoteOff, 0, note, 0, 0, 0});
        return seq;
    }

    /// Counts how many times a windowed RMS envelope rises from below `threshold` to
    /// at/above it -- a proxy for "how many discrete note onsets happened", used to
    /// distinguish "one sustained tone" from "many short retriggered notes" without
    /// needing to inspect engine internals from the test.
    int countAmplitudeOnsets(const std::vector<float>& interleavedStereo, int windowSize, float threshold)
    {
        const std::size_t numFrames = interleavedStereo.size() / 2;
        int onsets = 0;
        bool above = false;
        for (std::size_t start = 0; start < numFrames; start += static_cast<std::size_t>(windowSize))
        {
            const std::size_t end = std::min(numFrames, start + static_cast<std::size_t>(windowSize));
            double sumSq = 0.0;
            for (std::size_t i = start; i < end; ++i)
            {
                const float l = interleavedStereo[i * 2];
                const float r = interleavedStereo[i * 2 + 1];
                sumSq += static_cast<double>(l) * l + static_cast<double>(r) * r;
            }
            const float rms = static_cast<float>(std::sqrt(sumSq / (2.0 * static_cast<double>(end - start))));
            const bool nowAbove = rms >= threshold;
            if (nowAbove && !above)
                ++onsets;
            above = nowAbove;
        }
        return onsets;
    }
} // namespace

TEST_CASE("Renderer produces finite, non-silent audio for INIT SINE", "[render][regression]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].engine = algorithm::EngineType::Classic;
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.005f;
    p.layerA.envelopes[0].decaySeconds = 0.05f;
    p.layerA.envelopes[0].sustainLevel = 0.8f;
    p.layerA.envelopes[0].releaseSeconds = 0.1f;

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
    p.layerA.envelopes[0].attackSeconds = 0.005f;
    p.layerA.envelopes[0].sustainLevel = 1.0f;

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

    p.layerA.lfos[0].waveform = lfo::LfoWaveform::Sine;
    p.layerA.lfos[0].mode = lfo::LfoMode::Free;
    p.layerA.lfos[0].rateHz = 6.0f;

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
    p.layerA.envelopes[0].sustainLevel = 1.0f;
    p.layerA.lfos[0].mode = lfo::LfoMode::TempoSync;
    p.layerA.lfos[0].syncDivisionIndex = 4; // 1/4 note -- rate scales directly with BPM.
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

TEST_CASE("Renderer: enabling the arpeggiator turns one held chord into many discrete note onsets",
          "[render][regression][arpeggiator]")
{
    // Short, percussive envelope so each arp-triggered note is a distinct audible
    // "blip" rather than blending into a sustained tone.
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.002f;
    p.layerA.envelopes[0].decaySeconds = 0.04f;
    p.layerA.envelopes[0].sustainLevel = 0.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.02f;
    p.voiceSettings.polyphony = 8;

    auto midiSeq = heldChordSequence(0.0, 2.0, {60, 64, 67});

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 2.2;

    const auto withoutArp = render::render(p, midiSeq, options);
    REQUIRE(withoutArp.ok);
    REQUIRE_FALSE(withoutArp.metrics.containsNaNOrInf);

    p.arpeggiator.enabled = true;
    p.arpeggiator.mode = sequencer::ArpMode::Up;
    p.arpeggiator.rateMode = sequencer::ArpRateMode::Free;
    p.arpeggiator.rateHz = 8.0f; // 8 notes/sec over a 2s hold -> ~16 onsets expected.
    p.arpeggiator.octaveRange = 1;
    p.arpeggiator.numSteps = 1;
    p.arpeggiator.steps[0] = sequencer::ArpStep{};

    const auto withArp = render::render(p, midiSeq, options);
    REQUIRE(withArp.ok);
    REQUIRE_FALSE(withArp.metrics.containsNaNOrInf);
    REQUIRE(withArp.metrics.peak > 0.0f);

    constexpr int kWindow = 240; // 5ms windows at 48kHz.
    constexpr float kThreshold = 0.02f;
    const int onsetsWithoutArp = countAmplitudeOnsets(withoutArp.interleavedStereo, kWindow, kThreshold);
    const int onsetsWithArp = countAmplitudeOnsets(withArp.interleavedStereo, kWindow, kThreshold);

    // Without the arp: one chord attack (plus maybe the shared-envelope release
    // tail crossing back up briefly) -- a handful of onsets at most.
    REQUIRE(onsetsWithoutArp <= 3);
    // With the arp at 8 Hz over ~2s: many distinct retriggered notes.
    REQUIRE(onsetsWithArp >= 10);
}

TEST_CASE("Renderer: a master TapeDelay slot turns one short hit into multiple decaying echoes",
          "[render][regression][effects]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.envelopes[0].attackSeconds = 0.002f;
    p.layerA.envelopes[0].decaySeconds = 0.03f;
    p.layerA.envelopes[0].sustainLevel = 0.0f;
    p.layerA.envelopes[0].releaseSeconds = 0.02f;

    auto midiSeq = singleNoteSequence(0.0, 0.05, 60);

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 1.5;

    const auto withoutFx = render::render(p, midiSeq, options);
    REQUIRE(withoutFx.ok);
    REQUIRE_FALSE(withoutFx.metrics.containsNaNOrInf);

    p.masterEffects[0].type = effects::EffectType::TapeDelay;
    p.masterEffects[0].mix = 0.9f;
    p.masterEffects[0].tapeDelayMs = 150.0f;
    p.masterEffects[0].tapeFeedback = 0.55f;
    p.masterEffects[0].tapeDriftDepthMs = 0.0f;

    const auto withFx = render::render(p, midiSeq, options);
    REQUIRE(withFx.ok);
    REQUIRE_FALSE(withFx.metrics.containsNaNOrInf);
    REQUIRE(withFx.metrics.peak > 0.0f);

    constexpr int kWindow = 240; // 5ms windows at 48kHz.
    constexpr float kThreshold = 0.01f;
    const int onsetsWithoutFx = countAmplitudeOnsets(withoutFx.interleavedStereo, kWindow, kThreshold);
    const int onsetsWithFx = countAmplitudeOnsets(withFx.interleavedStereo, kWindow, kThreshold);

    // One short hit, no delay: a single onset.
    REQUIRE(onsetsWithoutFx <= 1);
    // 150ms repeats over 1.5s at 55% feedback: several audible echoes before decaying
    // under the onset threshold.
    REQUIRE(onsetsWithFx >= 4);
}

TEST_CASE("Renderer: a layer insert Saturation slot audibly compresses a loud signal (RMS moves toward the ceiling)",
          "[render][regression][effects]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    p.layerA.operators[0].level = 1.0f;
    p.layerA.envelopes[0].attackSeconds = 0.005f;
    p.layerA.envelopes[0].decaySeconds = 0.05f;
    p.layerA.envelopes[0].sustainLevel = 0.9f;
    p.layerA.envelopes[0].releaseSeconds = 0.05f;
    p.layerA.gain = 1.8f; // deliberately loud, so saturation has something to bite into.

    auto midiSeq = singleNoteSequence(0.0, 0.6, 48, 127);

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 0.8;

    const auto withoutFx = render::render(p, midiSeq, options);
    REQUIRE(withoutFx.ok);
    REQUIRE_FALSE(withoutFx.metrics.containsNaNOrInf);

    p.layerA.insertEffects[0].type = effects::EffectType::Saturation;
    p.layerA.insertEffects[0].mix = 1.0f;
    p.layerA.insertEffects[0].saturationDriveDb = 18.0f;

    const auto withFx = render::render(p, midiSeq, options);
    REQUIRE(withFx.ok);
    REQUIRE_FALSE(withFx.metrics.containsNaNOrInf);

    // Saturation compresses the loud signal toward unity -- the effect's own peak
    // must be lower than the dry signal's, even though the dry signal was already
    // clamped/summed the same way upstream (this isolates the FX slot's own effect,
    // not a difference in voice rendering).
    REQUIRE(withFx.metrics.peak < withoutFx.metrics.peak);
    REQUIRE(withFx.metrics.peak <= 1.05f);
}

TEST_CASE("Renderer: a LAYER-scoped LFO route is one continuously-running shared clock, not reset per note-on",
          "[render][regression][modulation][scope]")
{
    // The distinguishing claim of LAYER/GLOBAL scope (docs/MODULATION.md, ModScope's
    // doc comment in ModMatrixTypes.hpp): the shared LFO tick keeps running from
    // render::Engine::process()'s very first sample regardless of when any note
    // starts, unlike a VOICE-scoped LFO, which restarts from phase 0 at every
    // note-on. Proven by triggering the SAME patch's SAME note at two different
    // start times and comparing each render's pan a fixed 100ms after each note's
    // own start: if the shared clock is real, a note starting a quarter-LFO-cycle
    // later hears a measurably different absolute LFO phase at that point. A
    // per-voice-reset LFO would show the same phase (and pan) relative to each
    // note's own onset regardless of when it started.
    //
    // Uses a slow (0.2Hz, 5s period) LFO and a full-second note-start offset so the
    // renderer's block-quantized MIDI dispatch (events fire at the start of whichever
    // `options.blockSize`-sized block contains their timestamp, +/- ~10ms at the
    // default 512-sample block size, not sample-accurately) is a negligible fraction
    // of the timing this test depends on.
    auto buildPatch = [](modulation::ModScope scope) {
        patch::Patch p = patch::Patch::makeInit();
        p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
        p.layerA.envelopes[0].attackSeconds = 0.001f;
        p.layerA.envelopes[0].decaySeconds = 0.05f;
        p.layerA.envelopes[0].sustainLevel = 1.0f;
        p.layerA.envelopes[0].releaseSeconds = 0.05f;

        p.layerA.lfos[0].waveform = lfo::LfoWaveform::Sine;
        p.layerA.lfos[0].mode = lfo::LfoMode::Free;
        p.layerA.lfos[0].rateHz = 0.2f; // 5s period -- a 1.25s note-start offset is exactly a quarter cycle.

        modulation::ModRoute route;
        route.source = modulation::ModSource::Lfo1;
        route.destination = modulation::ModDestination::Pan;
        route.amount = 0.9f;
        route.scope = scope;
        p.layerA.modRoutes.push_back(route);
        return p;
    };

    // Windowed-RMS(R) - windowed-RMS(L) around a point in time -- positive means
    // panned right, negative left, ~0 centered. Deliberately NOT a single
    // instantaneous sample difference: equal-power panning scales L and R by fixed
    // multipliers (cos/sin(panRad)) of the SAME instantaneous carrier sample, and
    // that carrier itself is a ~261Hz sine oscillating through zero every ~2ms --
    // an instantaneous (R-L) reading's sign flips at audio rate regardless of pan.
    // RMS over a window many carrier cycles wide (here 40ms) is stable and reflects
    // only the (slowly-moving) pan, not the carrier's own instantaneous phase.
    auto panAtTime = [](const std::vector<float>& interleaved, double sampleRate, double timeSeconds) -> float {
        const auto windowSamples = static_cast<std::size_t>(0.04 * sampleRate);
        const auto centerIdx = static_cast<std::size_t>(timeSeconds * sampleRate);
        const auto numFrames = interleaved.size() / 2;
        const auto start = centerIdx > windowSamples / 2 ? centerIdx - windowSamples / 2 : 0;
        const auto end = std::min(numFrames, start + windowSamples);
        if (start >= end)
            return 0.0f;

        double sumSqL = 0.0, sumSqR = 0.0;
        for (std::size_t i = start; i < end; ++i)
        {
            sumSqL += static_cast<double>(interleaved[i * 2]) * interleaved[i * 2];
            sumSqR += static_cast<double>(interleaved[i * 2 + 1]) * interleaved[i * 2 + 1];
        }
        const auto n = static_cast<double>(end - start);
        return static_cast<float>(std::sqrt(sumSqR / n) - std::sqrt(sumSqL / n));
    };

    constexpr double kNoteStartOffset = 1.25; // quarter cycle of the 0.2Hz LFO.
    constexpr double kSettleTime = 0.1;       // sampled 100ms after each note's own start.

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = kNoteStartOffset + 2.0;

    SECTION("Layer scope: pan (100ms after note-on) differs between an immediate note and one starting a "
            "quarter-LFO-cycle later")
    {
        const patch::Patch p = buildPatch(modulation::ModScope::Layer);

        const auto immediate = render::render(p, singleNoteSequence(0.0, 1.5), options);
        const auto delayed = render::render(p, singleNoteSequence(kNoteStartOffset, kNoteStartOffset + 1.5), options);
        REQUIRE(immediate.ok);
        REQUIRE(delayed.ok);

        // Immediate: absolute LFO phase at t=0.1s is 0.1*0.2=0.02 turns -> sin(~7deg) small -> near-centered.
        const float panImmediate = panAtTime(immediate.interleavedStereo, options.sampleRate, kSettleTime);
        // Delayed: absolute LFO phase at t=1.35s is 1.35*0.2=0.27 turns -> sin(~97deg) ~ +1 -> hard right.
        const float panDelayed = panAtTime(delayed.interleavedStereo, options.sampleRate, kNoteStartOffset + kSettleTime);

        REQUIRE(std::abs(panImmediate) < 0.25f); // near-centered.
        REQUIRE(panDelayed > 0.5f);              // clearly panned right -- a different absolute phase was heard.
    }

    SECTION("Voice scope: pan (100ms after note-on) is the same regardless of note start time (each voice resets "
            "its own LFO)")
    {
        const patch::Patch p = buildPatch(modulation::ModScope::Voice);

        const auto immediate = render::render(p, singleNoteSequence(0.0, 1.5), options);
        const auto delayed = render::render(p, singleNoteSequence(kNoteStartOffset, kNoteStartOffset + 1.5), options);
        REQUIRE(immediate.ok);
        REQUIRE(delayed.ok);

        const float panImmediate = panAtTime(immediate.interleavedStereo, options.sampleRate, kSettleTime);
        const float panDelayed = panAtTime(delayed.interleavedStereo, options.sampleRate, kNoteStartOffset + kSettleTime);

        // Both start their own LFO fresh at noteOn(), so both should land at
        // essentially the same phase/pan 100ms into their own note -- confirming the
        // scope distinction above is real (VOICE scope does NOT show the
        // shared-clock behavior).
        REQUIRE(std::abs(panImmediate - panDelayed) < 0.25f);
    }
}
