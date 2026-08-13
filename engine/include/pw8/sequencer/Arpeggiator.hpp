#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "pw8/core/Types.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/dsp/Random.hpp"
#include "pw8/lfo/Lfo.hpp" // reuses kSyncDivisionQuarterNotes -- one division table, not two.
#include "pw8/sequencer/ArpeggiatorTypes.hpp"

// Arpeggiator engine (docs/ARPEGGIATOR.md). Intercepts held notes (fed by
// noteHeld()/noteReleased() instead of going straight to voices) and, ticked once
// per sample from render::Engine::process(), emits synthetic note-on/note-off
// events on a tempo-locked clock. No allocation, no locking -- every container is
// fixed-capacity, matching the realtime rules in docs/ARCHITECTURE.md.
//
// Two independent, simultaneously-cycling counters give the classic
// "polymetric" arpeggiator feel: the STEP pattern (up to 64 steps, each with its
// own probability/gate/ratchet/tie/accent/octave modifiers) and the NOTE SEQUENCE
// (the held chord expanded across octaveRange, ordered per ArpMode) advance
// together but wrap at different lengths whenever they don't divide evenly --
// e.g. an 8-step pattern over a 3-note chord drifts through a different
// step-modifier/note pairing every 24 steps rather than repeating every 3.

namespace pw8::sequencer
{
    enum class ArpEventType : std::uint8_t
    {
        NoteOn,
        NoteOff,
    };

    struct ArpEvent
    {
        ArpEventType type = ArpEventType::NoteOn;
        int note = 60;
        int channel = 0;
        float velocityUnit = 1.0f;
    };

    class Arpeggiator
    {
    public:
        void configure(const ArpeggiatorParams& params, std::uint64_t seed) noexcept
        {
            params_ = params;
            rng_.reseed(seed);
            heldNotes_.clear();
            patternNotes_.clear();
            noteSequence_.clear();
            currentStepIndex_ = 0;
            noteSequenceIndex_ = 0;
            nextOrderIndex_ = 0;
            stepSampleCounter_ = 0;
            for (auto& slot : pendingNoteOffs_)
                slot.active = false;
            for (auto& slot : pendingTriggers_)
                slot.active = false;
        }

        /// Swaps in new parameters WITHOUT resetting held notes, pattern position, or
        /// pending events -- unlike configure(). For live automation of the scalar
        /// fields (rate/mode/octaveRange/latch/...) from a running plugin: every index
        /// derived from `numSteps`/`noteSequence_.size()` is already taken modulo that
        /// size on every use (see fireStep()/tieLookaheadSamples()), so swapping
        /// `params_` mid-stream can never read out of bounds even if numSteps shrinks
        /// between one tick and the next. See docs/PLUGIN_ARCHITECTURE.md "Automation".
        void setLiveParams(const ArpeggiatorParams& params) noexcept { params_ = params; }

        [[nodiscard]] std::size_t getCurrentStepIndex() const noexcept { return currentStepIndex_; }
        [[nodiscard]] std::size_t getNoteSequenceIndex() const noexcept { return noteSequenceIndex_; }

        void prepare(double sampleRate) noexcept { sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0; }

        void noteHeld(int note, int channel, float velocityUnit) noexcept
        {
            for (auto& h : heldNotes_)
            {
                if (h.note == note && h.channel == channel)
                {
                    h.velocityUnit = velocityUnit;
                    return;
                }
            }
            if (heldNotes_.size() < heldNotes_.capacity())
                heldNotes_.push_back(HeldNote{note, channel, velocityUnit, nextOrderIndex_++});
            syncPatternAndRebuild();
        }

        void noteReleased(int note, int channel) noexcept
        {
            core::FixedVector<HeldNote, kMaxHeldNotes> kept;
            for (const auto& h : heldNotes_)
                if (!(h.note == note && h.channel == channel))
                    kept.push_back(h);
            heldNotes_ = kept;
            syncPatternAndRebuild();
        }

