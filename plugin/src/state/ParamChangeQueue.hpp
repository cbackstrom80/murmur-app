#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

// Atomic dirty bitmask: any thread marks groups dirty, audio thread consumes
// once per block. Stores a compact parameter-group index rather than scanning
// all 361 atomics every callback.
//
// Replaces a previous lossy SPSC ring-buffer design (kept working as the
// producer/consumer roles here suggested, but pushLiveParametersToEngine()
// stopped trusting it -- see MurmurProcessor.cpp's own history). Two real
// problems with the ring buffer, not just style:
//   1. It could silently drop entries on overflow (fixed-capacity ring,
//      "full -- drop"), which is a real correctness bug -- a dropped entry
//      meant a knob drag never reached the engine.
//   2. Its "message thread produces" comment describes an assumption already
//      violated in practice: parameterChanged() (the sole producer) runs
//      synchronously from whatever thread calls RangedAudioParameter::
//      setValue(), and hosts apply automation from the audio thread as well
//      as the message thread doing UI drags -- multiple concurrent
//      producers, not one.
// An atomic bitmask fixes both: fetch_or is safe under any number of
// concurrent producers, and can never overflow or drop a bit -- marking the
// same group dirty twice before a drain just coalesces to one bit, which is
// correct (the *value* is always re-read fresh from APVTS at drain time;
// this bitmask only ever signals "something in this group changed since the
// last drain," never carries the value itself).

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

    static_assert(static_cast<std::size_t>(ParamGroup::Count) <= 64,
                  "ParamGroup must fit in a uint64_t dirty mask -- see ParamDirtyMask below.");

    class ParamDirtyMask
    {
    public:
        void markDirty(ParamGroup group) noexcept
        {
            mask_.fetch_or(std::uint64_t{1} << static_cast<unsigned>(group), std::memory_order_release);
        }

        void markAllDirty() noexcept { mask_.fetch_or(kAllGroupsMask, std::memory_order_release); }

        /// Audio-thread only, once per block: atomically reads and clears the mask.
        [[nodiscard]] std::uint64_t consume() noexcept { return mask_.exchange(0, std::memory_order_acq_rel); }

    private:
        static constexpr std::uint64_t kAllGroupsMask =
            (std::uint64_t{1} << static_cast<unsigned>(ParamGroup::Count)) - 1;
        // Dirty on construction: the very first block after prepare() has no
        // prior drain to rely on, so it must full-rebuild -- matches the old
        // ring buffer's documented "first block after prepare" fallback.
        std::atomic<std::uint64_t> mask_{kAllGroupsMask};
    };
    static_assert(std::atomic<std::uint64_t>::is_always_lock_free,
                  "ParamDirtyMask::consume()/markDirty() must stay lock-free -- called from the audio thread.");

} // namespace pw8::plugin
