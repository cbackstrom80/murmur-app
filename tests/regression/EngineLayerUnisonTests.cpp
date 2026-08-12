#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

#include "pw8/patch/Patch.hpp"
#include "pw8/render/Renderer.hpp"

using namespace pw8;

TEST_CASE("Renderer: STACK mode sums layer A and layer B", "[render][stack]")
{
    patch::Patch p = patch::Patch::makeInit();
    p.layerMode = patch::LayerMode::Stack;
    p.layerA.operators[0].engine = algorithm::EngineType::Classic;
    p.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Sine;
    p.layerA.gain = 0.5f;

    p.layerB.operators[0].engine = algorithm::EngineType::Classic;
    p.layerB.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    p.layerB.gain = 0.5f;

    for (auto* layer : {&p.layerA, &p.layerB})
    {
        layer->envelopes[0].attackSeconds = 0.005f;
        layer->envelopes[0].decaySeconds = 0.05f;
        layer->envelopes[0].sustainLevel = 0.8f;
        layer->envelopes[0].releaseSeconds = 0.1f;
    }

    midi::MidiSequence midi;
    midi.events.push_back(midi::MidiEvent{0.0, midi::EventType::NoteOn, 0, 60, 100, 0, 0});
    midi.events.push_back(midi::MidiEvent{0.5, midi::EventType::NoteOff, 0, 60, 0, 0, 0});

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 0.75;

    const auto stackResult = render::render(p, midi, options);

    p.layerMode = patch::LayerMode::SingleA;
    const auto singleResult = render::render(p, midi, options);

    REQUIRE(stackResult.ok);
    REQUIRE(singleResult.ok);

    auto peak = [](const std::vector<float>& interleaved) {
        float maxAbs = 0.0f;
        for (float s : interleaved)
            maxAbs = std::max(maxAbs, std::abs(s));
        return maxAbs;
    };

    REQUIRE(peak(stackResult.interleavedStereo) > peak(singleResult.interleavedStereo) * 1.2f);
}

TEST_CASE("Renderer: unison voices produce wider stereo output than mono", "[render][unison]")
{
    patch::Patch mono = patch::Patch::makeInit();
    mono.layerA.operators[0].engine = algorithm::EngineType::Classic;
    mono.layerA.operators[0].classicWaveform = oscillator::ClassicWaveform::Saw;
    mono.layerA.unison.voices = 1;
    mono.layerA.envelopes[0].attackSeconds = 0.01f;
    mono.layerA.envelopes[0].sustainLevel = 1.0f;
    mono.layerA.envelopes[0].releaseSeconds = 0.2f;

    patch::Patch wide = mono;
    wide.layerA.unison.mode = patch::UnisonMode::Full;
    wide.layerA.unison.voices = 5;
    wide.layerA.unison.detuneCents = 18.0f;
    wide.layerA.unison.spread = 1.0f;
    wide.layerA.unison.blend = 1.0f;

    midi::MidiSequence midi;
    midi.events.push_back(midi::MidiEvent{0.0, midi::EventType::NoteOn, 0, 60, 100, 0, 0});
    midi.events.push_back(midi::MidiEvent{0.6, midi::EventType::NoteOff, 0, 60, 0, 0, 0});

    render::RenderOptions options;
    options.sampleRate = 48000.0;
    options.durationSecondsOverride = 0.8;

    const auto monoResult = render::render(mono, midi, options);
    const auto wideResult = render::render(wide, midi, options);
    REQUIRE(monoResult.ok);
    REQUIRE(wideResult.ok);

    auto stereoSpread = [](const std::vector<float>& interleaved) {
        double sumL = 0.0;
        double sumR = 0.0;
        const std::size_t frames = interleaved.size() / 2;
        for (std::size_t i = 0; i < frames; ++i)
        {
            sumL += static_cast<double>(interleaved[i * 2]) * interleaved[i * 2];
            sumR += static_cast<double>(interleaved[i * 2 + 1]) * interleaved[i * 2 + 1];
        }
        const double rmsL = std::sqrt(sumL / static_cast<double>(frames));
        const double rmsR = std::sqrt(sumR / static_cast<double>(frames));
        return std::abs(rmsL - rmsR);
    };

    REQUIRE(stereoSpread(wideResult.interleavedStereo) > stereoSpread(monoResult.interleavedStereo));
}
