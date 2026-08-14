#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/core/Types.hpp"
#include "pw8/core/Version.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/envelope/DahdsrEnvelope.hpp"
#include "pw8/filter/CharacterFilter.hpp"
#include "pw8/filter/StateVariableFilter.hpp"
#include "pw8/lfo/Lfo.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"
#include "pw8/operator/OperatorNode.hpp"
#include "pw8/sequencer/ArpeggiatorTypes.hpp"

// Pure data model for the native `.pw8` patch format (schemaVersion 1).
// Deliberately free of any JSON/serialization dependency -- see
// pw8/patch/PatchSerializer.hpp for the (de)serialization boundary, which is the
// only place nlohmann::json is allowed to appear. This header is safe to include
// from anywhere, including realtime-adjacent code, without dragging in a JSON parser.
//
// Control-path only: nothing in this header is touched by the audio thread directly.
// A Patch is compiled into voice-ready operator params + a CompiledAlgorithm by
// pw8::render::Engine::loadPatch() (background/UI thread), and only that compiled
// result crosses to the audio thread.

namespace pw8::patch
{
    /// How Layer A and Layer B combine. Only SINGLE_A is exercised by the render path
    /// in this pass -- see docs/ROADMAP.md Phase 8 (dual layer mixing) and Phase 9
    /// (algorithm/layer morph). The rest of the enum exists now so patch data authored
    /// today keeps meaning as those phases land (no retrofitted schema break).
    enum class LayerMode : std::uint8_t
    {
        SingleA = 0,
        SingleB,
        Stack,
        Split,
        VelocitySplit,
        Morph,
        KeyZone,
    };

    enum class UnisonMode : std::uint8_t
    {
        Off = 0,
        Full,
        Operator,
        Stereo,
        Hyper,
        Harmonic,
    };

    struct UnisonSettings
    {
        UnisonMode mode = UnisonMode::Off;
        int voices = 1;          ///< 1..kMaxUnisonVoices
        float detuneCents = 0.0f;
        float spread = 0.0f;     ///< stereo spread, 0..1
        float phaseRandom = 0.0f;
        float blend = 1.0f;
    };

    /// Persisted per-node operator configuration (maps to op::OperatorParams at compile time).
    struct OperatorPatch
    {
        algorithm::EngineType engine = algorithm::EngineType::Classic;
        oscillator::ClassicWaveform classicWaveform = oscillator::ClassicWaveform::Saw;
        float classicMorph = -1.0f;
        float pulseWidth = 0.5f;
        float wavetableFramePosition = 0.0f;
        std::string wavetableId; ///< empty == none loaded; resolved by content pipeline, not the core.
        float frequencyRatio = 1.0f;
        float fixedFrequencyHz = 440.0f;
        bool keyTrack = true;
        float level = 1.0f;
        float pan = 0.0f; ///< reserved for per-operator stereo placement (PLANNED).

        // Engine Type 3 (FM/PM) only -- see op::OperatorParams's matching fields for
        // the full doc comment on what these mean.
        float fmModulatorRatio = 1.0f;
        float fmModulatorIndex = 0.5f;
        float fmModulatorFeedback = 0.0f;
        oscillator::ClassicWaveform fmModulatorWaveform = oscillator::ClassicWaveform::Sine;

        /// NoiseChaos engine fields (only meaningful when engine == NoiseChaos).
        /// noiseVariant is noise::NoiseVariant's ordinal (0-6), stored as a float to
        /// match every other discrete field's flat-struct convention.
        float noiseVariant = 0.0f; // noise::NoiseVariant::White
        float noiseRate = 200.0f; // Hz, retarget rate for S&H/SmoothRandom/Dust variants.

        /// PhaseShape engine fields (only meaningful when engine == PhaseShape).
        /// See oscillator::PhaseShapeParams for the full per-field writeup.
        float phaseBend = 0.0f;
        float phaseFold = 0.0f;
        float phaseAsymmetry = 0.0f;
        float phaseShape = 0.0f;

        /// Additive engine fields (only meaningful when engine == Additive).
        /// See oscillator::AdditiveParams for the full per-field writeup.
        float additivePartialCount = 32.0f;
        float additiveTilt = 0.0f;
        float additiveOddEven = 0.5f;
        float additiveStretch = 0.0f;

        /// Resonator engine fields (only meaningful when engine == Resonator).
        /// See oscillator::ResonatorParams for the full per-field writeup.
        float resonatorStructure = 0.3f;
        float resonatorDecay = 0.5f;
        float resonatorDamping = 0.5f;
        float resonatorBrightness = 0.5f;
        float resonatorModeCount = 6.0f;

