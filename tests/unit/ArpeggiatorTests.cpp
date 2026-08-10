#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "pw8/sequencer/Arpeggiator.hpp"

using namespace pw8;
using namespace pw8::sequencer;

namespace
{
    constexpr double kSampleRate = 48000.0;

    struct Captured
    {
        ArpEventType type;
        int note;
    };

    std::vector<Captured> runTicks(Arpeggiator& arp, int numSamples, float bpm = 120.0f)
    {
        std::vector<Captured> out;
        for (int i = 0; i < numSamples; ++i)
        {
            core::FixedVector<ArpEvent, 32> events;
            arp.tick(kSampleRate, bpm, events);
            for (const auto& e : events)
                out.push_back({e.type, e.note});
        }
        return out;
    }

    std::vector<int> noteOnSequence(const std::vector<Captured>& events)
    {
        std::vector<int> out;
        for (const auto& e : events)
            if (e.type == ArpEventType::NoteOn)
                out.push_back(e.note);
        return out;
    }

    ArpeggiatorParams basicParams(ArpMode mode, int octaveRange = 1, float rateHz = 100.0f)
    {
        ArpeggiatorParams p;
        p.enabled = true;
        p.mode = mode;
        p.rateMode = ArpRateMode::Free;
        p.rateHz = rateHz;
        p.octaveRange = octaveRange;
        p.numSteps = 1; // one plain, always-firing step -- isolates note-sequence behavior for these tests.
        p.steps[0] = ArpStep{};
        return p;
    }
} // namespace

TEST_CASE("Arpeggiator Up mode ascends through the held chord across octaveRange", "[arpeggiator]")
{
    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(basicParams(ArpMode::Up, 2), 1);

    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);
    arp.noteHeld(67, 0, 1.0f);

    const auto seq = noteOnSequence(runTicks(arp, 480 * 8)); // 8 steps at 100Hz/48kHz.
    REQUIRE(seq.size() >= 6);
    const std::vector<int> expected = {60, 64, 67, 72, 76, 79};
    for (std::size_t i = 0; i < 6; ++i)
        REQUIRE(seq[i] == expected[i]);
    REQUIRE(seq[6] == 60); // wraps back to the start of the pattern.
}

TEST_CASE("Arpeggiator Down mode descends", "[arpeggiator]")
{
    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(basicParams(ArpMode::Down, 1), 1);

    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);
    arp.noteHeld(67, 0, 1.0f);

    const auto seq = noteOnSequence(runTicks(arp, 480 * 4));
    REQUIRE(seq.size() >= 3);
    REQUIRE(seq[0] == 67);
    REQUIRE(seq[1] == 64);
    REQUIRE(seq[2] == 60);
}

TEST_CASE("Arpeggiator UpDown mode doesn't repeat the peak", "[arpeggiator]")
{
    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(basicParams(ArpMode::UpDown, 1), 1);

    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);
    arp.noteHeld(67, 0, 1.0f);

    const auto seq = noteOnSequence(runTicks(arp, 480 * 5));
    REQUIRE(seq.size() >= 4);
    // up: 60, 64, 67 -- then down without repeating 67 or (eventually) 60: 64, then back to 60...
    REQUIRE(seq[0] == 60);
    REQUIRE(seq[1] == 64);
    REQUIRE(seq[2] == 67);
    REQUIRE(seq[3] == 64);
}

TEST_CASE("Arpeggiator AsPlayed mode preserves press order, not pitch order", "[arpeggiator]")
{
    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(basicParams(ArpMode::AsPlayed, 1), 1);

    arp.noteHeld(67, 0, 1.0f); // pressed first, despite being the highest pitch.
    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);

    const auto seq = noteOnSequence(runTicks(arp, 480 * 3));
    REQUIRE(seq.size() >= 3);
    REQUIRE(seq[0] == 67);
    REQUIRE(seq[1] == 60);
    REQUIRE(seq[2] == 64);
}

TEST_CASE("Arpeggiator Chord mode fires every held note together on each step", "[arpeggiator]")
{
    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(basicParams(ArpMode::Chord, 1), 1);

    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);
    arp.noteHeld(67, 0, 1.0f);

    // Tick until just past the first step boundary, collecting everything that fires.
    std::vector<Captured> all = runTicks(arp, 480 + 1);
    const auto seq = noteOnSequence(all);
    REQUIRE(seq.size() == 3);
    REQUIRE(((seq[0] == 60 || seq[0] == 64 || seq[0] == 67)));
}

TEST_CASE("Arpeggiator latch keeps cycling after all notes release; without latch it stops", "[arpeggiator][latch]")
{
    ArpeggiatorParams params = basicParams(ArpMode::Up, 1);
    params.latch = true;

    Arpeggiator latched;
    latched.prepare(kSampleRate);
    latched.configure(params, 1);
    latched.noteHeld(60, 0, 1.0f);
    latched.noteHeld(64, 0, 1.0f);
    (void)runTicks(latched, 480); // let the first step fire.
    latched.noteReleased(60, 0);
    latched.noteReleased(64, 0); // fully released -- latch should keep the pattern alive.

    const auto latchedSeq = noteOnSequence(runTicks(latched, 480 * 4));
    REQUIRE(latchedSeq.size() >= 2); // still producing notes after release.

    ArpeggiatorParams unlatchedParams = basicParams(ArpMode::Up, 1);
    unlatchedParams.latch = false;
    Arpeggiator unlatched;
    unlatched.prepare(kSampleRate);
    unlatched.configure(unlatchedParams, 1);
    unlatched.noteHeld(60, 0, 1.0f);
    unlatched.noteHeld(64, 0, 1.0f);
    (void)runTicks(unlatched, 480);
    unlatched.noteReleased(60, 0);
    unlatched.noteReleased(64, 0);

    const auto unlatchedSeq = noteOnSequence(runTicks(unlatched, 480 * 4));
    REQUIRE(unlatchedSeq.empty()); // pattern stopped -- no held notes, no latch.
}

