// STATUS: SCAFFOLD / PARTIAL -- see PatchworkEightProcessor.h.

#include "processor/PatchworkEightProcessor.h"

#include "pw8/core/AudioBlock.hpp"
#include "pw8/patch/PatchSerializer.hpp"

namespace pw8::plugin
{
    PatchworkEightProcessor::PatchworkEightProcessor()
        : juce::AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
    {
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
        // JUCE's built-in generic parameter editor. It'll be an empty list today since
        // no juce::AudioProcessorValueTreeState parameters are registered yet
        // (plugin/src/parameters/ is still PLANNED) -- but it's a real, consistent
        // editor rather than a null one, which is what AudioProcessor::hasEditor()
        // promises callers.
        return new juce::GenericAudioProcessorEditor(*this);
    }

    void PatchworkEightProcessor::getStateInformation(juce::MemoryBlock& destData)
    {
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
        auto fresh = std::make_unique<render::Engine>();
        fresh->prepare(getSampleRate() > 0.0 ? getSampleRate() : 48000.0);
        const bool ok = fresh->loadPatch(currentPatch_);
        publishEngine(std::move(fresh));
        return ok;
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
