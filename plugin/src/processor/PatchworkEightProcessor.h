#pragma once

// STATUS: PARTIAL, build-verified against real JUCE 8.0.6 (AU passes `auval` in
// full; both AU and VST3 pass `pluginval` at max strictness) -- see
// plugin/CMakeLists.txt and docs/PLUGIN_ARCHITECTURE.md.
//
// Threading contract (docs/PLUGIN_ARCHITECTURE.md "Host State"):
//   - prepareToPlay() / releaseResources() / setStateInformation() run on the message
//     thread. They may load patches, compile algorithm graphs, and allocate.
//   - processBlock() runs on the audio thread. It must only ever read the currently
//     "live" pw8::render::Engine and never touch a Patch, JSON, or the filesystem.
//   - A patch change prepares a fresh pw8::render::Engine off the audio thread, then
//     publishes it via a single atomic pointer swap (std::atomic<Engine*>) that
///    processBlock() reads once per block. This is the "prepare -> atomic swap"
//     pattern described in docs/ARCHITECTURE.md "Threading Model" -- deliberately NOT
//     a hand-rolled lock-free queue.

#include <array>
#include <atomic>
#include <functional>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "pw8/patch/Patch.hpp"
#include "pw8/dsp/SidechainFollower.hpp"
#include "pw8/lfo/Lfo.hpp"
#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/render/Engine.hpp"
#include "processor/ScopeAudioTap.h"
#include "state/ParamChangeQueue.hpp"
#include "state/PluginState.h"
#include "ui/ScopeViewMode.h"

namespace pw8::plugin
{
    struct GraphEditResult
    {
        bool ok = false;
        algorithm::CompileStatus status = algorithm::CompileStatus::Ok;
        juce::String detail;
    };

    class PatchworkEightProcessor : public juce::AudioProcessor,
                                     private juce::AudioProcessorValueTreeState::Listener
    {
    public:
        PatchworkEightProcessor();
        ~PatchworkEightProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return "MURMUR"; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 4.0; }

        bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}

        void getStateInformation(juce::MemoryBlock& destData) override;
        void setStateInformation(const void* data, int sizeInBytes) override;

        /// Loads a new patch. Safe to call from the message thread only -- compiles a
        /// new Engine off-thread and atomically publishes it for processBlock() to pick up.
        bool loadPatch(const patch::Patch& newPatch);

        /// Message-thread only: invoked after a successful `loadPatch()` / preset reload.
        std::function<void()> onPatchLoaded;

        /// Message-thread only: lightweight UI refresh after patch metadata edits that do
        /// not require a full engine reload (e.g. operator engine pill, graph output flags).
        std::function<void()> onPatchMetadataChanged;

        /// Message-thread only: load a `.pw8` from disk and remember its path for preset browsing.
        bool loadPatchFromFile(const juce::String& filePath);

        [[nodiscard]] juce::String getCurrentPresetPath() const noexcept { return currentPresetPath_; }

        /// Message-thread only (WavetableStackView's "Load..." button -- the only
        /// UI anywhere that can assign a wavetable to an operator; everything else
        /// requires hand-editing a .pw8's wavetableId field). Sets operator
        /// `opIndex`'s wavetableId to `filePath` and reloads the current patch so
        /// the file is actually read -- unlike setOrReplaceModRouteLive(), there's
        /// no live-publish path for this: oscillator::loadWavetableFromFile() only
        /// ever runs inside Engine::loadPatch() (see that function and
        /// docs/PATCH_FORMAT.md's "Wavetable Resource Resolution"), so getting a
        /// newly-picked file into the live engine means going through the same
        /// full reload as opening a different patch. Returns false if `opIndex` is
        /// out of range or the file failed to load (see Engine::loadPatch()'s
        /// return value) -- either way, currentPatch_ still records the requested
        /// wavetableId, matching loadPatch()'s existing "a bad reference doesn't
        /// fail the whole patch, that operator just renders silence" contract.
        bool setOperatorWavetableFile(std::size_t opIndex, const juce::String& filePath);

        /// Message-thread only: pull every automatable APVTS field into `currentPatch_`
        /// so UI edits (engine pills, knobs) and patch-only fields (wavetableId) stay
        /// aligned before a reload or save.
        void syncCurrentPatchFromApvts() noexcept;

        /// Message-thread only: sync algorithm node engines from operators and notify UI.
        void notifyPatchMetadataChanged() noexcept;

        /// Swap two adjacent effect slots within the layer insert chain (0..2) or
        /// master chain (0..3). Message-thread only; reloads the engine.
        bool swapEffectSlots(bool masterChain, std::size_t indexA, std::size_t indexB);

        /// Message-thread only (JUCE guarantees editor construction/paint/resize all
        /// run on the message thread, the same thread every currentPatch_ mutation
        /// already runs on per the class-level threading contract above) -- read-only
        /// access for the UI to structural, non-automatable patch data the APVTS
        /// deliberately doesn't cover (the algorithm graph's nodes/edges, patch
        /// metadata) so the editor doesn't need a second, divergent copy of it.
        [[nodiscard]] const patch::Patch& getCurrentPatch() const noexcept { return currentPatch_; }

        /// Message-thread only: compile preview for DESIGN graph editor.
        [[nodiscard]] GraphEditResult tryCompileAlgorithm(const algorithm::AlgorithmGraphDefinition& def) const;

        /// Message-thread only: apply a validated graph to Layer A and reload the engine.
        bool commitAlgorithmGraph(const algorithm::AlgorithmGraphDefinition& def);

        /// Message-thread only: replace Layer A mod routes and publish live (persists to patch for save).
        bool commitModMatrix(const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes);

        /// Message-thread only. True once this player has performed the
        /// drag-to-modulate gesture at least once via setOrReplaceModRouteLive()
        /// in the current session -- deliberately NOT "does the current patch have
        /// any active mod route," since a hand-authored/factory preset can arrive
        /// with routes already wired by its author, before this player has ever
        /// tried the gesture themselves. ModSourceStrip uses this (not
        /// getCurrentPatch().layerA.modRoutes) to decide when its instructional
        /// title has actually done its job.
        [[nodiscard]] bool hasUserCreatedModRouteLive() const noexcept { return hasUserCreatedModRouteLive_; }

        /// Message-thread only (drag-to-modulate, docs/UI.md). Adds a Layer A mod
        /// route, or replaces the existing one if any route already targets the same
        /// (destination, targetIndex) pair -- at most one source per knob in this
        /// UI's model; the full mod matrix's multi-route-per-destination summing is
        /// still reachable via the native .pw8 format, just not this convenience API.
        /// Publishes the change to the live engine WITHOUT rebuilding it (unlike
        /// loadPatch()), so an already-sustaining voice picks it up the very next
        /// block -- see Engine::setModRoutesLive().
        void setOrReplaceModRouteLive(modulation::ModSource source, modulation::ModDestination destination,
                                       std::uint8_t targetIndex, float amount,
                                       modulation::ModScope scope = modulation::ModScope::Voice);

        /// Message-thread only. Removes the Layer A route matching (source,
        /// destination, targetIndex) exactly, if one exists. A no-op otherwise.
        /// Takes `source` too (not just destination/targetIndex) so it can
        /// identify one specific route even when more than one source targets
        /// the same destination -- reachable only via a hand-authored .pw8 (this
        /// UI's own drag-to-modulate gesture keeps the "at most one source per
        /// knob" model setOrReplaceModRouteLive() documents above), but the
        /// connections list (ModSourceStrip) shows and lets you remove every real
        /// route in the patch, including those, so removal has to be exact rather
        /// than "whichever route happens to occupy that destination."
        void removeModRouteLive(modulation::ModSource source, modulation::ModDestination destination,
                                 std::uint8_t targetIndex);

        /// Message-thread only: live arp playhead for UI visualization.
        [[nodiscard]] std::size_t getArpPlayheadStep() const noexcept;
        [[nodiscard]] std::size_t getArpNoteSequenceIndex() const noexcept;

        /// Message-thread only: toggle step enabled/rest and push to live engine.
        void toggleArpStepEnabled(std::size_t stepIndex) noexcept;

        /// Message-thread only: pull mono mix samples for header spectrum scope.
        [[nodiscard]] int readScopeSamples(float* dest, int maxSamples) noexcept { return scopeAudioTap_.readMono(dest, maxSamples); }

        [[nodiscard]] double getScopeSampleRate() const noexcept { return currentSampleRate_; }

        /// Message-thread only: channel 0 mod wheel position (MIDI CC1), 0..1.
        [[nodiscard]] float getModWheelValue() const noexcept;

        /// Message-thread only: channel 0 expression pedal position (MIDI CC11), 0..1.
        [[nodiscard]] float getExpressionValue() const noexcept;

        /// AU sidechain follower active this block (message-thread read of atomic).
        [[nodiscard]] bool getSidechainActive() const noexcept
        {
            return sidechainActive_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] float getSidechainLevel() const noexcept
        {
            return sidechainLevel_.load(std::memory_order_relaxed);
        }

        [[nodiscard]] float getMasterCompressorGainReductionDb(std::size_t slot) const noexcept
        {
            if (slot >= kNumMasterFxSlots)
                return 0.0f;
            return masterCompressorGrDb_[slot].load(std::memory_order_relaxed);
        }

        [[nodiscard]] float getInsertCompressorGainReductionDb(std::size_t slot) const noexcept
        {
            if (slot >= kNumInsertFxSlots)
                return 0.0f;
            return insertCompressorGrDb_[slot].load(std::memory_order_relaxed);
        }

        /// Message-thread mod preview for UI ghost rings (macros, MW, LFO layer tick).
        [[nodiscard]] float getHostBpm() const noexcept;
        [[nodiscard]] modulation::ModSourceValues buildModPreviewSources(float bpm) noexcept;
        [[nodiscard]] bool hasModRouteTo(modulation::ModDestination destination,
                                         std::uint8_t targetIndex) const noexcept;
        [[nodiscard]] bool macroHasActiveRoutes(std::size_t macroIndex) const noexcept;

        [[nodiscard]] ui::ScopeViewMode getScopeViewMode() const noexcept { return scopeViewMode_; }

        void setScopeViewMode(ui::ScopeViewMode mode) noexcept { scopeViewMode_ = mode; }

        /// Message-thread only (a UI-thread poll, same pattern as every other
        /// GATE-3/UI_GATE-4 timer-driven read of live engine state). Reads node
        /// `opIndex`'s currently-loaded wavetable table through the SAME atomic
        /// `activeEngine_` pointer processBlock() reads -- safe for a second, non-
        /// audio, reader here specifically because `publishEngine()`'s double-buffer
        /// only ever destroys the *previous* Engine on the NEXT publishEngine() call,
        /// and that call is itself message-thread-only, so it's sequenced against
        /// this read on the same thread; the audio thread never destroys anything.
        /// Returns nullptr if no Engine has been published yet, or per
        /// Engine::getWavetableTable()'s own nullptr cases (no wavetable loaded for
        /// that node). Never call this from processBlock().
        [[nodiscard]] const oscillator::WavetableTable* getActiveWavetableTable(std::size_t opIndex) const noexcept
        {
            const auto* engine = activeEngine_.load(std::memory_order_acquire);
            return engine != nullptr ? engine->getWavetableTable(opIndex) : nullptr;
        }

        /// The 361 host-automatable parameters (docs/PLUGIN_ARCHITECTURE.md
        /// "Automation"). Public so createEditor()/tests can reach it; processBlock()
        /// reads it via the cached raw-value pointers below, never through this
        /// directly (getRawParameterValue() is the audio-thread-safe path).
        juce::AudioProcessorValueTreeState apvts;

    private:
        void publishEngine(std::unique_ptr<render::Engine> newEngine);

        /// Caches every parameter's `getRawParameterValue()` pointer once, at
        /// construction -- the audio-thread-safe read path processBlock() uses every
        /// block instead of the heavier getParameter()/normalisation lookup.
        void cacheParameterPointers();

        /// Message-thread only: pushes every automatable field currently in
        /// `currentPatch_` into the matching APVTS parameter (e.g. after loading a
        /// preset or a saved session), so the host's UI/automation lane reflects the
        /// patch's values rather than whatever was there from the previously loaded
        /// patch.
        void syncAllParametersFromPatch();

        /// Message-thread only: the reverse direction -- called right before
        /// serializing state, so a saved session captures every automatable field's
        /// current (possibly host-automated) value rather than only the patch's
        /// original defaults. Keeps `currentPatch_` as the single source of truth
        /// described in docs/PLUGIN_ARCHITECTURE.md "Host State" rather than a second,
        /// divergent store of the same 361 values.
        void syncPatchFromAllParameters();

        /// Audio-thread only, called once per block from processBlock(): pushes changed
        /// parameter groups into the live Engine. Falls back to a full push when the
        /// queue is empty (first block after prepare).
        void pushLiveParametersToEngine(render::Engine& engine) noexcept;

        void parameterChanged(const juce::String& parameterID, float newValue) override;

        [[nodiscard]] ParamGroup paramGroupForId(const juce::String& parameterID) const noexcept;

        void registerParamListeners();
        void updateReportedLatency() noexcept;

        /// Message-thread only: writes `routes` into whichever of the two
        /// `modRoutesStorage_` slots wasn't published last, then atomically publishes
        /// a pointer to it -- the same "prepare off-thread, atomic pointer swap"
        /// pattern already used for `activeEngine_` above, just applied to this one
        /// smaller piece of state instead of the whole Engine, so a mod-route change
        /// never has to rebuild (and thereby silence) the currently-playing Engine
        /// the way loadPatch() would. `pushLiveParametersToEngine()` (audio thread)
        /// consumes it via `pendingModRoutes_.exchange(nullptr, ...)`.
        void publishModRoutesLive(const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>& routes);

        void applyMorphFromPosition(float position) noexcept;

        patch::Patch currentPatch_ = patch::Patch::makeInit();
        juce::String currentPresetPath_;

        // Cached once in the constructor: audio-thread-safe raw pointers into the
        // APVTS's atomic parameter storage. Organized to mirror the field-spec tables
        // in PluginState.h exactly (same field order), so building a POD params
        // struct in pushLiveParametersToEngine() is a straight positional read.
        std::array<std::atomic<float>*, 8> macroParamPointers_{};
        std::array<std::atomic<float>*, kNumFilterFields> filterParamPointers_{};
        std::array<std::atomic<float>*, kNumFilter2Fields> filter2ParamPointers_{};
        std::array<std::array<std::atomic<float>*, kNumLfoFields>, kNumLfos> lfoParamPointers_{};
        std::array<std::array<std::atomic<float>*, kNumOperatorFields>, kNumOperators> operatorParamPointers_{};
        std::array<std::array<std::atomic<float>*, kNumOperatorFilterFields>, kNumOperators> operatorFilterParamPointers_{};
        std::array<std::array<std::atomic<float>*, kNumEnvelopeFields>, kNumEnvelopes> envelopeParamPointers_{};
        std::atomic<float>* layerGainPointer_ = nullptr;
        std::atomic<float>* layerPanPointer_ = nullptr;
        std::atomic<float>* masterGainPointer_ = nullptr;
        std::array<std::array<std::atomic<float>*, kNumEffectSlotFields>, kNumInsertFxSlots> insertFxParamPointers_{};
        std::array<std::array<std::atomic<float>*, kNumEffectSlotFields>, kNumMasterFxSlots> masterFxParamPointers_{};
        std::array<std::atomic<float>*, kNumArpFields> arpParamPointers_{};
        std::atomic<float>* modWheelParamPointer_ = nullptr;
        std::atomic<float> mirroredModWheel_{0.0f};
        std::atomic<float>* expressionParamPointer_ = nullptr;
        std::atomic<float> mirroredExpression_{0.0f};
        std::atomic<float>* morphPositionPointer_ = nullptr;

        ::pw8::dsp::SidechainFollower sidechainFollower_{};
        std::atomic<bool> sidechainActive_{false};
        std::atomic<float> sidechainLevel_{0.0f};
        std::array<std::atomic<float>, kNumInsertFxSlots> insertCompressorGrDb_{};
        std::array<std::atomic<float>, kNumMasterFxSlots> masterCompressorGrDb_{};
        std::array<lfo::Lfo, kNumLfos> modPreviewLfos_{};

        /// Tracks host transport so we can all-sound-off when playback stops (Logic rarely
        /// sends note-offs on stop).
        bool hostWasPlaying_ = false;

        // The audio thread only ever dereferences activeEngine_.load(); everything else
        // (loadPatch, prepareToPlay building a replacement) happens off-thread and swaps
        // this pointer in with release/acquire ordering.
        std::atomic<render::Engine*> activeEngine_{nullptr};
        std::unique_ptr<render::Engine> engineStorageA_;
        std::unique_ptr<render::Engine> engineStorageB_;
        bool usingStorageA_ = true;

        // Double-buffered mod-route publishing (see publishModRoutesLive() above).
        // modRoutesStorageUsingA_ is message-thread-only bookkeeping (which slot was
        // written last); pendingModRoutes_ is the cross-thread handoff.
        std::array<core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>, 2> modRoutesStorage_{};
        std::atomic<const core::FixedVector<modulation::ModRoute, core::kMaxModRoutes>*> pendingModRoutes_{nullptr};
        bool modRoutesStorageUsingA_ = true;
        // See hasUserCreatedModRouteLive()'s doc comment above -- set once, the
        // first time setOrReplaceModRouteLive() runs, and never cleared.
        bool hasUserCreatedModRouteLive_ = false;

        ParamChangeQueue paramChangeQueue_{};
        ScopeAudioTap scopeAudioTap_{};
        ui::ScopeViewMode scopeViewMode_ = ui::ScopeViewMode::Fft;
        double currentSampleRate_ = 48000.0;
        render::QualityMode qualityMode_ = render::QualityMode::Normal;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchworkEightProcessor)
    };

} // namespace pw8::plugin
