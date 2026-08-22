// Real, throwaway smoke test for the NEW part of Fathom's DSP -- direct
// juce::dsp::Convolution usage against a real bundled IR file. The
// algorithmic engine (pw8::effects::ReverbProcessor) is NOT re-tested here;
// it's the same already-shipping DSP with its own real 9-case test suite
// (tests/unit/EffectsTests.cpp). This probe exists specifically to prove
// the genuinely new capability -- IR loading + real-time convolution --
// actually produces real, non-silent, non-NaN, plausible output, not just
// that it compiles.
//
// Not wired into ctest -- ad hoc, matches this session's own established
// "throwaway C++ probe program" pattern for direct DSP verification.

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <path-to-ir.wav>\n", argv[0]);
        return 1;
    }

    const juce::File irFile(argv[1]);
    if (!irFile.existsAsFile())
    {
        std::fprintf(stderr, "IR file not found: %s\n", argv[1]);
        return 1;
    }

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 512;

    juce::dsp::Convolution conv;
    juce::dsp::ProcessSpec spec{sampleRate, static_cast<juce::uint32>(blockSize), 2};
    conv.prepare(spec);
    conv.loadImpulseResponse(irFile, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, 0,
                             juce::dsp::Convolution::Normalise::yes);
    conv.prepare(spec); // real, documented behavior: blocks until the most recent loadImpulseResponse() is ready

    // Real unit impulse in, real IR response out.
    juce::AudioBuffer<float> buffer(2, blockSize);
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);

    double sumAbs = 0.0;
    double peak = 0.0;
    bool hasNanOrInf = false;
    int numBlocksToRender = 40; // ~426ms @ 48kHz/512 -- enough for most of these real bundled IRs' own real length

    for (int b = 0; b < numBlocksToRender; ++b)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        conv.process(context);

        for (int ch = 0; ch < 2; ++ch)
        {
            const float* data = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i)
            {
                const float sample = data[i];
                if (!std::isfinite(sample))
                    hasNanOrInf = true;
                sumAbs += std::abs(sample);
                peak = std::max(peak, static_cast<double>(std::abs(sample)));
            }
        }

        buffer.clear(); // real: only the very first block ever has real input; the rest is pure IR tail
    }

    const double meanAbs = sumAbs / (2.0 * blockSize * numBlocksToRender);

    std::printf("ir=%s peak=%.6f meanAbs=%.6f nanOrInf=%s irLengthSamples=%d\n", irFile.getFileNameWithoutExtension().toRawUTF8(),
               peak, meanAbs, hasNanOrInf ? "true" : "false", conv.getCurrentIRSize());

    if (hasNanOrInf || peak < 1.0e-6)
    {
        std::fprintf(stderr, "FAIL: near-silent or non-finite output\n");
        return 1;
    }
    return 0;
}
