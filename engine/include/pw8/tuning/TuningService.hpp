#pragma once

#include "pw8/dsp/Math.hpp"

// Centralized pitch/tuning service. Oscillators never compute note->frequency
// themselves -- they always receive a resolved Hz value from here (or from a Voice
// that consulted here), so tuning tables never leak into DSP code that shouldn't
// need to know about them.
//
// Status: 12-TET only in this pass. Scala (.scl/.kbm) file import and MTS-ESP client
// support are architected for (this is why TuningService is a class with a stable
// noteToFrequency() entry point rather than a free function baked into the oscillator)
// but not yet implemented -- see docs/ROADMAP.md.

namespace pw8::tuning
{
    class TuningService
    {
    public:
        explicit TuningService(float a4Hz = 440.0f) noexcept : a4Hz_(a4Hz) {}

        void setA4(float hz) noexcept { a4Hz_ = hz; }
        [[nodiscard]] float getA4() const noexcept { return a4Hz_; }

        /// `note` may be fractional (MIDI note + fine cents as a float) for microtonal
        /// / pitch-modulated lookups.
        [[nodiscard]] float noteToFrequency(float note) const noexcept
        {
            return dsp::noteToFrequency12Tet(note, a4Hz_);
        }

    private:
        float a4Hz_ = 440.0f;
    };

} // namespace pw8::tuning
