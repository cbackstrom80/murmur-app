#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/core/AudioBlock.hpp"
#include "pw8/effects/EffectChain.hpp"
#include "pw8/oscillator/WavetableTable.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/sequencer/Arpeggiator.hpp"
#include "pw8/tuning/TuningService.hpp"
#include "pw8/voice/VoiceAllocator.hpp"

// pw8::render::Engine is the top-level facade: load a Patch, feed it MIDI, pull
// rendered audio. This is what both pw8_render (native CLI) and, eventually,
// pw8_plugin wrap.
//
// Threading contract (docs/ARCHITECTURE.md "Threading Model"):
//   loadPatch()  -- control-path only (UI/background thread). Compiles the algorithm
//                   graph and copies patch data into voice-ready params. Never call
//                   from the audio thread.
//   noteOn/noteOff/pitchBend/controlChange/allNotesOff/process()
//               -- audio-thread safe: no allocation, no locking, no exceptions.
//
// This single-Engine-instance design (no atomic state swap) is appropriate for the
// native offline renderer, where there is exactly one thread driving the whole
// pipeline. The JUCE plugin wrapper (Phase 16) will add a prepare-then-atomic-swap
// layer on top before patch changes can happen concurrently with audio callbacks --
// see docs/PLUGIN_ARCHITECTURE.md.
namespace pw8::render
{
    class Engine
    {
    public:
        void prepare(double sampleRate) noexcept;

        /// Compiles and loads a patch. Returns false if the algorithm graph failed to
        /// compile (in which case the engine falls back to a known-safe default graph
        /// and keeps running rather than leaving the audio thread with nothing valid).
        bool loadPatch(const patch::Patch& patchToLoad) noexcept;

        void noteOn(int note, int channel, int velocity7) noexcept;
        void noteOff(int note, int channel, int velocity7) noexcept;
        void pitchBend(int channel, int value14) noexcept;
        void controlChange(int channel, int controller, int value7) noexcept;
        void channelPressure(int channel, int value7) noexcept;
        void polyAftertouch(int channel, int note, int value7) noexcept;
        void allNotesOff() noexcept;

        /// Sets the tempo used by tempo-synced LFOs. Audio-thread safe (a plain float
        /// store) -- call once per block from whatever knows the host/render tempo.
        void setTempo(float bpm) noexcept { bpm_ = bpm > 0.0f ? bpm : 120.0f; }

        /// Live-updates macro `index` (0..7) so it takes effect immediately on every
        /// currently-sustaining voice, not just the next note-on -- what a DAW
        /// automating a macro parameter mid-hold needs. Audio-thread safe: writes a
        /// plain float into `patch_.macros[index].value` (never touches the Macro's
        /// string fields) and into every voice's `macroValues[index]` (a plain
        /// per-voice array already read live, per-sample, by the mod matrix -- see
        /// `Voice::renderSample`). No allocation, no locking. Out-of-range `index` is
        /// a no-op. See docs/PLUGIN_ARCHITECTURE.md "Automation".
        void setMacroValue(std::size_t index, float value) noexcept;

        [[nodiscard]] float getMacroValue(std::size_t index) const noexcept
        {
            return index < patch_.macros.size() ? patch_.macros[index].value : 0.0f;
        }

        // --- Live parameter API (docs/PLUGIN_ARCHITECTURE.md "Automation") ---
        //
        // Every setter below is audio-thread safe: each writes only into the POD
        // (string-free) substructs of `patch_` -- so a host/plugin can call these once
        // per block without ever risking a std::string allocation on the audio thread
        // -- and, where the corresponding field is read live per-sample by a currently
        // *sustaining* voice (filter/LFO/operators/gain/pan; see Voice::renderSample),
        // pushes the new value into every active voice too, exactly like
        // setMacroValue() above. Fields NOT covered here (per-operator wavetableId,
        // algorithm graph topology, arpeggiator per-step array, effect delay-tree
        // node arrays/seeds, AI lock flags, patch metadata) are structural or
        // string-typed data, not continuous performance parameters -- see
        // docs/PLUGIN_ARCHITECTURE.md "Automation" for the full scope rationale.
        // The mod-route list gets its own setModRoutesLive() below rather than a flat
        // per-field APVTS parameter: adding/removing/retargeting a route is a
        // discrete structural edit (docs/UI.md "drag-to-modulate"), not a continuous
        // knob value -- still not part of the automation surface, but no longer
        // frozen to patch-load-only either.
        // Layer B is not voiced yet (Phase 8), so only Layer A has a live API.

        void setFilterLive(const filter::FilterParams& params) noexcept;
        [[nodiscard]] const filter::FilterParams& getFilterParams() const noexcept { return patch_.layerA.filter1; }

