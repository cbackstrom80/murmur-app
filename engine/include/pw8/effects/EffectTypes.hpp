#pragma once

#include <array>
#include <cstdint>

// Pure data model for the FX bank (docs/FX_BANK.md). Included by Patch.hpp, so --
// like every other *Types.hpp in this codebase -- it must stay free of any
// stateful DSP or JSON dependency. The stateful processors live in
// pw8/effects/EffectChain.hpp, included only where audio is actually rendered.
//
// Layout follows the master spec: 3 layer insert slots (LayerPatch::insertEffects)
// applied to that layer's summed voice output, plus 4 master slots
// (Patch::masterEffects) applied to the final mixed stereo bus. Each slot is one
// `EffectSlotParams` -- a flat struct with fields for every effect type (the same
// "all fields present, only the active type's are used" pattern already used by
// `patch::OperatorPatch` for its 5 engine types), so changing a slot's `type` at
// runtime never reallocates or restructures anything.
namespace pw8::effects
{
    /// Ten real, independently-voiced algorithms plus Bypass -- not ten variations
    /// on one delay engine. See docs/FX_BANK.md for the research and design behind
    /// each: TapeDelay/NodeDelay/FreqShiftEcho are informed by (not ported from)
    /// Cocoa Delay, ChowMatrix, and ValhallaFreqEcho respectively; FractalEcho is
    /// this project's own invention. Eq/Compressor/Limiter round out the master
    /// spec's "first effect set" basics (docs/ROADMAP.md "GATE 10"). Reverb (GATE
    /// 10, redesigned in GATE 11) is informed by (not ported from) a family of
    /// well-published, openly-documented algorithmic reverb techniques: Jot's
    /// Householder-matrix FDN, Schroeder/Dattorro-style series-allpass input
    /// diffusion, CloudSeed's many-delay-line approach to density/"massive"
    /// character, and -- for the frequency-dependent decay architecture
    /// specifically -- the parameter design documented in the Bricasti M7 owner's
    /// manual.
    enum class EffectType : std::uint8_t
    {
        Bypass = 0,
        Saturation,
        Chorus,
        TapeDelay,
        NodeDelay,
        FreqShiftEcho,
        FractalEcho,
        Reverb,
        Eq,
        Compressor,
        Limiter,
        BinauralSpace, ///< Quasar-inspired global binaural spatial mixer (see docs/GLOBAL_QUASAR_FX_PLAN.md).
    };

    enum class DelayPanMode : std::uint8_t
    {
        Static = 0, ///< each channel is its own independent delay line.
        PingPong,   ///< mono-summed input alternates hard-left/hard-right per repeat.
        Circular,   ///< the wet signal's pan continuously rotates L<->R.
    };

    inline constexpr std::size_t kMaxDelayNodes = 6;
    inline constexpr std::size_t kNumLayerInsertSlots = 3;
    inline constexpr std::size_t kNumMasterSlots = 4;
    inline constexpr float kMaxEffectDelaySeconds = 2.0f;     ///< TapeDelay / FreqShiftEcho.
    inline constexpr float kMaxTreeNodeDelaySeconds = 1.5f;   ///< NodeDelay / FractalEcho, per node.

    inline constexpr std::size_t kNumReverbLines = 8;
    inline constexpr float kMaxReverbLineSeconds = 0.25f;      ///< per delay line, before `reverbSizeParam` scaling.
    inline constexpr float kMaxReverbPreDelaySeconds = 0.2f;
    inline constexpr std::size_t kNumReverbDiffuserStages = 4;  ///< series input allpass chain, see effects::Reverb.
    inline constexpr float kMaxReverbDiffuserStageSeconds = 0.02f;
    inline constexpr std::size_t kNumReverbEarlyTaps = 8;       ///< discrete early-reflection cluster, see effects::Reverb.
    inline constexpr float kMaxReverbEarlyLineSeconds = 0.15f;
    inline constexpr float kMaxReverbModDepthMs = 4.0f;         ///< late-tank per-line delay-length modulation, at reverbModDepth=1.
    inline constexpr float kMaxLimiterLookaheadSeconds = 0.02f; ///< 20ms cap -- see effects::Limiter.

