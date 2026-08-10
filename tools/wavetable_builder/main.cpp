// pw8-wavetable-builder -- imports a mono 16-bit PCM WAV single-cycle-or-multi-cycle
// source, splits it into equal-length frames, normalizes each frame, and emits a
// pw8 wavetable JSON table.
//
// Status: PARTIAL, matching pw8::oscillator::WavetableOscillator's current
// limitations (see docs/DSP_ENGINE.md) -- this tool does NOT yet generate
// band-limited mip levels per octave; it emits exactly one (full-bandwidth) frame
// set. Mip-level generation is tracked in docs/ROADMAP.md as a Phase 2 follow-up.
//
//   pw8-wavetable-builder --input source.wav --frames 8 --samples-per-frame 2048 \
//                          --output content/wavetables/my_table.pw8wt.json

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    struct WavData
    {
        bool ok = false;
        std::string error;
        std::vector<float> samples; // mono, normalized to [-1, 1] range already by PCM scale.
        std::uint32_t sampleRate = 44100;
    };

    WavData readMonoPcm16Wav(const std::string& path)
    {
        WavData result;
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open())
        {
            result.error = "Could not open input file";
            return result;
        }

        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (bytes.size() < 44 || std::memcmp(bytes.data(), "RIFF", 4) != 0 || std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
        {
            result.error = "Not a valid RIFF/WAVE file";
            return result;
        }

        std::uint16_t numChannels = 1;
        std::uint16_t bitsPerSample = 16;
        std::uint16_t formatTag = 1;
        std::size_t pos = 12;
        const std::uint8_t* dataStart = nullptr;
        std::uint32_t dataSize = 0;

        while (pos + 8 <= bytes.size())
        {
            char chunkId[5] = {0};
            std::memcpy(chunkId, bytes.data() + pos, 4);
            std::uint32_t chunkSize;
            std::memcpy(&chunkSize, bytes.data() + pos + 4, 4);
            const std::size_t chunkDataStart = pos + 8;

            if (std::strcmp(chunkId, "fmt ") == 0 && chunkDataStart + 16 <= bytes.size())
            {
                std::memcpy(&formatTag, bytes.data() + chunkDataStart, 2);
                std::memcpy(&numChannels, bytes.data() + chunkDataStart + 2, 2);
                std::memcpy(&result.sampleRate, bytes.data() + chunkDataStart + 4, 4);
                std::memcpy(&bitsPerSample, bytes.data() + chunkDataStart + 14, 2);
            }
            else if (std::strcmp(chunkId, "data") == 0)
            {
                dataStart = bytes.data() + chunkDataStart;
                dataSize = chunkSize;
            }

            pos = chunkDataStart + chunkSize + (chunkSize % 2);
        }

        if (dataStart == nullptr)
        {
            result.error = "No data chunk found";
            return result;
        }
        if (formatTag != 1 || bitsPerSample != 16)
        {
            result.error = "Only 16-bit PCM WAV is supported by this tool in this pass";
            return result;
        }

        const std::size_t numSampleFrames = dataSize / (2 * numChannels);
        result.samples.reserve(numSampleFrames);
        for (std::size_t i = 0; i < numSampleFrames; ++i)
        {
            std::int16_t raw;
            std::memcpy(&raw, dataStart + i * numChannels * 2, 2); // channel 0 only (mono-down-mix by truncation).
            result.samples.push_back(static_cast<float>(raw) / 32768.0f);
        }

        result.ok = true;
        return result;
    }
} // namespace

int main(int argc, char** argv)
{
    std::string inputPath, outputPath;
    int numFrames = 1;
    int samplesPerFrame = 2048;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string{}; };
        if (arg == "--input") inputPath = next();
        else if (arg == "--output") outputPath = next();
        else if (arg == "--frames") numFrames = std::stoi(next());
        else if (arg == "--samples-per-frame") samplesPerFrame = std::stoi(next());
        else if (arg == "--help")
        {
            std::cout << "Usage: pw8-wavetable-builder --input <in.wav> --output <out.json> "
                         "[--frames N] [--samples-per-frame N]\n";
            return 0;
        }
    }

    if (inputPath.empty() || outputPath.empty())
    {
        std::cerr << "--input and --output are required. See --help.\n";
        return 2;
    }
    if (numFrames < 1 || samplesPerFrame < 8)
    {
        std::cerr << "Invalid --frames/--samples-per-frame.\n";
        return 2;
    }

    const auto wav = readMonoPcm16Wav(inputPath);
    if (!wav.ok)
    {
        std::cerr << "Failed to read WAV: " << wav.error << "\n";
        return 1;
    }

    const std::size_t needed = static_cast<std::size_t>(numFrames) * static_cast<std::size_t>(samplesPerFrame);
    if (wav.samples.size() < needed)
    {
        std::cerr << "Input WAV has " << wav.samples.size() << " samples but " << needed
                  << " are needed for " << numFrames << " frames of " << samplesPerFrame << " samples.\n";
        return 1;
    }

    nlohmann::json framesJson = nlohmann::json::array();
    for (int fr = 0; fr < numFrames; ++fr)
    {
        // Normalize each frame independently to unit peak (avoids silent/quiet frames
        // if the source has uneven levels across its cycles).
        float peak = 1.0e-9f;
        for (int i = 0; i < samplesPerFrame; ++i)
            peak = std::max(peak, std::abs(wav.samples[static_cast<std::size_t>(fr) * static_cast<std::size_t>(samplesPerFrame) +
                                                        static_cast<std::size_t>(i)]));

        nlohmann::json frame = nlohmann::json::array();
        for (int i = 0; i < samplesPerFrame; ++i)
        {
            const float s = wav.samples[static_cast<std::size_t>(fr) * static_cast<std::size_t>(samplesPerFrame) +
                                         static_cast<std::size_t>(i)] / peak;
            frame.push_back(s);
        }
        framesJson.push_back(frame);
    }

    nlohmann::json root;
    root["schemaVersion"] = 1;
    root["numFrames"] = numFrames;
    root["samplesPerFrame"] = samplesPerFrame;
    root["mipLevels"] = 1; // PARTIAL: single (full-bandwidth) mip level only, see docs/DSP_ENGINE.md.
    root["sourceFile"] = inputPath;
    root["frames"] = framesJson;

    std::ofstream out(outputPath);
    if (!out.is_open())
    {
        std::cerr << "Failed to open output file: " << outputPath << "\n";
        return 1;
    }
    out << root.dump(2);

    std::cout << "Wrote wavetable: " << outputPath << " (" << numFrames << " frames x " << samplesPerFrame
              << " samples)\n";
    return 0;
}
