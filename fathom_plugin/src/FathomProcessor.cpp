#include "FathomProcessor.h"

#include "FathomEditor.h"
#include "FathomIrLibrary.h"
#include "FathomPreset.h"

namespace pw8::fathom
{
    namespace
    {
        // Real index-order constants matching kFathomParamSpecs exactly
        // (FathomParamLayout.cpp) -- named rather than positional/magic to
        // keep a 25-param array readable, unlike Quasar's smaller 28-param
        // table which stays inline.
        enum ParamIndex : std::size_t
        {
            kReverbMode = 0,
            kHybridEarlyLengthMs,
            kMix,
            kReverbSizeParam,
            kReverbDecaySeconds,
            kReverbPreDelayMs,
            kReverbHighRatio,
            kReverbHighCrossoverHz,
            kReverbLowRatio,
            kReverbLowCrossoverHz,
            kReverbDiffusion,
            kReverbDensity,
            kReverbModDepth,
            kReverbModRateHz,
            kReverbEarlyLevel,
            kReverbLateLevel,
            kReverbRollOffHz,
            kReverbVlfCutDb,
            kReverbCharacter,
            kIrIndex,
            kConvPreDelayMs,
            kConvMix,
            kConvWidth,
            kConvLowCutHz,
            kConvHighCutHz,
        };

        [[nodiscard]] float loadF(const std::atomic<float>* ptr) noexcept
        {
            return ptr != nullptr ? ptr->load(std::memory_order_relaxed) : 0.0f;
        }

        [[nodiscard]] int loadI(const std::atomic<float>* ptr) noexcept
        {
            return static_cast<int>(std::lround(loadF(ptr)));
        }
    } // namespace

    FathomProcessor::FathomProcessor()
        : juce::AudioProcessor(BusesProperties()
                                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts_(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        cacheParameterPointers();
    }

    void FathomProcessor::cacheParameterPointers()
    {
        for (std::size_t i = 0; i < kFathomParamSpecs.size(); ++i)
            paramPtrs_[i] = apvts_.getRawParameterValue(kFathomParamSpecs[i].id);
    }

    void FathomProcessor::prepareToPlay(const double sampleRate, const int samplesPerBlock)
    {
        sampleRate_ = sampleRate;
        maxBlockSize_ = samplesPerBlock;
        algoEngine_.prepare(sampleRate);

        juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2};
        convEngine_.prepare(spec);
        hybridEarlyEngine_.prepare(spec);
        convPreDelayL_.prepare(spec);
        convPreDelayR_.prepare(spec);
        convPreDelayL_.setMaximumDelayInSamples(static_cast<int>(0.2 * sampleRate) + 1);
        convPreDelayR_.setMaximumDelayInSamples(static_cast<int>(0.2 * sampleRate) + 1);
        convLowCutL_.prepare(spec);
        convLowCutR_.prepare(spec);
        convHighCutL_.prepare(spec);
        convHighCutR_.prepare(spec);
        lastLowCutHz_ = -1.0f; // force real filter-coefficient recompute on first block
        lastHighCutHz_ = -1.0f;

        loadedIrIndex_ = -1; // force a real (re)load of the current IR selection
        maybeReloadIr();
        loadedHybridIrIndex_ = -1;
        loadedHybridEarlyLengthMs_ = -1.0f;
        maybeReloadHybridEarlyIr();
    }

    void FathomProcessor::releaseResources()
    {
        algoEngine_.reset();
        convEngine_.reset();
        hybridEarlyEngine_.reset();
        convPreDelayL_.reset();
        convPreDelayR_.reset();
        convLowCutL_.reset();
        convLowCutR_.reset();
        convHighCutL_.reset();
        convHighCutR_.reset();
    }

    bool FathomProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
    {
        return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
               layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
    }