        /// Advances the clock by one sample and appends any events that fire this
        /// sample into `outEvents` (usually empty; a handful at most with ratchets).
        void tick(double sampleRate, float bpm, core::FixedVector<ArpEvent, 32>& outEvents) noexcept
        {
            sampleRate_ = sampleRate > 0.0 ? sampleRate : sampleRate_;
            lastBpm_ = bpm;

            // Pending note-offs and delayed ratchet triggers run regardless of whether
            // the arp is currently enabled, so a live disable doesn't strand a held
            // voice or silently drop an already-scheduled ratchet sub-hit.
            for (auto& slot : pendingNoteOffs_)
            {
                if (!slot.active)
                    continue;
                if (--slot.samplesRemaining <= 0)
                {
                    outEvents.push_back(ArpEvent{ArpEventType::NoteOff, slot.note, slot.channel, 0.0f});
                    slot.active = false;
                }
            }

            for (auto& slot : pendingTriggers_)
            {
                if (!slot.active)
                    continue;
                if (--slot.samplesRemaining <= 0)
                {
                    outEvents.push_back(ArpEvent{ArpEventType::NoteOn, slot.note, slot.channel, slot.velocityUnit});
                    scheduleNoteOff(slot.note, slot.channel, slot.gateSamples);
                    slot.active = false;
                }
            }

            if (!params_.enabled || patternNotes_.empty() || params_.numSteps == 0)
                return;

            if (stepSampleCounter_ <= 0)
            {
                fireStep(outEvents);
                stepSampleCounter_ = stepLengthSamples(sampleRate_, bpm);
            }
            else
            {
                --stepSampleCounter_;
            }
        }

    private:
        struct HeldNote
        {
            int note = 60;
            int channel = 0;
            float velocityUnit = 1.0f;
            std::uint64_t orderIndex = 0;
        };

        struct SequencedNote
        {
            int note = 60;
            float velocityUnit = 1.0f;
        };

        struct PendingNoteOff
        {
            bool active = false;
            int note = 60;
            int channel = 0;
            int samplesRemaining = 0;
        };

        /// A ratchet sub-hit after the first (which fires immediately from fireStep()):
        /// waits `samplesRemaining` then fires its own NoteOn + schedules its own
        /// note-off, so a ratcheted step's repeats actually spread across the step's
        /// duration instead of all landing on the same sample.
        struct PendingTrigger
        {
            bool active = false;
            int note = 60;
            int channel = 0;
            float velocityUnit = 1.0f;
            int samplesRemaining = 0;
            int gateSamples = 1;
        };

        void syncPatternAndRebuild() noexcept
        {
            if (!heldNotes_.empty() || !params_.latch)
                patternNotes_ = heldNotes_;
            rebuildNoteSequence();
        }

        void rebuildNoteSequence() noexcept
        {
            noteSequence_.clear();
            noteSequenceIndex_ = 0;
            if (patternNotes_.empty())
                return;

            core::FixedVector<HeldNote, kMaxHeldNotes> ordered = patternNotes_;
            if (params_.mode == ArpMode::AsPlayed || params_.mode == ArpMode::Random)
            {
                std::sort(ordered.begin(), ordered.end(),
                          [](const HeldNote& a, const HeldNote& b) { return a.orderIndex < b.orderIndex; });
            }
            else
            {
                std::sort(ordered.begin(), ordered.end(), [](const HeldNote& a, const HeldNote& b) { return a.note < b.note; });
            }

            const int octaves = dsp::clamp(params_.octaveRange, 1, 4);

            core::FixedVector<SequencedNote, kMaxHeldNotes * 4> ascending;
            for (int o = 0; o < octaves; ++o)
                for (const auto& h : ordered)
                    ascending.push_back(SequencedNote{h.note + o * 12, h.velocityUnit});

            switch (params_.mode)
            {
                case ArpMode::Up:
                case ArpMode::AsPlayed:
                    noteSequence_ = ascending;
                    break;

                case ArpMode::Down:
                    for (std::size_t i = ascending.size(); i-- > 0;)
                        noteSequence_.push_back(ascending[i]);
                    break;

                case ArpMode::UpDown:
                {
                    noteSequence_ = ascending;
                    if (ascending.size() > 2)
                        for (std::size_t i = ascending.size() - 2; i-- > 0;)
                            noteSequence_.push_back(ascending[i + 1]);
                    break;
                }

                case ArpMode::DownUp:
                {
                    for (std::size_t i = ascending.size(); i-- > 0;)
                        noteSequence_.push_back(ascending[i]);
                    if (ascending.size() > 2)
                        for (std::size_t i = 1; i + 1 < ascending.size(); ++i)
                            noteSequence_.push_back(ascending[i]);
                    break;
                }

                case ArpMode::Random:
                {
                    noteSequence_ = ascending;
                    shuffle(noteSequence_);
                    break;
                }

                case ArpMode::Chord:
                    // Chord mode doesn't use noteSequence_ at all -- see fireStep().
                    break;
            }
        }

