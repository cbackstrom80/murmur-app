#pragma once

#include <array>
#include <atomic>

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "pw8/effects/EffectTypes.hpp"
#include "pw8/effects/Reverb.hpp"
#include "FathomParamLayout.h"

namespace pw8::fathom
{
    /// Fathom's real processor -- real structural twin of QuasarProcessor
    /// (stereo in/out, no MIDI, no Patch/Engine/voice machinery), holding
    /// real DSP engines side by side:
    ///   - algoEngine_: the real, already-shipping
    ///     pw8::effects::ReverbProcessor (the same 8-line Householder FDN
    ///     behind MURMUR's/Undertow's own master effect slot) -- zero fork.
    ///   - convEngine_: a real juce::dsp::Convolution loading one of the 38
    ///     real bundled impulse responses (FathomIrLibrary.h) at full
    ///     length, for pure Convolution mode.
    ///   - hybridEarlyEngine_ (Phase 2): a second, real
    ///     juce::dsp::Convolution loading the SAME selected IR but
    ///     truncated to `hybridEarlyLengthMs` real samples
    ///     (juce::dsp::Convolution::loadImpulseResponse's own real `size`
    ///     parameter does this truncation -- no hand-written windowing
    ///     needed) -- real captured early reflections, summed with
    ///     algoEngine_'s real late tank (reverbEarlyLevel forced to 0 at
    ///     the call site so only the synthetic early cluster is
    ///     suppressed, not the real late-tank params the user dialed in).
    /// `reverbMode` (APVTS, 0/1/2) selects Algorithmic/Convolution/Hybrid.
    class FathomProcessor : public juce::AudioProcessor
    {
    public:
        FathomProcessor();
        ~FathomProcessor() override = default;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return JucePlugin_Name; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 30.0; } // real max Decay range

        bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram(int) override {}
        const juce::String getProgramName(int) override { return {}; }
        void changeProgramName(int, const juce::String&) override {}

        void getStateInformation(juce::MemoryBlock& destData) override;
        void setStateInformation(const void* data, int sizeInBytes) override;

        [[nodiscard]] juce::AudioProcessorValueTreeState& getApvts() noexcept { return apvts_; }

    private:
        void cacheParameterPointers();
        [[nodiscard]] effects::EffectSlotParams readAlgoParams() const noexcept;
        void maybeReloadIr();
        void maybeReloadHybridEarlyIr();
        void updateTailFilters(double sampleRate) noexcept;
        void processConvolutionLike(juce::AudioBuffer<float>& buffer, bool hybrid);

        juce::AudioProcessorValueTreeState apvts_;
        std::array<std::atomic<float>*, kNumFathomParams> paramPtrs_{};

        effects::ReverbProcessor algoEngine_;
        juce::dsp::Convolution convEngine_;
        int loadedIrIndex_ = -1;

        // Phase 2: real second convolution engine for Hybrid mode's
        // real IR-derived early reflections (see class doc comment above).
        juce::dsp::Convolution hybridEarlyEngine_;
        int loadedHybridIrIndex_ = -1;
        float loadedHybridEarlyLengthMs_ = -1.0f;

        // Real convolution-mode signal chain, shared by Convolution and
        // Hybrid modes: predelay -> (convolution engine(s)) -> width ->
        // tail low/high cut -> dry/wet mix. All real, all functional -- no
        // decorative unwired knobs.
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> convPreDelayL_{
            static_cast<int>(0.2 * 96000)}; // real kMaxReverbPreDelaySeconds-equivalent cap (200ms @ up to 96kHz)
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> convPreDelayR_{
            static_cast<int>(0.2 * 96000)};
        juce::dsp::IIR::Filter<float> convLowCutL_, convLowCutR_;
        juce::dsp::IIR::Filter<float> convHighCutL_, convHighCutR_;
        float lastLowCutHz_ = -1.0f;
        float lastHighCutHz_ = -1.0f;

        double sampleRate_ = 48000.0;
        int maxBlockSize_ = 512;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FathomProcessor)
    };

} // namespace pw8::fathom