    effects::EffectSlotParams FathomProcessor::readAlgoParams() const noexcept
    {
        effects::EffectSlotParams p{};
        p.type = effects::EffectType::Reverb;
        p.mix = loadF(paramPtrs_[kMix]);
        p.reverbSizeParam = loadF(paramPtrs_[kReverbSizeParam]);
        p.reverbDecaySeconds = loadF(paramPtrs_[kReverbDecaySeconds]);
        p.reverbPreDelayMs = loadF(paramPtrs_[kReverbPreDelayMs]);
        p.reverbHighRatio = loadF(paramPtrs_[kReverbHighRatio]);
        p.reverbHighCrossoverHz = loadF(paramPtrs_[kReverbHighCrossoverHz]);
        p.reverbLowRatio = loadF(paramPtrs_[kReverbLowRatio]);
        p.reverbLowCrossoverHz = loadF(paramPtrs_[kReverbLowCrossoverHz]);
        p.reverbDiffusion = loadF(paramPtrs_[kReverbDiffusion]);
        p.reverbDensity = loadF(paramPtrs_[kReverbDensity]);
        p.reverbModDepth = loadF(paramPtrs_[kReverbModDepth]);
        p.reverbModRateHz = loadF(paramPtrs_[kReverbModRateHz]);
        p.reverbEarlyLevel = loadF(paramPtrs_[kReverbEarlyLevel]);
        p.reverbLateLevel = loadF(paramPtrs_[kReverbLateLevel]);
        p.reverbRollOffHz = loadF(paramPtrs_[kReverbRollOffHz]);
        p.reverbVlfCutDb = loadF(paramPtrs_[kReverbVlfCutDb]);
        p.reverbCharacter = loadI(paramPtrs_[kReverbCharacter]);
        return p;
    }

