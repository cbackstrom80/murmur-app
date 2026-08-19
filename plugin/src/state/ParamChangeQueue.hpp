#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Lock-free SPSC queue: message thread (APVTS listener) produces, audio thread
// consumes once per block. Stores a compact parameter-group index rather than
// scanning all 361 atomics every callback.

namespace pw8::plugin
{
    enum class ParamGroup : std::uint16_t
    {
        Macros = 0,
        Filter,
        Lfo0,
        Lfo1,
        Lfo2,
        Lfo3,
        Lfo4,
        Lfo5,
        Lfo6,
        Lfo7,
        Op0,
        Op1,
        Op2,
        Op3,
        Op4,
        Op5,
        Op6,
        Op7,
        Env0,
        Env1,
        Env2,
        Env3,
        Env4,
        Env5,
        Env6,
        Env7,
        LayerGainPan,
        MasterGain,
        Portamento,
        InsertFx0,
        InsertFx1,
        InsertFx2,
        MasterFx0,
        MasterFx1,
        MasterFx2,
        MasterFx3,
        Arp,
        Unison,
        FxRouting,
        MasterDynamics,
        Generative,
        PeaksUtility,
        Count
    };

    class ParamChangeQueue
    {
    public:
        static constexpr std::size_t kCapacity = 256;

        void push(ParamGroup group) noexcept
        {
            const auto write = writeIndex_.load(std::memory_order_relaxed);
            const auto nextWrite = (write + 1) % kCapacity;
            if (nextWrite == readIndex_.load(std::memory_order_acquire))
                return; // full -- drop (next block's full scan is fallback if needed)

            buffer_[write] = group;
            writeIndex_.store(nextWrite, std::memory_order_release);
        }

        [[nodiscard]] bool pop(ParamGroup& groupOut) noexcept
        {
            const auto read = readIndex_.load(std::memory_order_relaxed);
            if (read == writeIndex_.load(std::memory_order_acquire))
                return false;

            groupOut = buffer_[read];
            readIndex_.store((read + 1) % kCapacity, std::memory_order_release);
            return true;
        }

        void pushAllGroups() noexcept
        {
            for (std::uint16_t i = 0; i < static_cast<std::uint16_t>(ParamGroup::Count); ++i)
                push(static_cast<ParamGroup>(i));
        }

    private:
        std::array<ParamGroup, kCapacity> buffer_{};
        std::atomic<std::size_t> writeIndex_{0};
        std::atomic<std::size_t> readIndex_{0};
    };

} // namespace pw8::plugin
