#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace pw8::plugin
{
    /// Default CC → MURMUR macro / master mapping for master keyboards (Kawai MP11SE).
    /// Applied on the audio thread before APVTS values are pushed to the engine so
    /// hardware knobs move the same macros surfaced in Knobs of Interest.
    inline void applyPerformanceCcToApvts(int controller, int value7,
                                          const std::array<std::atomic<float>*, 8>& macroPointers,
                                          std::atomic<float>* masterGainPointer) noexcept
    {
        const float normalized = static_cast<float>(value7) / 127.0f;

        switch (controller)
        {
            case 7: // Channel volume — master output
                if (masterGainPointer != nullptr)
                    masterGainPointer->store(normalized, std::memory_order_relaxed);
                break;
            case 11: // Expression pedal → Macro 2 (also mod-matrix Expression source)
                if (macroPointers[1] != nullptr)
                    macroPointers[1]->store(normalized, std::memory_order_relaxed);
                break;
            case 74: // Brightness / timbre → Macro 3
                if (macroPointers[2] != nullptr)
                    macroPointers[2]->store(normalized, std::memory_order_relaxed);
                break;
            case 71: // Harmonic content → Macro 4
                if (macroPointers[3] != nullptr)
                    macroPointers[3]->store(normalized, std::memory_order_relaxed);
                break;
            case 73: // Attack time → Macro 5
                if (macroPointers[4] != nullptr)
                    macroPointers[4]->store(normalized, std::memory_order_relaxed);
                break;
            case 91: // Reverb send (GM) → Macro 6
                if (macroPointers[5] != nullptr)
                    macroPointers[5]->store(normalized, std::memory_order_relaxed);
                break;
            case 93: // Chorus send (GM) → Macro 7
                if (macroPointers[6] != nullptr)
                    macroPointers[6]->store(normalized, std::memory_order_relaxed);
                break;
            default:
                break;
        }
    }

} // namespace pw8::plugin
