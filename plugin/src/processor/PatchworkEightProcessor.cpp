// STATUS: PARTIAL -- see PatchworkEightProcessor.h.

#include "processor/PatchworkEightProcessor.h"

#include "pw8/core/AudioBlock.hpp"
#include "pw8/patch/PatchSerializer.hpp"

namespace pw8::plugin
{
    PatchworkEightProcessor::PatchworkEightProcessor()
        : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
    {
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
            macroParamPointers_[i] = apvts.getRawParameterValue(kMacroParameterIds[i]);
        syncMacroParametersFromPatch(); // matches makeInit()'s macro defaults, in case those ever stop being all-zero.

        engineStorageA_ = std::make_unique<render::Engine>();
        engineStorageA_->loadPatch(currentPatch_);
        activeEngine_.store(engineStorageA_.get(), std::memory_order_release);
    }

    PatchworkEightProcessor::~PatchworkEightProcessor() = default;

    void PatchworkEightProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
    {
        // Rebuild both storage slots at the new sample rate and republish -- this always
        // runs on the message thread (JUCE guarantees prepareToPlay isn't concurrent with
        // processBlock), so a plain rebuild-then-swap is safe without extra locking.
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(sampleRate);
        fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
    }

    void PatchworkEightProcessor::releaseResources() {}

    void PatchworkEightProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
    {
        auto* engine = activeEngine_.load(std::memory_order_acquire);
        if (engine == nullptr)
        {
            buffer.clear();
            return;
        }

        // Tempo-synced LFOs need the host tempo; default to 120 BPM when unavailable
        // (e.g. no playhead, or a host that doesn't report it).
        float bpm = 120.0f;
        if (auto* playHead = getPlayHead())
        {
            if (const auto position = playHead->getPosition(); position.hasValue())
                if (const auto hostBpm = position->getBpm())
                    bpm = static_cast<float>(*hostBpm);
        }
        engine->setTempo(bpm);

        // Push the host's current macro parameter values into the running Engine
        // every block -- audio-thread-safe (a handful of atomic loads plus plain
        // float writes into fixed-capacity voice arrays, see
        // Engine::setMacroValue()), and cheap enough to just always do rather than
        // diff against last block's values. This is what makes macro automation
        // audible on a currently-held note, not just the next one.
        for (std::size_t i = 0; i < macroParamPointers_.size(); ++i)
            if (macroParamPointers_[i] != nullptr)
                engine->setMacroValue(i, macroParamPointers_[i]->load(std::memory_order_relaxed));

        for (const auto metadata : midiMessages)
        {
            const auto msg = metadata.getMessage();
            if (msg.isNoteOn())
                engine->noteOn(msg.getNoteNumber(), msg.getChannel() - 1, msg.getVelocity());
            else if (msg.isNoteOff())
                engine->noteOff(msg.getNoteNumber(), msg.getChannel() - 1, msg.getVelocity());
            else if (msg.isPitchWheel())
                engine->pitchBend(msg.getChannel() - 1, msg.getPitchWheelValue());
            else if (msg.isController())
                engine->controlChange(msg.getChannel() - 1, msg.getControllerNumber(), msg.getControllerValue());
            else if (msg.isChannelPressure())
                engine->channelPressure(msg.getChannel() - 1, msg.getChannelPressureValue());
            else if (msg.isAftertouch())
                engine->polyAftertouch(msg.getChannel() - 1, msg.getNoteNumber(), msg.getAfterTouchValue());
        }

        buffer.clear();
        if (buffer.getNumChannels() < 2)
            return;

        core::StereoBlockView view(buffer.getWritePointer(0), buffer.getWritePointer(1),
                                    static_cast<std::size_t>(buffer.getNumSamples()));
        engine->process(view);
    }

    juce::AudioProcessorEditor* PatchworkEightProcessor::createEditor()
    {
        // Pragmatic placeholder until the real PLAY/DESIGN/LAB UI lands (Phase 17):
        // JUCE's built-in generic parameter editor. Now shows 8 real sliders (Macro
        // 1..8, backed by `apvts`) rather than an empty list -- but it's still a real,
        // consistent editor either way, which is what AudioProcessor::hasEditor()
        // promises callers.
        return new juce::GenericAudioProcessorEditor(*this);
    }

    void PatchworkEightProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
        // Bake the macros' current (possibly host-automated-since-load) values back
        // into currentPatch_ before serializing, so a saved session round-trips them
        // exactly rather than only ever saving the preset's original defaults.
        syncPatchMacrosFromParameters();
        const auto json = patch::savePatchToJson(currentPatch_);
        destData.replaceAll(json.data(), json.size());
    }

    void PatchworkEightProcessor::setStateInformation(const void* data, int sizeInBytes)
    {
        const std::string_view json(static_cast<const char*>(data), static_cast<std::size_t>(sizeInBytes));
        const auto result = patch::loadPatchFromJson(json);
        if (result.ok)
            loadPatch(result.patch);
    }

    bool PatchworkEightProcessor::loadPatch(const patch::Patch& newPatch)
    {
        currentPatch_ = newPatch;
        syncMacroParametersFromPatch();
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(getSampleRate() > 0.0 ? getSampleRate() : 48000.0);
        const bool ok = fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
        return ok;
    }

    void PatchworkEightProcessor::syncMacroParametersFromPatch()
    {
        for (std::size_t i = 0; i < kMacroParameterIds.size(); ++i)
        {
            if (auto* param = apvts.getParameter(kMacroParameterIds[i]))
                param->setValueNotifyingHost(currentPatch_.macros[i].value); // range is 0..1, so value == normalized.
        }
    }

    void PatchworkEightProcessor::syncPatchMacrosFromParameters()
    {
        for (std::size_t i = 0; i < macroParamPointers_.size(); ++i)
            if (macroParamPointers_[i] != nullptr)
                currentPatch_.macros[i].value = macroParamPointers_[i]->load(std::memory_order_relaxed);
    }

    void PatchworkEightProcessor::publishEngine(std::unique_ptr<render::Engine> newEngine)
    {
        // Double-buffer so the previously-active Engine (which the audio thread may have
        // just finished reading) stays alive until the NEXT swap, rather than being
        // destroyed out from under a very last in-flight processBlock() call.
        if (usingStorageA_)
        {
            engineStorageB_ = std::move(newEngine);
            activeEngine_.store(engineStorageB_.get(), std::memory_order_release);
        }
        else
        {
            engineStorageA_ = std::move(newEngine);
            activeEngine_.store(engineStorageA_.get(), std::memory_order_release);
        }
        usingStorageA_ = !usingStorageA_;
    }

} // namespace pw8::plugin

// Standard JUCE plugin entry point.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new pw8::plugin::PatchworkEightProcessor();
}