        void shuffle(core::FixedVector<SequencedNote, kMaxHeldNotes * 4>& seq) noexcept
        {
            // Deterministic Fisher-Yates using the arp's own seeded RNG.
            for (std::size_t i = seq.size(); i-- > 1;)
            {
                const auto j = static_cast<std::size_t>(rng_.nextRange(0.0f, static_cast<float>(i) + 0.999f));
                std::swap(seq[i], seq[j]);
            }
        }

        [[nodiscard]] int stepLengthSamples(double sampleRate, float bpm) const noexcept
        {
            float rateHz = params_.rateHz;
            if (params_.rateMode == ArpRateMode::TempoSync)
            {
                const auto idx = static_cast<std::size_t>(
                    dsp::clamp(params_.syncDivisionIndex, 0, static_cast<int>(lfo::kSyncDivisionQuarterNotes.size()) - 1));
                const float quarterNotesPerCycle = lfo::kSyncDivisionQuarterNotes[idx];
                const float quarterNotesPerSecond = dsp::clamp(bpm, 20.0f, 999.0f) / 60.0f;
                rateHz = quarterNotesPerSecond / quarterNotesPerCycle;
            }
            rateHz = dsp::clamp(rateHz, 0.1f, 100.0f);
            const int samples = static_cast<int>(sampleRate / static_cast<double>(rateHz));
            return samples > 1 ? samples : 1;
        }

        void scheduleNoteOff(int note, int channel, int samplesFromNow) noexcept
        {
            for (auto& slot : pendingNoteOffs_)
            {
                if (!slot.active)
                {
                    slot.active = true;
                    slot.note = note;
                    slot.channel = channel;
                    slot.samplesRemaining = samplesFromNow > 0 ? samplesFromNow : 1;
                    return;
                }
            }
            // Pool exhausted (very heavy ratchet load) -- drop silently rather than
            // allocate; a stuck-note risk is bounded by the step rate itself.
        }

        void scheduleTrigger(int note, int channel, float velocityUnit, int samplesFromNow, int gateSamples) noexcept
        {
            for (auto& slot : pendingTriggers_)
            {
                if (!slot.active)
                {
                    slot = PendingTrigger{true, note, channel, velocityUnit,
                                           samplesFromNow > 0 ? samplesFromNow : 1, gateSamples};
                    return;
                }
            }
            // Pool exhausted -- same policy as scheduleNoteOff(): drop silently.
        }

        /// Sums the full duration of every TIE step immediately following the step at
        /// `afterStepIndex` (wrapping around the pattern), so a fired note's scheduled
        /// note-off already accounts for the steps that will hold it through rather
        /// than needing to "extend" an event that may well have already fired by the
        /// time a later tick reaches the tied step -- see docs/ARPEGGIATOR.md "Tie"
        /// for why the naive extend-on-arrival approach doesn't work.
        [[nodiscard]] int tieLookaheadSamples(std::size_t afterStepIndex) const noexcept
        {
            const auto numSteps = dsp::clamp<std::size_t>(params_.numSteps, 1, kMaxArpSteps);
            std::size_t idx = (afterStepIndex + 1) % numSteps;
            int extra = 0;
            for (std::size_t guard = 0; guard < numSteps && params_.steps[idx].tie; ++guard)
            {
                extra += stepLengthSamples(sampleRate_, lastBpm_);
                idx = (idx + 1) % numSteps;
            }
            return extra;
        }

