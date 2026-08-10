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
