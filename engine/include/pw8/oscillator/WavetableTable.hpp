#pragma once

#include <cstdint>
#include <vector>

#include "pw8/oscillator/WavetableOscillator.hpp"

// Multi-mip wavetable content: one full-bandwidth mip plus progressively
// harmonic-truncated mips, generated offline by murmur-wavetable-builder (see
// tools/wavetable_builder). This is the runtime data model -- pure data, no JSON
// dependency, safe to include from realtime-adjacent headers (op::OperatorState
// holds a pointer to one of these). Loading a table from disk is a separate,
// JSON-dependent, control-path-only concern -- see WavetableTableLoader.hpp.
//
// This is what finishes the mip-mapping half of docs/DSP_ENGINE.md's "Wavetable
// Warp" / "Wavetable Mipmapping" sections: each mip is band-limited to a known
// maximum harmonic via FFT-based harmonic truncation (pw8/dsp/Fft.hpp), and
// viewForFrequency() picks the highest-fidelity mip that won't alias at the current
// note frequency and actual output sample rate.

namespace pw8::oscillator
{
    inline constexpr std::size_t kMaxWavetableMipLevels = 16;

    class WavetableTable
    {
    public:
        struct MipLevel
        {
            std::vector<float> samples; // numFrames * samplesPerFrame, frame-major.
            int maxHarmonic = 0;        // highest harmonic index retained in this mip.
        };

        int numFrames = 0;
        int samplesPerFrame = 0;
        std::vector<MipLevel> mips; // ordered highest-fidelity (largest maxHarmonic) first.

        [[nodiscard]] bool isValid() const noexcept
        {
            return numFrames > 0 && samplesPerFrame > 1 && !mips.empty();
        }

        /// Picks the highest-fidelity mip that stays safely below Nyquist at
        /// `frequencyHz` and `sampleRate`, falling back to the most band-limited mip
        /// available if even that one would alias (extreme high notes). Cheap (a
        /// short linear scan over `mips`, no allocation) -- safe to call every
        /// sample from the audio thread, which is what op::OperatorState does.
        [[nodiscard]] WavetableView viewForFrequency(float frequencyHz, double sampleRate) const noexcept
        {
            if (!isValid() || frequencyHz <= 0.0f)
                return {};

            const float nyquist = static_cast<float>(sampleRate) * 0.5f;
            // Small safety margin below the true Nyquist limit: brick-wall harmonic
            // truncation plus the oscillator's own linear interpolation both have a
            // little practical rolloff, so backing off ~10% avoids edge-case aliasing
            // right at the boundary between two mip levels.
            const float safeLimit = nyquist * 0.9f;

            for (const auto& mip : mips)
            {
                const float highestHarmonicFreq = static_cast<float>(mip.maxHarmonic) * frequencyHz;
                if (highestHarmonicFreq <= safeLimit)
                    return makeView(mip);
            }
            return makeView(mips.back()); // most band-limited available -- best effort at extreme registers.
        }

    private:
        [[nodiscard]] WavetableView makeView(const MipLevel& mip) const noexcept
        {
            return WavetableView{mip.samples.data(), numFrames, samplesPerFrame};
        }
    };

} // namespace pw8::oscillator