    /// One node in a NodeDelay tree. `parentIndex < static_cast<int>(ownIndex)` is
    /// required (validated/clamped at load, same "always route from a lower index"
    /// discipline the algorithm graph compiler and fuzz tool already rely on to
    /// guarantee acyclic-by-construction structure without a runtime cycle check).
    /// `parentIndex == -1` means "fed directly from the slot's input," i.e. a new
    /// root tap rather than a child of another node.
    struct DelayNodeParams
    {
        bool enabled = true;
        int parentIndex = -1;
        float delayMs = 250.0f;
        float feedback = 0.35f;   ///< 0..0.95, this node's own repeat decay.
        float pan = 0.0f;         ///< -1..1
        float distortion = 0.0f;  ///< 0..1, soft-clip drive on this node's output.
        float level = 0.7f;       ///< this node's contribution to the wet mix.
    };

    struct EffectSlotParams
    {
        EffectType type = EffectType::Bypass;
        float mix = 1.0f; ///< 0 (dry only) .. 1 (full wet), shared by every non-Bypass type.

        // -- Saturation --
        float saturationDriveDb = 6.0f;

        // -- Chorus --
        float chorusRateHz = 0.5f;
        float chorusDepthMs = 4.0f;
        float chorusBaseDelayMs = 12.0f;

        // -- TapeDelay (docs/FX_BANK.md "Cocoa Delay") --
        float tapeDelayMs = 350.0f;
        float tapeFeedback = 0.4f;
        float tapeDriveDb = 3.0f;
        float tapeDuckAmount = 0.0f;      ///< 0 (no ducking) .. 1 (full duck under the dry signal).
        float tapeDriftDepthMs = 1.5f;    ///< wow/flutter depth.
        float tapeDriftRateHz = 0.3f;
        DelayPanMode tapePanMode = DelayPanMode::Static;

        // -- NodeDelay (docs/FX_BANK.md "ChowMatrix") --
        std::array<DelayNodeParams, kMaxDelayNodes> nodes{};
        float nodeInsanity = 0.0f; ///< 0..1, per-node deterministic random delay-time wander.

        // -- FreqShiftEcho (docs/FX_BANK.md "ValhallaFreqEcho") --
        float freqShiftHz = 7.0f;         ///< can be negative (downward-shifting spiral).
        float freqShiftDelayMs = 280.0f;
        float freqShiftFeedback = 0.55f;
        float freqShiftLowCutHz = 120.0f;
        float freqShiftHighCutHz = 8000.0f;

        // -- FractalEcho (docs/FX_BANK.md "The invented algorithm") --
        std::uint64_t fractalSeedA = 1;
        std::uint64_t fractalSeedB = 2;
        float fractalMorph = 0.0f;      ///< 0 = topology A .. 1 = topology B, continuous.
        float fractalBaseDelayMs = 180.0f;
        float fractalRatio = 0.62f;     ///< self-similar time-scaling ratio between tree depths.
        float fractalSpreadMs = 15.0f;  ///< per-node deterministic jitter added to the fractal time.