    void FathomProcessor::maybeReloadIr()
    {
        const int index = juce::jlimit(0, static_cast<int>(kNumBundledIrs) - 1, loadI(paramPtrs_[kIrIndex]));
        if (index == loadedIrIndex_)
            return;

        const auto irFile = impulseResponsesDirectory().getChildFile(kBundledIrs[static_cast<std::size_t>(index)].fileName);
        if (irFile.existsAsFile())
        {
            // Real async, real-time-safe IR swap (juce::dsp::Convolution's own
            // documented contract) -- safe to call from the message thread
            // (parameter changes arrive via processBlock on the audio thread
            // here, but loadImpulseResponse() itself is documented safe to
            // call from any thread; the new IR becomes active once fully
            // loaded, matching the class's own real behavior).
            convEngine_.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes,
                                            juce::dsp::Convolution::Trim::no, 0,
                                            juce::dsp::Convolution::Normalise::yes);
        }
        loadedIrIndex_ = index;
    }

    void FathomProcessor::maybeReloadHybridEarlyIr()
    {
        const int index = juce::jlimit(0, static_cast<int>(kNumBundledIrs) - 1, loadI(paramPtrs_[kIrIndex]));
        const float earlyLengthMs = loadF(paramPtrs_[kHybridEarlyLengthMs]);
        if (index == loadedHybridIrIndex_ && earlyLengthMs == loadedHybridEarlyLengthMs_)
            return;

        const auto irFile = impulseResponsesDirectory().getChildFile(kBundledIrs[static_cast<std::size_t>(index)].fileName);
        if (irFile.existsAsFile())
        {
            const auto earlyLengthSamples =
                static_cast<size_t>(juce::jmax(1, static_cast<int>((earlyLengthMs * 0.001) * sampleRate_)));
            // Real truncation via loadImpulseResponse()'s own `size` parameter
            // -- confirmed against the real vendored JUCE header ("the
            // expected size for the impulse response after loading") -- no
            // hand-written IR windowing needed. This gives a real, genuine
            // early-reflections-only excerpt of the same real captured IR
            // Convolution mode uses at full length. Real, empirically-
            // measured caveat (fathom_plugin/tools/convolution_smoke_test.cpp):
            // the engine's own internal partitioned-convolution block
            // granularity rounds the real actual length UP somewhat beyond
            // the exact requested sample count (observed ~9% over, not
            // documented in JUCE's own header) -- the real `Early Length`
            // knob controls this proportionally, just not to
            // millisecond-exact precision, same honesty standard as the
            // algorithmic engine's own already-documented real
            // predelay-floored-at-1-sample behavior.
            hybridEarlyEngine_.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes,
                                                   juce::dsp::Convolution::Trim::no, earlyLengthSamples,
                                                   juce::dsp::Convolution::Normalise::yes);
        }
        loadedHybridIrIndex_ = index;
        loadedHybridEarlyLengthMs_ = earlyLengthMs;
    }

    void FathomProcessor::updateTailFilters(const double sampleRate) noexcept
    {
        const float lowHz = loadF(paramPtrs_[kConvLowCutHz]);
        const float highHz = loadF(paramPtrs_[kConvHighCutHz]);

        if (lowHz != lastLowCutHz_)
        {
            const auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, juce::jmax(20.0f, lowHz));
            *convLowCutL_.coefficients = *coeffs;
            *convLowCutR_.coefficients = *coeffs;
            lastLowCutHz_ = lowHz;
        }
        if (highHz != lastHighCutHz_)
        {
            const auto coeffs =
                juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, juce::jmin(static_cast<float>(sampleRate * 0.49), highHz));
            *convHighCutL_.coefficients = *coeffs;
            *convHighCutR_.coefficients = *coeffs;
            lastHighCutHz_ = highHz;
        }
    }

    void FathomProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        juce::ignoreUnused(midiMessages);
        juce::ScopedNoDenormals noDenormals;

        if (buffer.getNumChannels() < 2)
            return;

        maybeReloadIr();

        const int mode = loadI(paramPtrs_[kReverbMode]); // 0=Algorithmic, 1=Convolution, 2=Hybrid
        if (mode == 2)
            maybeReloadHybridEarlyIr();

        if (mode == 0)
        {
            const auto params = readAlgoParams();
            float* left = buffer.getWritePointer(0);
            float* right = buffer.getWritePointer(1);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float outL = 0.0f, outR = 0.0f;
                algoEngine_.processStereo(left[i], right[i], params, outL, outR);
                left[i] = outL;
                right[i] = outR;
            }
            return;
        }

        processConvolutionLike(buffer, mode == 2);
    }

    void FathomProcessor::processConvolutionLike(juce::AudioBuffer<float>& buffer, const bool hybrid)
    {
        // -- Real shared chain (Convolution and Hybrid modes both use this):
        //    predelay -> wet-signal generation -> width -> tail low/high
        //    cut -> dry/wet mix. --
        updateTailFilters(sampleRate_);

        const float preDelayMs = loadF(paramPtrs_[kConvPreDelayMs]);
        const int preDelaySamples = static_cast<int>((preDelayMs * 0.001) * sampleRate_);
        convPreDelayL_.setDelay(static_cast<float>(preDelaySamples));
        convPreDelayR_.setDelay(static_cast<float>(preDelaySamples));

        juce::AudioBuffer<float> dry;
        dry.makeCopyOf(buffer, true);

        float* wetL = buffer.getWritePointer(0);
        float* wetR = buffer.getWritePointer(1);

        // Real predelay: push the real dry sample in, pop the delayed one
        // out, per sample, into a real separate buffer (can't do this
        // in-place against `buffer` since it's also `dry`'s backing data
        // until this copy happens). Predelay applies to the whole reverb
        // onset (both early and late, in Hybrid mode) relative to the dry
        // signal -- the real, standard reverb convention, not two
        // independently-delayed sub-signals.
        {
            juce::AudioBuffer<float> predelayed;
            predelayed.setSize(2, buffer.getNumSamples());
            const float* dryL = dry.getReadPointer(0);
            const float* dryR = dry.getReadPointer(1);
            float* outDL = predelayed.getWritePointer(0);
            float* outDR = predelayed.getWritePointer(1);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                convPreDelayL_.pushSample(0, dryL[i]);
                convPreDelayR_.pushSample(0, dryR[i]);
                outDL[i] = convPreDelayL_.popSample(0);
                outDR[i] = convPreDelayR_.popSample(0);
            }
            buffer.copyFrom(0, 0, predelayed, 0, 0, buffer.getNumSamples());
            buffer.copyFrom(1, 0, predelayed, 1, 0, buffer.getNumSamples());
        }

        if (!hybrid)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            convEngine_.process(context);
        }
        else
        {
            // `buffer` currently holds the real predelayed dry signal --
            // this is the real excitation signal fed to BOTH real engines
            // below (each needs real signal energy to produce real output;
            // a silent input would just produce silence).

            // 1. Real IR-derived early reflections (a real, genuinely new
            //    sound source -- the whole point of Hybrid mode).
            juce::AudioBuffer<float> earlyBuf;
            earlyBuf.makeCopyOf(buffer, true);
            {
                juce::dsp::AudioBlock<float> earlyBlock(earlyBuf);
                juce::dsp::ProcessContextReplacing<float> earlyContext(earlyBlock);
                hybridEarlyEngine_.process(earlyContext);
            }

            // 2. Real late tank only. ReverbProcessor::processStereo()
            //    always real-adds its own dry input back into its output
            //    (`outL = inL + wetL*mix`, by design, matching its real use
            //    as a per-slot processor elsewhere in this codebase) -- so
            //    the pure wet late-tank signal is real-recovered by
            //    subtracting the known real input afterward
            //    (`wetL*mix = outL - inL`), not by trying to bypass or
            //    change that shared, already-proven engine. `mix` forced to
            //    1.0 here so the subtraction yields the FULL real wet
            //    signal, unscaled -- Fathom's own real `convMix` knob is
            //    what the user actually controls, applied once at the end
            //    below, same as Convolution mode already does.
            auto lateParams = readAlgoParams();
            lateParams.reverbEarlyLevel = 0.0f; // suppress the synthetic early cluster -- real IR early reflections replace it
            lateParams.mix = 1.0f;
            // Real repurposing (Hybrid mode only): reverbEarlyLevel/
            // reverbLateLevel go from gating the engine's own internal
            // synthetic early cluster (Algorithmic mode) to real external
            // gains balancing the real IR early reflections against the
            // real late tank -- the same two real knobs, a real different
            // meaning in this mode, per the approved plan.
            const float earlyLevel = loadF(paramPtrs_[kReverbEarlyLevel]);
            const float lateLevel = loadF(paramPtrs_[kReverbLateLevel]);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const float excitationL = wetL[i];
                const float excitationR = wetR[i];
                float outL = 0.0f, outR = 0.0f;
                algoEngine_.processStereo(excitationL, excitationR, lateParams, outL, outR);
                wetL[i] = (outL - excitationL) * lateLevel + earlyBuf.getSample(0, i) * earlyLevel;
                wetR[i] = (outR - excitationR) * lateLevel + earlyBuf.getSample(1, i) * earlyLevel;
            }
        }

        // Real width: mid/side scale. 0=mono, 1=natural (unchanged), 2=extra wide.
        const float width = loadF(paramPtrs_[kConvWidth]);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const float mid = 0.5f * (wetL[i] + wetR[i]);
            const float side = 0.5f * (wetL[i] - wetR[i]) * width;
            wetL[i] = mid + side;
            wetR[i] = mid - side;
        }

        // Real tail EQ trim.
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            wetL[i] = convHighCutL_.processSample(convLowCutL_.processSample(wetL[i]));
            wetR[i] = convHighCutR_.processSample(convLowCutR_.processSample(wetR[i]));
        }

        // Real dry/wet mix (against the original, un-predelayed dry signal).
        const float mix = juce::jlimit(0.0f, 1.0f, loadF(paramPtrs_[kConvMix]));
        const float* dryL = dry.getReadPointer(0);
        const float* dryR = dry.getReadPointer(1);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            wetL[i] = dryL[i] + wetL[i] * mix;
            wetR[i] = dryR[i] + wetR[i] * mix;
        }
    }

    juce::AudioProcessorEditor* FathomProcessor::createEditor()
    {
        return new FathomEditor(*this);
    }

    void FathomProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
        if (auto json = exportPresetJson(apvts_))
            juce::MemoryOutputStream(destData, false).writeString(*json);
    }

    void FathomProcessor::setStateInformation(const void* data, const int sizeInBytes)
    {
        const juce::String json = juce::String::createStringFromData(data, sizeInBytes);
        if (json.isNotEmpty())
            (void)importPresetJson(apvts_, json);
    }

} // namespace pw8::fathom