        /// `lfoIndex` in [0, kNumLfosPerLayer). Out-of-range is a no-op. Updates both
        /// the per-voice template/active-voice copies AND (since `Engine::process()`
        /// reads `patch_.layerA.lfos[lfoIndex]` fresh every sample for the shared
        /// layer-scope tick) the LAYER/GLOBAL-scoped shared LFO -- one call covers
        /// both scopes, see ModScope's doc comment in ModMatrixTypes.hpp.
        void setLfoLive(std::size_t lfoIndex, const lfo::LfoParams& params) noexcept;
        [[nodiscard]] lfo::LfoParams getLfoParams(std::size_t lfoIndex) const noexcept
        {
            return lfoIndex < core::kNumLfosPerLayer ? patch_.layerA.lfos[lfoIndex] : lfo::LfoParams{};
        }

        /// `opIndex` in [0, kNodesPerLayer). Out-of-range is a no-op.
        void setOperatorLive(std::size_t opIndex, const op::OperatorParams& params) noexcept;
        [[nodiscard]] op::OperatorParams getOperatorParams(std::size_t opIndex) const noexcept
        {
            return opIndex < core::kNodesPerLayer ? operatorParamsTemplateA_[opIndex] : op::OperatorParams{};
        }

        /// `envIndex` in [0, kNumEnvelopesPerLayer). Out-of-range is a no-op. Takes
        /// effect on the NEXT note-on (triggerNoteOnDirect() reads
        /// `patch_.layerA.envelopes` fresh every trigger) -- an already-ringing
        /// voice's envelope keeps the shape it was triggered with, since DahdsrEnvelope
        /// captures its targets/coefficients once at noteOn() and has no live
        /// mid-ramp-retarget API. Automating envelope times still works exactly as a
        /// performer would expect between notes; it just isn't a glitch-free
        /// mid-attack edit.
        void setEnvelopeLive(std::size_t envIndex, const envelope::DahdsrParams& params) noexcept;
        [[nodiscard]] envelope::DahdsrParams getEnvelopeParams(std::size_t envIndex) const noexcept
        {
            return envIndex < core::kNumEnvelopesPerLayer ? patch_.layerA.envelopes[envIndex] : envelope::DahdsrParams{};
        }

        /// Replaces the entire Layer A mod-route list. Unlike every setter above,
        /// there is no per-voice copy to also update -- `Voice::renderSample` already
        /// reads `patch_.layerA.modRoutes` fresh every sample (the `liveModRoutes`
        /// parameter), so this takes effect on already-sustaining voices immediately,
        /// not just the next note-on. Message-thread-safe the same way every other
        /// live setter is (a plain POD write racing an audio-thread read of the same
        /// struct, tolerated the same way the rest of this API already does).
        void setModRoutesLive(const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes) noexcept;
        [[nodiscard]] const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& getModRoutes() const noexcept
        {
            return patch_.layerA.modRoutes;
        }

        void setLayerGainLive(float gain) noexcept;
        [[nodiscard]] float getLayerGain() const noexcept { return patch_.layerA.gain; }

        /// Read-only access to node `opIndex`'s currently-loaded wavetable content
        /// (nullptr if that operator isn't Wavetable-engine, has no `wavetableId` set,
        /// or the referenced file failed to load -- see loadPatch()'s wavetable-loading
        /// comment). For UI display only (e.g. a frame-stack preview) -- never touched
        /// by the audio thread through this accessor; `Voice`/`WavetableOscillator`
        /// read `wavetableTablesA_` directly on the audio thread via their own path.
        /// Same message-thread-safety tolerance as every other getter above: a plain
        /// pointer read that can race a concurrent loadPatch() write, accepted the same
        /// way the rest of this API already accepts it.
        [[nodiscard]] const oscillator::WavetableTable* getWavetableTable(std::size_t opIndex) const noexcept
        {
            return opIndex < core::kNodesPerLayer ? wavetableTablesA_[opIndex] : nullptr;
        }

        void setLayerPanLive(float pan) noexcept;
        [[nodiscard]] float getLayerPan() const noexcept { return patch_.layerA.pan; }

        void setMasterGainLive(float masterGain) noexcept;
        [[nodiscard]] float getMasterGainValue() const noexcept { return patch_.voiceSettings.masterGain; }

        /// `slot` in [0, kNumLayerInsertSlots)/[0, kNumMasterSlots). Read-modify-write
        /// against the getter first if you need to preserve fields this API doesn't
        /// expose to automation (NodeDelay/FractalEcho's `nodes[]`, FractalEcho's
        /// seeds) -- `Engine::process()` reads these structs live every sample
        /// (`layerAInsertChain_.process(patch_.layerA.insertEffects, ...)`), so a
        /// write here takes effect on the very next sample with no extra plumbing.
        void setInsertEffectLive(std::size_t slot, const effects::EffectSlotParams& params) noexcept;
        [[nodiscard]] effects::EffectSlotParams getInsertEffectParams(std::size_t slot) const noexcept
        {
            return slot < patch_.layerA.insertEffects.size() ? patch_.layerA.insertEffects[slot]
                                                               : effects::EffectSlotParams{};
        }

