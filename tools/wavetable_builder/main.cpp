// murmur-wavetable-builder -- imports a mono 16-bit PCM WAV single-cycle-or-multi-cycle
// source, splits it into equal-length frames, normalizes each frame, and emits a
// pw8 wavetable JSON table with real band-limited mip levels.
//
// Status: IMPLEMENTED. Each frame is treated as exactly one wavetable cycle, so its
// forward FFT bins correspond exactly to harmonics (bin k == harmonic k). Mip level 0
// is full-bandwidth (unmodified); each subsequent level halves the retained harmonic
// count via FFT harmonic truncation (zero out bins above the cutoff, and their
// conjugate-symmetric mirror, then inverse FFT) -- standard band-limiting technique,
// not sourced from any product. `--samples-per-frame` must be a power of two (the
// FFT requirement); `--mip-levels` caps how many are generated (fewer are emitted
// once the retained harmonic count would drop to zero).
//
//   murmur-wavetable-builder --input source.wav --frames 8 --samples-per-frame 2048 \
//                          --output content/wavetables/my_table.json [--mip-levels 10]

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "pw8/dsp/Fft.hpp"
#include "pw8/oscillator/WavetableTable.hpp"

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

    /// Band-limits one frame to `maxHarmonic` via FFT harmonic truncation. `frame`
    /// must have a power-of-two size. `maxHarmonic == fullBandwidthHarmonic` is a
    /// no-op copy (mip level 0).
    std::vector<float> bandLimitFrame(const std::vector<float>& frame, int maxHarmonic)
    {
        const std::size_t n = frame.size();
        std::vector<std::complex<float>> spectrum(n);
        for (std::size_t i = 0; i < n; ++i)
            spectrum[i] = std::complex<float>(frame[i], 0.0f);

        pw8::dsp::fft(spectrum, /*invert=*/false);

        const auto nyquistBin = static_cast<int>(n / 2);
        for (int bin = 0; bin < static_cast<int>(n); ++bin)
        {
            // Harmonic index for this bin: bins 1..nyquistBin-1 are positive
            // frequencies k; bins n-1..nyquistBin+1 are their conjugate mirrors
            // (harmonic n-bin). Bin 0 is DC, bin nyquistBin is the Nyquist bin itself.
            const int harmonic = (bin <= nyquistBin) ? bin : static_cast<int>(n) - bin;
            if (harmonic > maxHarmonic)
                spectrum[static_cast<std::size_t>(bin)] = {0.0f, 0.0f};
        }

        pw8::dsp::fft(spectrum, /*invert=*/true);

        std::vector<float> out(n);
        for (std::size_t i = 0; i < n; ++i)
            out[i] = spectrum[i].real();
        return out;
    }

} // namespace

int main(int argc, char** argv)
{
    std::string inputPath, outputPath;
    int numFrames = 1;
    int samplesPerFrame = 2048;
    int requestedMipLevels = 10;

    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string { return (i + 1 < argc) ? argv[++i] : std::string{}; };
        if (arg == "--input") inputPath = next();
        else if (arg == "--output") outputPath = next();
        else if (arg == "--frames") numFrames = std::stoi(next());
        else if (arg == "--samples-per-frame") samplesPerFrame = std::stoi(next());
        else if (arg == "--mip-levels") requestedMipLevels = std::stoi(next());
        else if (arg == "--help")
        {
            std::cout << "Usage: murmur-wavetable-builder --input <in.wav> --output <out.json> "
                         "[--frames N] [--samples-per-frame N (power of two)] [--mip-levels N]\n";
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
    if (!pw8::dsp::isPowerOfTwo(static_cast<std::size_t>(samplesPerFrame)))
    {
        std::cerr << "--samples-per-frame must be a power of two (FFT requirement); got " << samplesPerFrame << ".\n";
        return 2;
    }
    if (requestedMipLevels < 1 || requestedMipLevels > static_cast<int>(pw8::oscillator::kMaxWavetableMipLevels))
    {
        std::cerr << "--mip-levels must be between 1 and " << pw8::oscillator::kMaxWavetableMipLevels << ".\n";
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

    // Extract and per-frame-normalize the full-bandwidth (mip 0) frames first.
    std::vector<std::vector<float>> fullBandwidthFrames(static_cast<std::size_t>(numFrames));
    for (int fr = 0; fr < numFrames; ++fr)
    {
        float peak = 1.0e-9f;
        for (int i = 0; i < samplesPerFrame; ++i)
            peak = std::max(peak, std::abs(wav.samples[static_cast<std::size_t>(fr) * static_cast<std::size_t>(samplesPerFrame) +
                                                        static_cast<std::size_t>(i)]));

        std::vector<float> frame(static_cast<std::size_t>(samplesPerFrame));
        for (int i = 0; i < samplesPerFrame; ++i)
            frame[static_cast<std::size_t>(i)] =
                wav.samples[static_cast<std::size_t>(fr) * static_cast<std::size_t>(samplesPerFrame) + static_cast<std::size_t>(i)] /
                peak;
        fullBandwidthFrames[static_cast<std::size_t>(fr)] = std::move(frame);
    }

    // Build mip levels: level 0 is full-bandwidth (maxHarmonic = Nyquist bin - 1);
    // each subsequent level halves the retained harmonic count until it would hit 0.
    const int fullBandwidthHarmonic = samplesPerFrame / 2 - 1;
    std::vector<int> mipHarmonics;
    for (int level = 0, maxHarmonic = fullBandwidthHarmonic; level < requestedMipLevels && maxHarmonic >= 1; ++level, maxHarmonic /= 2)
        mipHarmonics.push_back(maxHarmonic);

    nlohmann::json mipsJson = nlohmann::json::array();
    for (std::size_t level = 0; level < mipHarmonics.size(); ++level)
    {
        const int maxHarmonic = mipHarmonics[level];
        nlohmann::json framesJson = nlohmann::json::array();

        for (int fr = 0; fr < numFrames; ++fr)
        {
            const auto& src = fullBandwidthFrames[static_cast<std::size_t>(fr)];
            const std::vector<float> banded =
                (level == 0) ? src : bandLimitFrame(src, maxHarmonic);

            nlohmann::json frameJson = nlohmann::json::array();
            for (float s : banded)
                frameJson.push_back(s);
            framesJson.push_back(frameJson);
        }

        nlohmann::json mipJson;
        mipJson["maxHarmonic"] = maxHarmonic;
        mipJson["frames"] = framesJson;
        mipsJson.push_back(mipJson);
    }

    nlohmann::json root;
    root["schemaVersion"] = 2; // v2: multi-mip "mips" array (v1 had a single flat "frames" array).
    root["numFrames"] = numFrames;
    root["samplesPerFrame"] = samplesPerFrame;
    root["mipLevels"] = static_cast<int>(mipsJson.size());
    root["sourceFile"] = inputPath;
    root["mips"] = mipsJson;

    std::ofstream out(outputPath);
    if (!out.is_open())
    {
        std::cerr << "Failed to open output file: " << outputPath << "\n";
        return 1;
    }
    out << root.dump(2);

    std::cout << "Wrote wavetable: " << outputPath << " (" << numFrames << " frames x " << samplesPerFrame
              << " samples, " << mipsJson.size() << " mip levels, harmonics "
              << mipHarmonics.front() << ".." << mipHarmonics.back() << ")\n";
    return 0;
}