        /// Granular engine fields (only meaningful when engine == Granular).
        /// Deliberately reuses wavetableId/wavetableFramePosition/level above
        /// rather than duplicating them -- see oscillator::GranularParams for the
        /// full per-field writeup.
        float grainDensity = 20.0f;
        float grainSizeMs = 60.0f;
        float grainPositionJitter = 0.1f;
        float grainPitchJitter = 0.0f;

        /// Wavetable engine warp fields (only meaningful when engine == Wavetable).
        /// See docs/DESIGN_AND_WARPS_PLAN.md §3.3 and oscillator::WtWarpParams.
        float wtBend = 0.0f;
        float wtAsymmetry = 0.0f;
        float wtSyncRatio = 1.0f;
        float wtSyncAmount = 0.0f;
        float wtFormantShift = 0.0f;

        /// Optional per-engine multimode SVF on this operator's own generated output
        /// (before routed graph audio is mixed in). Off by default.
        filter::FilterParams filter1{};
    };

    struct LayerPatch
    {
        std::array<OperatorPatch, core::kNodesPerLayer> operators{};
        algorithm::AlgorithmGraphDefinition algorithm = algorithm::AlgorithmGraphDefinition::makeDefaultParallel8();

        /// 8 envelopes. `envelopes[0]` is conventionally "the" amp envelope -- the
        /// only one wired to the VCA and to voice lifetime, see voice::Voice::isFree()
        /// -- the rest are fully general-purpose mod matrix sources (Env1..Env8, see
        /// docs/MODULATION.md). Schema v1's singular `ampEnvelope` field migrates to
        /// `envelopes[0]` on load -- see PatchSerializer's v1->v2 migration.
        std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer> envelopes{};
        UnisonSettings unison{};

        /// Global filter: clean multimode SVF, applied per-voice to the full algorithm
        /// graph output (after all engine filters), before the amplitude envelope.
        /// Optional -- disabled by default on new operators/patches. See docs/DSP_ENGINE.md.
        filter::FilterParams filter1{};

        /// Global character filter (nonlinear ladder + drive), serial after Filter 1.
        filter::CharacterFilterParams filter2{};

        /// 8 LFOs, usable as mod matrix sources (Lfo1..Lfo8, see docs/MODULATION.md).
        /// `lfos[0]` is what used to be the singular `lfo1` field (schema v1 migrates
        /// on load, same as `envelopes` above). VOICE-scoped routes give each voice
        /// its own independently-phased instance of a given LFO index; LAYER/GLOBAL-
        /// scoped routes instead read one shared tick per index, computed once per
        /// sample by render::Engine and identical across every voice in the layer --
        /// see ModScope's doc comment in ModMatrixTypes.hpp.
        std::array<lfo::LfoParams, core::kNumLfosPerLayer> lfos{};

        /// Fixed-capacity mod matrix routes (VOICE scope executed in this pass).
        core::FixedVector<modulation::ModRoute, core::kMaxModRoutes> modRoutes;

        float gain = 1.0f;
        float pan = 0.0f;
        float width = 1.0f; ///< stereo width, 0 (mono) .. 1 (full) .. 2 (wide), reserved (PLANNED).
        float centerGravity = 0.5f; ///< see docs/DSP_ENGINE.md "Center Gravity" (PLANNED wiring).

        /// 3 layer insert FX slots, applied in order to this layer's summed voice
        /// output before it reaches the master bus. See docs/FX_BANK.md.
        std::array<effects::EffectSlotParams, effects::kNumLayerInsertSlots> insertEffects{};
    };

    /// AI-generation lock flags: control what Generate/Mutate/Breed are allowed to touch.
    /// These are metadata for the (future, Patchwork-side) AI pipeline, not DSP parameters.
    struct LockFlags
    {
        bool lockSources = false;
        bool lockAlgorithm = false;
        bool lockFilters = false;
        bool lockModulation = false;
        bool lockEffects = false;
        bool lockSequence = false;
    };

    struct PatchMetadata
    {
        std::string id;
        std::string name = "Init";
        std::string author;
        std::string description;
        std::string category; ///< bass, lead, pluck, keyboard, pad, arp, drone, chord, fx, brass, vocal texture, strings, sequence
        std::vector<std::string> moods;
        std::vector<std::string> genres;
        std::vector<std::string> tags;
        std::string createdAt; ///< ISO-8601
        std::string engineVersion = std::string(core::EngineVersion::string());
        int schemaVersion = core::kPatchSchemaVersion;
        std::uint64_t seed = 0;
        std::vector<std::string> lineage; ///< parent patch IDs, for breeding provenance.
    };