TEST_CASE("Arpeggiator probability is deterministic for a given seed", "[arpeggiator][determinism]")
{
    ArpeggiatorParams params = basicParams(ArpMode::Up, 2);
    params.steps[0].probability = 0.5f; // some steps will be skipped -- must be identical across runs.

    Arpeggiator a, b;
    a.prepare(kSampleRate);
    b.prepare(kSampleRate);
    a.configure(params, 42);
    b.configure(params, 42);
    a.noteHeld(60, 0, 1.0f);
    a.noteHeld(64, 0, 1.0f);
    b.noteHeld(60, 0, 1.0f);
    b.noteHeld(64, 0, 1.0f);

    const auto seqA = noteOnSequence(runTicks(a, 480 * 20));
    const auto seqB = noteOnSequence(runTicks(b, 480 * 20));
    REQUIRE(seqA == seqB);
    REQUIRE(seqA.size() < 20); // probability 0.5 should have skipped roughly half of 20 steps.
}

TEST_CASE("Arpeggiator ratchet subdivides a step into multiple triggers", "[arpeggiator][ratchet]")
{
    ArpeggiatorParams params = basicParams(ArpMode::Up, 1, 50.0f); // slower rate -> more room to see sub-triggers.
    params.steps[0].ratchetCount = 3;

    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(params, 1);
    arp.noteHeld(60, 0, 1.0f);

    // The arp fires its first step immediately (tick 0), not after a full step
    // period -- so this window must stop *before* the second step-cycle fires
    // (at absolute sample `stepSamples`), or it'll pick up that cycle's first
    // sub-hit too.
    const int stepSamples = static_cast<int>(kSampleRate / 50.0);
    const auto seq = noteOnSequence(runTicks(arp, stepSamples - 10));
    REQUIRE(seq.size() == 3); // one step, ratchet 3 -- three note-on triggers within that one step.
}

TEST_CASE("Arpeggiator tie suppresses retriggering for that step", "[arpeggiator][tie]")
{
    ArpeggiatorParams params = basicParams(ArpMode::Up, 1);
    params.numSteps = 2;
    params.steps[0] = ArpStep{};
    params.steps[1] = ArpStep{};
    params.steps[1].tie = true;

    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(params, 1);
    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);

    // Step 0 fires immediately (tick 0); step 1 (the tie) fires at absolute sample
    // 480 and must NOT add a note-on. Stop well before step 0's *next* cycle would
    // fire again (at absolute sample 960).
    const auto seq = noteOnSequence(runTicks(arp, 480 + 100));
    REQUIRE(seq.size() == 1);
}

TEST_CASE("Arpeggiator step pattern and note sequence cycle independently (polymetric)", "[arpeggiator][polymetric]")
{
    // 5-step pattern (varying octaveOffset per step) over a 3-note chord (octaveRange=1):
    // the (step, note) pairing should not repeat until step 15 (lcm(5,3)), not step 3 or 5.
    ArpeggiatorParams params = basicParams(ArpMode::Up, 1);
    params.numSteps = 5;
    for (std::size_t i = 0; i < 5; ++i)
    {
        params.steps[i] = ArpStep{};
        params.steps[i].octaveOffset = static_cast<int>(i) % 2; // steps 0,2,4 -> +0; steps 1,3 -> +1 octave.
    }

    Arpeggiator arp;
    arp.prepare(kSampleRate);
    arp.configure(params, 1);
    arp.noteHeld(60, 0, 1.0f);
    arp.noteHeld(64, 0, 1.0f);
    arp.noteHeld(67, 0, 1.0f);

    const auto seq = noteOnSequence(runTicks(arp, 480 * 16));
    REQUIRE(seq.size() >= 16);

    // First 5 notes (steps 0-4): chord notes 0,1,2 with octave pattern [0,+12,0,+12,0]
    // applied to note-sequence positions 0,1,2,0,1 (note-sequence length 3, step count 5).
    REQUIRE(seq[0] == 60);      // step0: note idx0(60), oct+0
    REQUIRE(seq[1] == 76);      // step1: note idx1(64), oct+12
    REQUIRE(seq[2] == 67);      // step2: note idx2(67), oct+0
    REQUIRE(seq[3] == 72);      // step3: note idx0(60), oct+12
    REQUIRE(seq[4] == 64);      // step4: note idx1(64), oct+0
    // Step 5 wraps the step pattern back to octaveOffset 0, but the note-sequence index
    // has advanced to position 2 (67) -- a different pairing than step 0 had.
    REQUIRE(seq[5] == 67);
}
