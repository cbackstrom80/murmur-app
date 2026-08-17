#pragma once

#include <array>

#include "pw8/effects/Chorus.hpp"
#include "pw8/effects/Compressor.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/Eq.hpp"
#include "pw8/effects/FractalEcho.hpp"
#include "pw8/effects/FreqShiftEcho.hpp"
#include "pw8/effects/Limiter.hpp"
#include "pw8/effects/NodeDelay.hpp"
#include "pw8/effects/Reverb.hpp"
#include "pw8/effects/Saturation.hpp"
#include "pw8/effects/TapeDelay.hpp"
#include "pw8/effects/Vocoder.hpp"

// One slot's worth of processing state (one instance of each of the ten
// algorithms) plus `EffectChain<N>`, N slots run in series. Every processor
// instance is always constructed (a plain struct-of-all rather than a tagged
// union/variant, matching this codebase's existing style of dispatch-by-enum over
// every other engine/effect-type switch -- see `algorithm::EngineType`,
// `filter::FilterMode`, `sequencer::ArpMode`) -- only the one matching
// `EffectSlotParams::type` is ever exercised on a given sample, but all ten keep
// their own state (delay lines etc.) alive across a live `type` change, so
// switching a slot's effect type mid-performance never glitches from
// reading/writing another effect's stale buffers.
namespace pw8::effects
{
    class EffectSlotProcessor
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            saturation_.prepare(sampleRate);
            chorus_.prepare(sampleRate);
            tapeDelay_.prepare(sampleRate);
            nodeDelay_.prepare(sampleRate);
            freqShiftEcho_.prepare(sampleRate);
            fractalEcho_.prepare(sampleRate);
            reverb_.prepare(sampleRate);
            eq_.prepare(sampleRate);
            compressor_.prepare(sampleRate);
            limiter_.prepare(sampleRate);
            vocoder_.prepare(sampleRate);
        }

        void reset() noexcept
        {
            saturation_.reset();
            chorus_.reset();
            tapeDelay_.reset();
            nodeDelay_.reset();
            freqShiftEcho_.reset();
            fractalEcho_.reset();
            reverb_.reset();
            eq_.reset();
            compressor_.reset();
            limiter_.reset();
            vocoder_.reset();
        }

        void processStereo(float inL, float inR, float sidechainL, float sidechainR, const EffectSlotParams& p,
                           float& outL, float& outR, bool sidechainConnected = false, float bpm = 120.0f) noexcept
        {
            switch (p.type)
            {
                case EffectType::Bypass: outL = inL; outR = inR; return;
                case EffectType::Saturation: saturation_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::Chorus: chorus_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::TapeDelay: tapeDelay_.processStereo(inL, inR, p, outL, outR, bpm); return;
                case EffectType::NodeDelay: nodeDelay_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::FreqShiftEcho: freqShiftEcho_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::FractalEcho: fractalEcho_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::Reverb: reverb_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::Eq: eq_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::Compressor: compressor_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::Limiter: limiter_.processStereo(inL, inR, p, outL, outR); return;
                case EffectType::Vocoder:
                    vocoder_.processStereo(inL, inR, sidechainL, sidechainR, p, outL, outR, sidechainConnected);
                    return;
            }
            outL = inL;
            outR = inR;
        }

        [[nodiscard]] float getCompressorGainReductionDb() const noexcept
        {
            return compressor_.getGainReductionDb();
        }

    private:
        SaturationProcessor saturation_{};
        ChorusProcessor chorus_{};
        TapeDelayProcessor tapeDelay_{};
        NodeDelayProcessor nodeDelay_{};
        FreqShiftEchoProcessor freqShiftEcho_{};
        FractalEchoProcessor fractalEcho_{};
        ReverbProcessor reverb_{};
        EqProcessor eq_{};
        CompressorProcessor compressor_{};
        LimiterProcessor limiter_{};
        VocoderProcessor vocoder_{};
    };

    /// `NumSlots` `EffectSlotProcessor`s run in series (slot 0's output feeds slot
    /// 1's input, and so on) -- `LayerInsertChain` (3 slots, docs/FX_BANK.md) or
    /// `MasterChain` (4 slots) below.
    template <std::size_t NumSlots>
    class EffectChain
    {
    public:
        void prepare(double sampleRate) noexcept
        {
            for (auto& slot : slots_)
                slot.prepare(sampleRate);
        }

        void reset() noexcept
        {
            for (auto& slot : slots_)
                slot.reset();
        }

        void process(const std::array<EffectSlotParams, NumSlots>& params, float& l, float& r, float sidechainL,
                     float sidechainR, bool sidechainConnected = false, float bpm = 120.0f) noexcept
        {
            process(params, defaultOrder(), l, r, sidechainL, sidechainR, sidechainConnected, bpm);
        }

        void process(const std::array<EffectSlotParams, NumSlots>& params,
                     const std::array<std::uint8_t, NumSlots>& slotOrder, float& l, float& r, float sidechainL,
                     float sidechainR, bool sidechainConnected = false, float bpm = 120.0f) noexcept
        {
            for (std::size_t i = 0; i < NumSlots; ++i)
            {
                const std::size_t paramIndex = slotOrder[i] < NumSlots ? slotOrder[i] : i;
                float outL = l, outR = r;
                slots_[i].processStereo(l, r, sidechainL, sidechainR, params[paramIndex], outL, outR,
                                          sidechainConnected, bpm);
                l = outL;
                r = outR;
            }
        }

        [[nodiscard]] static std::array<std::uint8_t, NumSlots> defaultOrder() noexcept
        {
            std::array<std::uint8_t, NumSlots> order{};
            for (std::size_t i = 0; i < NumSlots; ++i)
                order[i] = static_cast<std::uint8_t>(i);
            return order;
        }

        [[nodiscard]] float getCompressorGainReductionDb(std::size_t slot) const noexcept
        {
            return slot < NumSlots ? slots_[slot].getCompressorGainReductionDb() : 0.0f;
        }

    private:
        std::array<EffectSlotProcessor, NumSlots> slots_{};
    };

    using LayerInsertChain = EffectChain<kNumLayerInsertSlots>;
    using MasterChain = EffectChain<kNumMasterSlots>;

} // namespace pw8::effects
