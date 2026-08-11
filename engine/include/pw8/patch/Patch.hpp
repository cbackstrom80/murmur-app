#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/core/Types.hpp"
#include "pw8/core/Version.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/envelope/DahdsrEnvelope.hpp"
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

        /// Granular engine fields (only meaningful when engine == Granular).
        /// Deliberately reuses wavetableId/wavetableFramePosition/level above
        /// rather than duplicating them -- see oscillator::GranularParams for the
        /// full per-field writeup.
        float grainDensity = 20.0f;
        float grainSizeMs = 60.0f;
        float grainPositionJitter = 0.1f;
        float grainPitchJitter = 0.0f;
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

        /// Filter 1: clean multimode SVF, applied per-voice between the algorithm
        /// graph's output and the amplitude envelope. See docs/DSP_ENGINE.md.
        filter::FilterParams filter1{};

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

        // Filter 2 (character) is architected as of docs/DSP_ENGINE.md but not yet
        // part of the signal path in this pass -- see docs/ROADMAP.md Phase 6.
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
