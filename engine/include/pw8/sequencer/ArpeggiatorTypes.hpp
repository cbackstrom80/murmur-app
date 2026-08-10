#pragma once

#include <array>
#include <cstdint>

// Arpeggiator data model (docs/ARPEGGIATOR.md). Pure data -- no logic here, so this
// header is safe to include from pw8/patch/Patch.hpp without dragging in the
// stateful engine. Execution lives in sequencer/Arpeggiator.hpp, consumed by
// pw8::render::Engine.

namespace pw8::sequencer
{
    /// How the held chord is expanded into a note sequence. Chord mode is the odd
    /// one out: it doesn't arpeggiate at all, it retriggers the whole held chord
    /// together on every step (a rhythmic "chord stab" pattern) -- included because
    /// it reuses 100% of the same step-pattern/timing machinery for no extra cost.
    enum class ArpMode : std::uint8_t
    {
        Up = 0,
        Down,
        UpDown,
        DownUp,
        AsPlayed,
        Random,
        Chord,
    };

    enum class ArpRateMode : std::uint8_t
    {
        Free = 0,
        TempoSync,
    };

    inline constexpr std::size_t kMaxArpSteps = 64;
    inline constexpr std::size_t kMaxHeldNotes = 16;
    inline constexpr int kMaxRatchet = 4;

    struct ArpStep
    {
        bool enabled = true;      ///< false == a rest (silence) for this step.
        int octaveOffset = 0;     ///< -2..+2, added on top of the note sequence's own octave.
        float gate = 0.8f;        ///< 0..1, fraction of the step's duration the note stays held.
        float probability = 1.0f; ///< 0..1, chance this step actually fires (seeded, reproducible).
        int ratchetCount = 1;     ///< 1..kMaxRatchet, subdivides the step into N repeated triggers.
        bool tie = false;         ///< true: don't retrigger -- extend the previous note's gate instead.
        float velocityScale = 1.0f; ///< 0..1, multiplies the held note's velocity.
        bool accent = false;      ///< true: boost velocity for this step.
    };

    struct ArpeggiatorParams
    {
        bool enabled = false;
        ArpMode mode = ArpMode::Up;
        ArpRateMode rateMode = ArpRateMode::TempoSync;
        float rateHz = 8.0f;         ///< used when rateMode == Free.
        int syncDivisionIndex = 6;   ///< used when rateMode == TempoSync; see lfo::kSyncDivisionQuarterNotes.
        int octaveRange = 1;         ///< 1..4, how many octaves the held chord repeats across.
        std::size_t numSteps = 8;    ///< 1..kMaxArpSteps -- length of the step-pattern cycle.
        std::array<ArpStep, kMaxArpSteps> steps{};
        bool latch = false;          ///< true: keep cycling the last chord after all keys release.
    };

} // namespace pw8::sequencer
