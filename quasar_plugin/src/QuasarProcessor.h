#pragma once

#include <array>

#include <juce_audio_processors/juce_audio_processors.h>

#include "pw8/effects/BinauralSpace.hpp"
#include "QuasarParamLayout.h"

namespace pw8::quasar
{
    class QuasarProcessor : public juce::AudioProcessor
    {
    public:
        QuasarProcessor();
        ~QuasarProcessor() override = default;

        void prepareToPlay(double sampleRate, int samplesPerBlock) override;
        void releaseResources() override;
        void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;

        juce::AudioProcessorEditor* createEditor() override;
        bool hasEditor() const override { return true; }

        const juce::String getName() const override { return JucePlugin_Name; }
        bool acceptsMidi() const override { return false; }
        bool producesMidi() const override { return false; }
        double getTailLengthSeconds() const override { return 20.0; }

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
        [[nodiscard]] effects::BinauralSpaceParams readEffectParams() const noexcept;
        [[nodiscard]] float readHostBpm() const noexcept;

        juce::AudioProcessorValueTreeState apvts_;
        effects::BinauralSpaceProcessor processor_;
        std::array<std::atomic<float>*, kNumQuasarParams> paramPtrs_{};

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasarProcessor)
    };

} // namespace pw8::quasar
