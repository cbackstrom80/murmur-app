#include "QuasarProcessor.h"

#include "QuasarEditor.h"
#include "QuasarPreset.h"
#include "pw8/effects/QuasarSpatialMacros.hpp"

namespace pw8::quasar
{
    namespace
    {
        [[nodiscard]] float loadF(const std::atomic<float>* ptr) noexcept
        {
            return ptr != nullptr ? ptr->load(std::memory_order_relaxed) : 0.0f;
        }

        [[nodiscard]] int loadI(const std::atomic<float>* ptr) noexcept
        {
            return static_cast<int>(std::lround(loadF(ptr)));
        }

        [[nodiscard]] bool loadB(const std::atomic<float>* ptr) noexcept
        {
            return loadF(ptr) >= 0.5f;
        }
    } // namespace

    QuasarProcessor::QuasarProcessor()
#if JucePlugin_Build_AU
        : juce::AudioProcessor(BusesProperties()
                                   .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#else
        : juce::AudioProcessor(BusesProperties()
                                   .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                   .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
#endif
          apvts_(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        cacheParameterPointers();
    }

    void QuasarProcessor::cacheParameterPointers()
    {
        for (std::size_t i = 0; i < kQuasarParamSpecs.size(); ++i)
            paramPtrs_[i] = apvts_.getRawParameterValue(kQuasarParamSpecs[i].id);
    }

    void QuasarProcessor::prepareToPlay(const double sampleRate, int /*samplesPerBlock*/)
    {
        processor_.prepare(sampleRate);
    }

    void QuasarProcessor::releaseResources()
    {
        processor_.reset();
    }

    bool QuasarProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

#if JucePlugin_Build_AU
        if (getBusCount(true) > 1)
        {
            const auto sidechain = layouts.getChannelSet(true, 1);
            if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() &&
                sidechain != juce::AudioChannelSet::stereo())
                return false;
        }
#endif
        return true;
    }

    effects::BinauralSpaceParams QuasarProcessor::readEffectParams() const noexcept
    {
        effects::BinauralSpaceParams p{};
        p.mix = loadF(paramPtrs_[0]);
        p.qsr1Level = loadF(paramPtrs_[1]);
        p.qsr2Level = loadF(paramPtrs_[2]);
        p.cntrLevel = loadF(paramPtrs_[3]);
        p.inputSplitHpfHz = loadF(paramPtrs_[4]);
        p.cntrHpfHz = loadF(paramPtrs_[5]);
        p.qsr1Height = loadF(paramPtrs_[6]);
        p.qsr1AngleDeg = loadF(paramPtrs_[7]);
        p.qsr1Distance = loadF(paramPtrs_[8]);
        p.qsr2Height = loadF(paramPtrs_[9]);
        p.qsr2AngleDeg = loadF(paramPtrs_[10]);
        p.qsr2Distance = loadF(paramPtrs_[11]);
        p.qsr1RoomAmount = loadF(paramPtrs_[12]);
        p.qsr1RoomSize = loadF(paramPtrs_[13]);
        p.qsr1RoomDamping = loadF(paramPtrs_[14]);
        p.qsr2RoomAmount = loadF(paramPtrs_[15]);
        p.qsr2RoomSize = loadF(paramPtrs_[16]);
        p.qsr2RoomDamping = loadF(paramPtrs_[17]);
        p.quasarDelayTimeMs = loadF(paramPtrs_[18]);
        p.quasarDelayFeedback = loadF(paramPtrs_[19]);
        p.quasarDelayVolume = loadF(paramPtrs_[20]);
        p.quasarOutputMode = loadI(paramPtrs_[21]);
        p.quasarCrossfeed = loadF(paramPtrs_[22]);
        p.quasarDelaySync = loadB(paramPtrs_[23]);
        p.quasarDelaySyncDivisionIndex = loadI(paramPtrs_[24]);
        p.qsrStereoSplit = true;

        effects::QuasarSpatialTargets spatial{
            p.qsr1AngleDeg, p.qsr2AngleDeg, p.qsr1Distance, p.qsr2Distance, p.qsr1Height, p.qsr2Height};
        const effects::QuasarMacroKoinValues macros{loadF(paramPtrs_[25]), loadF(paramPtrs_[26])};
        const auto macroApplied = effects::applyQuasarMacroKoins(spatial, macros);
        p.qsr1AngleDeg = macroApplied.qsr1AngleDeg;
        p.qsr2AngleDeg = macroApplied.qsr2AngleDeg;
        p.qsr1Distance = macroApplied.qsr1Distance;
        p.qsr2Distance = macroApplied.qsr2Distance;
        p.qsr1Height = macroApplied.qsr1Height;
        p.qsr2Height = macroApplied.qsr2Height;
        return p;
    }