        // -- Reverb (see docs/FX_BANK.md "GATE 11" for the full research writeup).
        // An 8-line Jot/Householder-matrix FDN late tank (informed by CloudSeed's
        // "massive, modulated ambient spaces from many delay lines" approach) fed
        // through a Schroeder/Dattorro-style series-allpass input diffuser, plus a
        // parallel discrete early-reflection tap cluster mixed independently against
        // the late tank -- and, the single most distinctive researched principle,
        // *three-band* frequency-dependent decay (independent HF/LF RT60 multipliers
        // relative to a mid-band `reverbDecaySeconds`, each with its own crossover),
        // informed by (not ported from) the parameter architecture documented in the
        // Bricasti M7 owner's manual (Rev 5.02.08): "Reverb Time" is explicitly
        // defined there as *mid-frequency* reverb time, with independent "HF RT
        // MPY"/"HF Crossover" and "LF RT MPY"/"LF Crossover" controls -- exactly
        // `reverbHighRatio`/`reverbHighCrossoverHz`/`reverbLowRatio`/
        // `reverbLowCrossoverHz` below.
        float reverbSizeParam = 1.0f;      ///< scales delay-line lengths; ~0.3 (small room) .. ~2.0 (large hall). Decoupled from decay time, matching the M7's separate "Reverb Size" control.
        float reverbDecaySeconds = 2.0f;   ///< mid-band RT60 (the M7's "Reverb Time" is explicitly mid-frequency).
        float reverbPreDelayMs = 20.0f;
        float reverbHighRatio = 0.6f;      ///< 0.2..1.0, HF RT60 as a multiplier of reverbDecaySeconds (M7 "HF RT MPY").
        float reverbHighCrossoverHz = 4500.0f; ///< M7 "HF Crossover".
        float reverbLowRatio = 1.3f;       ///< 0.2..4.0, LF RT60 multiplier -- can exceed 1 (M7 "LF RT MPY": bass can ring *longer* than mids, not just shorter).
        float reverbLowCrossoverHz = 400.0f;   ///< M7 "LF Crossover".
        float reverbDiffusion = 0.65f;     ///< 0..1, input-diffuser allpass coefficient strength (instantaneous smoothness).
        float reverbDensity = 0.85f;       ///< 0..1, how many diffuser stages are engaged (echo density build-up over time -- M7's distinct "Density" control).
        float reverbModDepth = 0.35f;      ///< 0..1, late-tank per-line delay modulation depth (M7 "Reverb Modulation": pitch variation that keeps a dense network from ringing metallically).
        float reverbModRateHz = 0.4f;      ///< 0.05..2.0, per-line-decorrelated modulation rate.
        float reverbEarlyLevel = 0.5f;     ///< 0..1, early-reflection cluster level (M7 "Early/Reverb Mix", early half).
        float reverbLateLevel = 1.0f;      ///< 0..1, late-tank level (M7 "Early/Reverb Mix", reverb half).
        float reverbRollOffHz = 12000.0f;  ///< 80..20000, final output lowpass (M7 "Roll Off").
        float reverbVlfCutDb = 0.0f;       ///< -18..0 dB, low-shelf cut of very-low-frequency wet content (M7 "VLF Cut").

        // -- Eq (3-band: low shelf, mid peak, high shelf; RBJ Audio EQ Cookbook biquads) --
        float eqLowFreqHz = 200.0f;
        float eqLowGainDb = 0.0f;
        float eqMidFreqHz = 1000.0f;
        float eqMidGainDb = 0.0f;
        float eqMidQ = 0.8f;
        float eqHighFreqHz = 6000.0f;
        float eqHighGainDb = 0.0f;

        // -- Compressor (feedforward, peak detector, soft knee) --
        float compThresholdDb = -18.0f;
        float compRatio = 3.0f;
        float compAttackMs = 8.0f;
        float compReleaseMs = 120.0f;
        float compKneeDb = 6.0f;
        float compMakeupDb = 0.0f;
        // Master-compressor output transformer (post-GR colour); 0 = bypass.
        float compTransformerCore = 0.0f;   // 0 off, 1 nickel, 2 iron, 3 steel
        float compTransformerBrand = 0.0f;  // 0 neutral, 1 jensen, 2 cinemag, 3 sowter
        float compTransformerAmount = 1.0f; // 0..1 blend of transformer colour

        // -- Limiter (lookahead, sliding-window-minimum gain -- true no-overshoot) --
        float limiterCeilingDb = -0.3f;
        float limiterLookaheadMs = 5.0f;
        float limiterReleaseMs = 60.0f;
    };

} // namespace pw8::effects
