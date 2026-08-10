#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

#include "pw8/core/Types.hpp"
#include "pw8/voice/Voice.hpp"

// Fixed-capacity voice pool + allocation/stealing policy.
//
// Stealing priority (see docs/DSP_ENGINE.md "Voice Allocation"):
//   1. A fully free voice, if one exists within the configured polyphony.
//   2. Among RELEASED voices (note-off already sent), the QUIETEST one.
//   3. If none are released, the QUIETEST voice overall.
//   4. Ties broken by OLDEST (lowest age counter).
// Stealing does not crossfade in this pass (documented PLANNED) but a stolen voice's
// envelope is retriggered from Delay rather than hard-reset, so a short shared-articulation
// click is avoided in the common case; true stolen-voice ramping is tracked in
// docs/ROADMAP.md Phase 7.

namespace pw8::voice
{
    using VoicePool = std::array<Voice, core::kMaxVoices>;

    class VoiceAllocator
    {
    public:
        void configure(std::size_t polyphony) noexcept
        {
            polyphony_ = polyphony < 1 ? 1 : (polyphony > core::kMaxVoices ? core::kMaxVoices : polyphony);
        }

        [[nodiscard]] std::size_t getPolyphony() const noexcept { return polyphony_; }

        /// Returns the index of the voice that should handle this note-on (assigned or stolen).
        [[nodiscard]] std::size_t allocate(VoicePool& voices) noexcept
        {
            ++ageCounter_;

            for (std::size_t i = 0; i < polyphony_; ++i)
                if (voices[i].isFree())
                    return i;

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

            return bestReleased != SIZE_MAX ? bestReleased : bestAny;
        }

        [[nodiscard]] std::uint64_t nextAge() const noexcept { return ageCounter_; }

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

    private:
        std::size_t polyphony_ = core::kDefaultVoices;
        std::uint64_t ageCounter_ = 0;
    };

} // namespace pw8::voice
