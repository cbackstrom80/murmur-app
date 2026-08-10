#pragma once

// STATUS: SCAFFOLD / PARTIAL, not yet compiled against a real JUCE checkout in this
// pass -- see plugin/CMakeLists.txt and docs/PLUGIN_ARCHITECTURE.md.
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

#include <atomic>
#include <memory>

#include <juce_audio_processors/juce_audio_processors.h>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Engine.hpp"

namespace pw8::plugin
{
    class PatchworkEightProcessor : public juce::AudioProcessor
    {
    public:
        PatchworkEightProcessor();
        ~PatchworkEightProcessor() override;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return "Patchwork Eight"; }
        bool acceptsMidi() const override { return true; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 4.0; }

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

    private:
        void publishEngine(std::unique_ptr<render::Engine> newEngine);

        patch::Patch currentPatch_ = patch::Patch::makeInit();

        // The audio thread only ever dereferences activeEngine_.load(); everything else
        // (loadPatch, prepareToPlay building a replacement) happens off-thread and swaps
        // this pointer in with release/acquire ordering.
        std::atomic<render::Engine*> activeEngine_{nullptr};
        std::unique_ptr<render::Engine> engineStorageA_;
        std::unique_ptr<render::Engine> engineStorageB_;
        bool usingStorageA_ = true;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PatchworkEightProcessor)
    };

} // namespace pw8::plugin