        void setMasterEffectLive(std::size_t slot, const effects::EffectSlotParams& params) noexcept;
        [[nodiscard]] effects::EffectSlotParams getMasterEffectParams(std::size_t slot) const noexcept
        {
            return slot < patch_.masterEffects.size() ? patch_.masterEffects[slot] : effects::EffectSlotParams{};
        }

        /// Only the scalar top-level fields of `params` (enabled/mode/rateMode/rateHz/
        /// syncDivisionIndex/octaveRange/numSteps/latch) are applied -- `params.steps`
        /// is ignored, the currently-loaded per-step pattern is always preserved (see
        /// getArpeggiatorParams() if you need to read it back first). Uses
        /// Arpeggiator::setLiveParams(), NOT configure() -- held notes, pattern
        /// position, and pending ratchet/tie events are never reset by this call, so
        /// automating the rate mid-pattern doesn't restart it from step 0.
        void setArpeggiatorScalarLive(const sequencer::ArpeggiatorParams& params) noexcept;
        [[nodiscard]] const sequencer::ArpeggiatorParams& getArpeggiatorParams() const noexcept { return patch_.arpeggiator; }

        /// Renders `output.numFrames()` samples into `output`, accumulating from all
        /// active voices. Audio-thread safe.
        void process(core::StereoBlockView output) noexcept;

        [[nodiscard]] double getSampleRate() const noexcept { return sampleRate_; }
        [[nodiscard]] const algorithm::CompiledAlgorithm& getCompiledAlgorithm() const noexcept { return compiledLayerA_; }
        [[nodiscard]] algorithm::CompileStatus getLastCompileStatus() const noexcept { return lastCompileStatus_; }

    private:
        [[nodiscard]] static op::OperatorParams toOperatorParams(const patch::OperatorPatch& p) noexcept;

        /// Shared by the public noteOn/noteOff and the arpeggiator's internally
        /// generated events -- the arpeggiator is indistinguishable from a real
        /// performer as far as voice allocation/envelopes are concerned. Bypasses the
        /// `patch_.arpeggiator.enabled` redirect that the public methods apply, so
        /// calling this from the arp tick can never recurse back into the arp.
        void triggerNoteOnDirect(int note, int channel, int velocity7) noexcept;
        void triggerNoteOffDirect(int note, int channel, int velocity7) noexcept;

        patch::Patch patch_{};
        algorithm::CompiledAlgorithm compiledLayerA_{};
        algorithm::CompileStatus lastCompileStatus_ = algorithm::CompileStatus::Ok;

        std::array<op::OperatorParams, core::kNodesPerLayer> operatorParamsTemplateA_{};

        /// Owns loaded wavetable content (control-path-populated in loadPatch(), which
        /// treats `OperatorPatch::wavetableId` as a filesystem path -- see
        /// docs/PATCH_FORMAT.md "Wavetable Resource Resolution" for the current, PARTIAL
        /// resolution scheme). `wavetableTablesA_` holds read-only pointers into this
        /// storage for the audio thread; rebuilt only in loadPatch(), never mutated
        /// concurrently with process().
        std::array<std::optional<oscillator::WavetableTable>, core::kNodesPerLayer> wavetableStorageA_{};
        std::array<const oscillator::WavetableTable*, core::kNodesPerLayer> wavetableTablesA_{};

        voice::VoicePool voices_{};
        voice::VoiceAllocator allocator_{};
        tuning::TuningService tuning_{};
        sequencer::Arpeggiator arpeggiator_{};

        /// 8 shared, layer-wide LFOs, ticked once per sample in process() (before the
        /// voice loop) rather than once per voice -- what LAYER/GLOBAL-scoped LFO mod
        /// routes read (see ModScope's doc comment in ModMatrixTypes.hpp). Each is
        /// initialized once (loadPatch()) rather than per-note, since there's no
        /// single coherent "note-on" for a layer-wide modulator; Retrigger/OneShot
        /// LFO modes have no distinct trigger event at this scope and behave the same
        /// as Free (documented, not silently wrong).
        std::array<lfo::Lfo, core::kNumLfosPerLayer> layerLfosA_{};

        /// Layer A's 3 insert FX slots (applied to the summed voice output) and the
        /// engine-wide 4 master FX slots (applied to the final mixed bus). See
        /// docs/FX_BANK.md. Layer B has no voiced signal yet (Phase 8), so it has no
        /// insert chain of its own in this pass.
        effects::LayerInsertChain layerAInsertChain_{};
        effects::MasterChain masterChain_{};

        static constexpr float kPitchBendRangeSemitones = 2.0f;

        double sampleRate_ = 48000.0;
        float bpm_ = 120.0f;
        std::uint64_t noteGenerationCounter_ = 0;
    };

} // namespace pw8::render