        void fireStep(core::FixedVector<ArpEvent, 32>& outEvents) noexcept
        {
            const auto numSteps = dsp::clamp<std::size_t>(params_.numSteps, 1, kMaxArpSteps);
            const auto stepIdx = currentStepIndex_ % numSteps;
            const auto& step = params_.steps[stepIdx];
            ++currentStepIndex_;

            const int stepSamples = stepLengthSamples(sampleRate_, lastBpm_);

            // A tie step never fires on its own -- the step it's tying FROM already
            // extended its note-off to cover this step's duration (tieLookaheadSamples).
            // It also doesn't advance the note sequence: it's still "on" the same note.
            if (step.tie)
                return;

            if (!step.enabled)
                return; // a rest: doesn't fire, doesn't advance the note sequence either.

            if (rng_.nextFloat() >= dsp::clamp(step.probability, 0.0f, 1.0f))
            {
                // Skipped by probability: the pattern still moves on to the next note.
                if (params_.mode != ArpMode::Chord && !noteSequence_.empty())
                    noteSequenceIndex_ = (noteSequenceIndex_ + 1) % noteSequence_.size();
                return;
            }

            const int ratchet = dsp::clamp(step.ratchetCount, 1, kMaxRatchet);
            const int subStepSamples = std::max(1, stepSamples / ratchet);
            const int tieExtension = tieLookaheadSamples(stepIdx);
            const float velocityMul = dsp::clamp(step.velocityScale * (step.accent ? 1.3f : 1.0f), 0.0f, 1.0f);

            // Ratchet repeats THIS step's note (or, in Chord mode, this step's whole
            // chord) `ratchet` times, spread evenly across the step's duration -- it is
            // not a way to advance through extra notes. Sub-hit 0 fires immediately
            // (into outEvents, this sample); sub-hits 1..ratchet-1 are scheduled via
            // pendingTriggers_ so they land on their own later sample, not all at once.
            // Only the LAST sub-hit absorbs the tie lookahead extension.
            auto fireOneNote = [&](int note, int channel, float velocityUnit) {
                for (int r = 0; r < ratchet; ++r)
                {
                    const bool isLast = (r == ratchet - 1);
                    const float gateSamples =
                        std::max(1.0f, static_cast<float>(subStepSamples) * dsp::clamp(step.gate, 0.01f, 1.0f)) +
                        (isLast ? static_cast<float>(tieExtension) : 0.0f);
                    const int offset = subStepSamples * r;

                    if (r == 0)
                    {
                        outEvents.push_back(ArpEvent{ArpEventType::NoteOn, note, channel, velocityUnit});
                        scheduleNoteOff(note, channel, static_cast<int>(gateSamples));
                    }
                    else
                    {
                        scheduleTrigger(note, channel, velocityUnit, offset, static_cast<int>(gateSamples));
                    }
                }
            };

            if (params_.mode == ArpMode::Chord)
            {
                for (const auto& h : patternNotes_)
                    fireOneNote(h.note + step.octaveOffset * 12, h.channel, dsp::clamp(h.velocityUnit * velocityMul, 0.0f, 1.0f));
            }
            else if (!noteSequence_.empty())
            {
                const auto& sn = noteSequence_[noteSequenceIndex_ % noteSequence_.size()];
                fireOneNote(sn.note + step.octaveOffset * 12, 0, dsp::clamp(sn.velocityUnit * velocityMul, 0.0f, 1.0f));

                noteSequenceIndex_ = (noteSequenceIndex_ + 1) % noteSequence_.size();
                if (noteSequenceIndex_ == 0 && params_.mode == ArpMode::Random)
                    shuffle(noteSequence_);
            }
        }

        ArpeggiatorParams params_{};
        double sampleRate_ = 48000.0;
        float lastBpm_ = 120.0f;

        core::FixedVector<HeldNote, kMaxHeldNotes> heldNotes_;
        core::FixedVector<HeldNote, kMaxHeldNotes> patternNotes_;
        core::FixedVector<SequencedNote, kMaxHeldNotes * 4> noteSequence_;

        std::size_t currentStepIndex_ = 0;
        std::size_t noteSequenceIndex_ = 0;
        std::uint64_t nextOrderIndex_ = 0;
        int stepSampleCounter_ = 0;

        std::array<PendingNoteOff, 32> pendingNoteOffs_{};
        std::array<PendingTrigger, 32> pendingTriggers_{};

        dsp::DeterministicRng rng_;
    };

} // namespace pw8::sequencer
