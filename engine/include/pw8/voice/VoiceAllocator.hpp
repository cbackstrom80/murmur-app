#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

#include "pw8/core/Types.hpp"
#include "pw8/voice/Voice.hpp"

// Fixed-capacity voice pool + allocation/stealing policy.
//
// Stealing priority (see docs/DSP_ENGINE.md "Voice Allocation"):
//   1. A fully free voice, if one exists within the configured polyphony.
//   2. Among RELEASED voices (note-off already sent), the QUIETEST one.
//   3. If none are released, the QUIETEST voice overall.
//   4. Ties broken by OLDEST (lowest age counter).
// Actively-gated steals (all voices still held) are crossfaded in Voice::beginStealCrossfadeNoteOn()
// so filter/operator resets and envelope retrigger don't click; released-voice reuse is quiet
// enough to retrigger without a ramp.

namespace pw8::voice
{
    using VoicePool = std::array<Voice, core::kMaxVoices>;

    enum class AllocateResult : std::uint8_t
    {
        Free,     ///< Voice was idle (noteNumber < 0, amp envelope finished).
        Released, ///< Voice was in release (gate off) -- quietest released candidate.
        Stolen,   ///< No released voice available; an actively-gated voice was taken.
    };

    struct VoiceAllocation
    {
        std::size_t index = 0;
        AllocateResult result = AllocateResult::Free;
    };

    class VoiceAllocator
    {
    public:
        void configure(std::size_t polyphony) noexcept
        {
            polyphony_ = polyphony < 1 ? 1 : (polyphony > core::kMaxVoices ? core::kMaxVoices : polyphony);
        }

        [[nodiscard]] std::size_t getPolyphony() const noexcept { return polyphony_; }

        /// Returns the voice that should handle this note-on (assigned or stolen) and how it was obtained.
        [[nodiscard]] VoiceAllocation allocate(VoicePool& voices) noexcept
        {
            ++ageCounter_;

            for (std::size_t i = 0; i < polyphony_; ++i)
                if (voices[i].isFree())
                    return { i, AllocateResult::Free };

            std::size_t bestReleased = std::numeric_limits<std::size_t>::max();
            float bestReleasedAmp = 1.0e9f;
            std::size_t bestAny = 0;
            float bestAnyAmp = 1.0e9f;
            std::uint64_t bestAnyAge = std::numeric_limits<std::uint64_t>::max();

            for (std::size_t i = 0; i < polyphony_; ++i)
            {
                const float amp = voices[i].amplitudeEstimate();
                if (voices[i].isReleased() && amp < bestReleasedAmp)
                {
                    bestReleasedAmp = amp;
                    bestReleased = i;
                }
                if (amp < bestAnyAmp || (amp == bestAnyAmp && voices[i].age < bestAnyAge))
                {
                    bestAnyAmp = amp;
                    bestAnyAge = voices[i].age;
                    bestAny = i;
                }
            }

            if (bestReleased != std::numeric_limits<std::size_t>::max())
                return { bestReleased, AllocateResult::Released };
            return { bestAny, AllocateResult::Stolen };
        }

        [[nodiscard]] std::uint64_t nextAge() const noexcept { return ageCounter_; }

        /// Returns a gated voice playing `(note, channel)`, if any — used for same-note retrigger.
        [[nodiscard]] std::optional<std::size_t> findGatedVoice(const VoicePool& voices, int note, int channel) const noexcept
        {
            for (std::size_t i = 0; i < polyphony_; ++i)
            {
                if (voices[i].gateOn && voices[i].noteNumber == note &&
                    (channel < 0 || voices[i].midiChannel == channel))
                    return i;
            }
            return std::nullopt;
        }

        /// Sends note-off to every active, gated voice matching (note, channel).
        /// `channel < 0` matches any channel (used for omni / non-MPE mode).
        void release(VoicePool& voices, int note, int channel, float releaseVelocity) noexcept
        {
            for (std::size_t i = 0; i < polyphony_; ++i)
            {
                auto& v = voices[i];
                if (v.gateOn && v.noteNumber == note && (channel < 0 || v.midiChannel == channel))
                    v.noteOff(releaseVelocity);
            }
        }

        /// Sends note-off to every currently gated voice (e.g. MIDI panic / all-notes-off).
        void releaseAll(VoicePool& voices, float releaseVelocity) noexcept
        {
            for (std::size_t i = 0; i < polyphony_; ++i)
                if (voices[i].gateOn)
                    voices[i].noteOff(releaseVelocity);
        }

        /// Immediately silences every sounding voice (MIDI all-sound-off / transport stop).
        void killAll(VoicePool& voices) noexcept
        {
            for (std::size_t i = 0; i < polyphony_; ++i)
                if (voices[i].isSounding() || voices[i].gateOn)
                    voices[i].hardKill();
        }

    private:
        std::size_t polyphony_ = core::kDefaultVoices;
        std::uint64_t ageCounter_ = 0;
    };

} // namespace pw8::voice