    float QuasarProcessor::readHostBpm() const noexcept
    {
        if (auto* playHead = getPlayHead())
        {
            if (auto pos = playHead->getPosition())
            {
                if (pos->getBpm().hasValue())
                    return static_cast<float>(*pos->getBpm());
            }
        }
        return 120.0f;
    }

    void QuasarProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        juce::ignoreUnused(midiMessages);
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();
        auto mainBlock = getBusBuffer(buffer, true, 0);
        auto outBlock = getBusBuffer(buffer, false, 0);

        if (outBlock.getNumChannels() < 2)
            return;

        float* outL = outBlock.getWritePointer(0);
        float* outR = outBlock.getWritePointer(1);

        const float* inL = mainBlock.getNumChannels() > 0 ? mainBlock.getReadPointer(0) : nullptr;
        const float* inR = mainBlock.getNumChannels() > 1 ? mainBlock.getReadPointer(1) : inL;

#if JucePlugin_Build_AU
        const auto sidechainBlock = getBusBuffer(buffer, true, 1);
        const bool sidechainBusEnabled =
            getBusCount(true) > 1 && !getChannelLayoutOfBus(true, 1).isDisabled();
        const float* scL = sidechainBusEnabled && sidechainBlock.getNumChannels() > 0
                               ? sidechainBlock.getReadPointer(0)
                               : nullptr;
        const float* scR = sidechainBusEnabled && sidechainBlock.getNumChannels() > 1
                               ? sidechainBlock.getReadPointer(1)
                               : scL;
        const bool hasSidechain =
            sidechainBusEnabled && scL != nullptr && sidechainBlock.getNumSamples() >= numSamples;
#else
        const float* scL = nullptr;
        const float* scR = nullptr;
        constexpr bool hasSidechain = false;
#endif

        const auto params = readEffectParams();
        const float bpm = readHostBpm();
        const bool sidechainToQsr2 = loadB(paramPtrs_[27]);

        for (int i = 0; i < numSamples; ++i)
        {
            const float mainLeft = inL != nullptr ? inL[i] : 0.0f;
            const float mainRight = inR != nullptr ? inR[i] : mainLeft;

            float procL = mainLeft;
            float procR = mainRight;
            float qsr2AuxIn = mainRight;
            bool useQsr2Aux = false;
            if (hasSidechain)
            {
                const float scLeft = scL[i];
                const float scRight = scR != nullptr ? scR[i] : scLeft;
                if (sidechainToQsr2)
                {
                    qsr2AuxIn = (scLeft + scRight) * 0.5f;
                    useQsr2Aux = true;
                }
                else
                {
                    procL = mainLeft + scLeft;
                    procR = mainRight + scRight;
                }
            }

            float wetL = 0.0f;
            float wetR = 0.0f;
            processor_.processStereo(procL, procR, params, wetL, wetR, bpm, qsr2AuxIn, useQsr2Aux);
            outL[i] = wetL;
            outR[i] = wetR;
        }
    }

    juce::AudioProcessorEditor* QuasarProcessor::createEditor()
    {
        return new QuasarEditor(*this);
    }

    void QuasarProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
        if (auto json = exportPresetJson(apvts_))
            juce::MemoryOutputStream(destData, false).writeString(*json);
    }

    void QuasarProcessor::setStateInformation(const void* data, const int sizeInBytes)
    {
        const juce::String json = juce::String::createStringFromData(data, sizeInBytes);
        if (json.isNotEmpty())
            (void)importPresetJson(apvts_, json);
    }

} // namespace pw8::quasar

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pw8::quasar::QuasarProcessor();
}