    struct GlobalVoiceSettings
    {
        std::size_t polyphony = core::kDefaultVoices;
        float masterGain = 1.0f;
        float a4Hz = 440.0f; ///< tuning reference; full tuning service is PLANNED (docs/ROADMAP.md).
        float portamentoSeconds = 0.0f; ///< 0 = off; glide pitch on legato note changes.
        /// PoliMATHS Modulation Dissemination (MVP): when true, Macro1–3 values are sampled
        /// per voice at note-on with variation; held voices ignore live macro sweeps.
        bool macroDissemination = false;
        /// Per-voice spread applied to Macro1–3 at note-on when `macroDissemination` is true
        /// (0 = no spread, 0.25 ≈ ±25% around the current macro value).
        float disseminationDepth = 0.22f;
    };

    struct Macro
    {
        std::string id;
        std::string name;
        std::string description;
        float value = 0.0f; ///< 0..1
        // Macro routing (destinations) lands with the mod matrix -- Phase 5. A macro
        // with no routes is valid and simply does nothing yet.
    };

    enum class UiFocusKnobKind
    {
        Macro,
        Param,
        Morph, ///< Frames-style morph KOIN — see docs/MORPH_KOIN_SPEC.md (Horizon 3 executor)
    };

    /// One feature KOIN in PLAY mode: a macro knob wired via modRoutes (see docs/PATCH_FORMAT.md uiFocus).
    struct UiFocusKnob
    {
        UiFocusKnobKind kind = UiFocusKnobKind::Macro;
        std::size_t macroIndex = 0; ///< 0..7 when kind == Macro (Macro1–3 typical for KOINS)
        std::string paramId;          ///< legacy APVTS id when kind == Param (not shown in Basic/Compact)
        std::string label;            ///< optional display override
    };

    /// One stored snapshot on the morph timeline (see docs/MORPH_KOIN_SPEC.md).
    struct MorphKoinKeyframe
    {
        std::string name;
        float position = 0.0f; ///< 0..1 timeline slot; evenly spaced when unset in JSON
        std::array<float, 8> macroValues{}; ///< optional per-keyframe macro levels
        bool hasMacroValues = false;
        /// Horizon 3: dotted APVTS / patch paths → float. Parsed in serializer; applied by morph executor.
        std::unordered_map<std::string, float> paramOverrides;
    };

    /// Frames-inspired morph between 2–4 keyframes. Metadata round-trips in Horizon 2; executor TODO Horizon 3.
    struct MorphKoin
    {
        std::string label;
        std::string description;
        float defaultPosition = 0.0f;
        float position = 0.0f;
        std::string curve = "linear"; ///< linear | smooth | step
        bool wrap = false;
        std::vector<MorphKoinKeyframe> keyframes;
    };

    /// Patch-authored feature macro KOINS (1–3 in Basic/Compact PLAY). When `knobs` is non-empty,
    /// PLAY mode uses macro entries from this list instead of inferring from mod routes.
    struct PatchUiFocus
    {
        std::size_t maxKnobs = 3;
        std::vector<UiFocusKnob> knobs;
    };

    struct Patch
    {
        int schemaVersion = core::kPatchSchemaVersion;
        PatchMetadata metadata{};
        LayerMode layerMode = LayerMode::SingleA;
        float layerMorph = 0.0f; ///< 0 = A, 1 = B (PLANNED, see LayerMode).
        LayerPatch layerA{};
        LayerPatch layerB{};
        GlobalVoiceSettings voiceSettings{};
        LockFlags locks{};
        std::array<Macro, 8> macros{};
        MorphKoin morphKoin{}; ///< optional; empty keyframes = no morph KOIN
        PatchUiFocus uiFocus{};
        /// Performance-wide (not per-layer): intercepts noteOn/noteOff before they
        /// reach voices when enabled. See docs/ARPEGGIATOR.md.
        sequencer::ArpeggiatorParams arpeggiator{};
        /// 4 master FX slots, applied in order to the final mixed stereo bus (after
        /// all layers' insert effects). See docs/FX_BANK.md.
        std::array<effects::EffectSlotParams, effects::kNumMasterSlots> masterEffects{};
        std::uint64_t seed = 0;

        [[nodiscard]] static Patch makeInit() noexcept
        {
            Patch p;
            p.metadata.name = "Init";
            p.metadata.category = "lead";
            return p;
        }
    };

} // namespace pw8::patch
