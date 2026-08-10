#pragma once

#include <array>
#include <cstdint>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/core/AudioBlock.hpp"
#include "pw8/oscillator/WavetableOscillator.hpp"
#include "pw8/patch/Patch.hpp"
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

        /// Renders `output.numFrames()` samples into `output`, accumulating from all
        /// active voices. Audio-thread safe.
        void process(core::StereoBlockView output) noexcept;

        [[nodiscard]] double getSampleRate() const noexcept { return sampleRate_; }
        [[nodiscard]] const algorithm::CompiledAlgorithm& getCompiledAlgorithm() const noexcept { return compiledLayerA_; }
        [[nodiscard]] algorithm::CompileStatus getLastCompileStatus() const noexcept { return lastCompileStatus_; }

    private:
        [[nodiscard]] static op::OperatorParams toOperatorParams(const patch::OperatorPatch& p) noexcept;

        patch::Patch patch_{};
        algorithm::CompiledAlgorithm compiledLayerA_{};
        algorithm::CompileStatus lastCompileStatus_ = algorithm::CompileStatus::Ok;

        std::array<op::OperatorParams, core::kNodesPerLayer> operatorParamsTemplateA_{};
        std::array<oscillator::WavetableView, core::kNodesPerLayer> wavetablesA_{};

        voice::VoicePool voices_{};
        voice::VoiceAllocator allocator_{};
        tuning::TuningService tuning_{};

        static constexpr float kPitchBendRangeSemitones = 2.0f;

        double sampleRate_ = 48000.0;
        std::uint64_t noteGenerationCounter_ = 0;
    };

} // namespace pw8::render
