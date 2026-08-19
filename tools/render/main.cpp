// murmur-render -- native offline renderer CLI.
//
//   murmur-render --patch content/presets/dark-bass.murmur \
//              --midi content/test_midi/bass.mid \
//              --bpm 105 --sample-rate 48000 \
//              --output /tmp/dark-bass.wav [--receipt /tmp/dark-bass.receipt.json]
//
// Renders WITHOUT any JUCE plugin hosting or DAW involvement -- see docs/RENDERER.md.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "pw8/core/Version.hpp"
#include "pw8/midi/StandardMidiFile.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/render/Renderer.hpp"
#include "pw8/render/WavWriter.hpp"

namespace
{
    struct Args
    {
        std::string patchPath;
        std::string midiPath;
        std::string outputPath;
        std::string receiptPath;
        double bpm = 120.0;
        double sampleRate = 48000.0;
        double durationOverride = -1.0;
        double releaseTail = 2.0;
        std::uint64_t seed = 0;
        bool help = false;
    };

    void printUsage()
    {
        std::cout <<
            "murmur-render -- MURMUR native offline renderer\n\n"
            "Usage:\n"
            "  murmur-render --patch <file.murmur> --midi <file.mid> --output <file.wav> [options]\n\n"
            "Options:\n"
            "  --patch <path>        Path to a .murmur (or legacy .pw8) patch file (required)\n"
            "  --midi <path>         Path to a Standard MIDI File (required)\n"
            "  --output <path>       Path to write the rendered WAV file (required)\n"
            "  --receipt <path>      Optional path to write a JSON render receipt\n"
            "  --bpm <n>             Tempo hint (informational; SMF tempo events take precedence)\n"
            "  --sample-rate <n>     Output sample rate in Hz (default 48000)\n"
            "  --duration <seconds>  Force total render duration (default: MIDI length + release tail)\n"
            "  --release-tail <s>    Extra seconds rendered after the last MIDI event (default 2.0)\n"
            "  --seed <n>            Deterministic seed override\n"
            "  --help                Show this message\n";
    }

    bool parseArgs(int argc, char** argv, Args& out)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string arg = argv[i];
            auto next = [&](const char* flag) -> std::string {
                if (i + 1 >= argc)
                {
                    std::cerr << "Missing value for " << flag << "\n";
                    std::exit(2);
                }
                return argv[++i];
            };

            if (arg == "--help" || arg == "-h") out.help = true;
            else if (arg == "--patch") out.patchPath = next("--patch");
            else if (arg == "--midi") out.midiPath = next("--midi");
            else if (arg == "--output") out.outputPath = next("--output");
            else if (arg == "--receipt") out.receiptPath = next("--receipt");
            else if (arg == "--bpm") out.bpm = std::stod(next("--bpm"));
            else if (arg == "--sample-rate") out.sampleRate = std::stod(next("--sample-rate"));
            else if (arg == "--duration") out.durationOverride = std::stod(next("--duration"));
            else if (arg == "--release-tail") out.releaseTail = std::stod(next("--release-tail"));
            else if (arg == "--seed") out.seed = std::stoull(next("--seed"));
            else
            {
                std::cerr << "Unknown argument: " << arg << "\n";
                return false;
            }
        }
        return true;
    }

    std::string readWholeFile(const std::string& path, bool& ok)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open())
        {
            ok = false;
            return {};
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        ok = true;
        return ss.str();
    }

    std::vector<std::uint8_t> readWholeFileBytes(const std::string& path, bool& ok)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open())
        {
            ok = false;
            return {};
        }
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        ok = true;
        return bytes;
    }

    std::string jsonEscape(const std::string& s)
    {
        std::string out;
        for (char c : s)
        {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }
        return out;
    }
} // namespace

int main(int argc, char** argv)
{
    Args args;
    if (!parseArgs(argc, argv, args))
    {
        printUsage();
        return 2;
    }
    if (args.help)
    {
        printUsage();
        return 0;
    }
    if (args.patchPath.empty() || args.midiPath.empty() || args.outputPath.empty())
    {
        std::cerr << "--patch, --midi and --output are all required.\n\n";
        printUsage();
        return 2;
    }

    bool ok = false;
    const std::string patchJson = readWholeFile(args.patchPath, ok);
    if (!ok)
    {
        std::cerr << "Could not read patch file: " << args.patchPath << "\n";
        return 1;
    }

    const auto patchResult = pw8::patch::loadPatchFromJson(patchJson);
    if (!patchResult.ok)
    {
        std::cerr << "Failed to parse patch: " << patchResult.error << "\n";
        return 1;
    }

    const auto midiBytes = readWholeFileBytes(args.midiPath, ok);
    if (!ok)
    {
        std::cerr << "Could not read MIDI file: " << args.midiPath << "\n";
        return 1;
    }

    const auto smf = pw8::midi::readStandardMidiFile(midiBytes);
    if (!smf.ok)
    {
        std::cerr << "Failed to parse MIDI file: " << smf.error << "\n";
        return 1;
    }

    pw8::render::RenderOptions options;
    options.sampleRate = args.sampleRate;
    options.bpm = args.bpm;
    options.durationSecondsOverride = args.durationOverride;
    options.releaseTailSeconds = args.releaseTail;
    options.seed = args.seed;

    const auto result = pw8::render::render(patchResult.patch, smf.sequence, options);
    if (!result.ok)
    {
        std::cerr << "Render failed: " << result.error << "\n";
        return 1;
    }

    if (!pw8::render::writeWavFileFloat32(args.outputPath, result.interleavedStereo, result.sampleRate))
    {
        std::cerr << "Failed to write WAV file: " << args.outputPath << "\n";
        return 1;
    }

    std::cout << "Rendered " << result.metrics.durationSeconds << "s -> " << args.outputPath << "\n"
              << "  peak=" << result.metrics.peak << " rms=" << result.metrics.rms
              << " nanOrInf=" << (result.metrics.containsNaNOrInf ? "true" : "false") << "\n";

    if (!args.receiptPath.empty())
    {
        std::ofstream receipt(args.receiptPath);
        if (receipt.is_open())
        {
            receipt << "{\n"
                    << "  \"engineVersion\": \"" << pw8::core::EngineVersion::string() << "\",\n"
                    << "  \"patchSchemaVersion\": " << patchResult.patch.schemaVersion << ",\n"
                    << "  \"patch\": \"" << jsonEscape(args.patchPath) << "\",\n"
                    << "  \"midi\": \"" << jsonEscape(args.midiPath) << "\",\n"
                    << "  \"sampleRate\": " << result.sampleRate << ",\n"
                    << "  \"bpm\": " << args.bpm << ",\n"
                    << "  \"seed\": " << args.seed << ",\n"
                    << "  \"duration\": " << result.metrics.durationSeconds << ",\n"
                    << "  \"metrics\": {\n"
                    << "    \"peak\": " << result.metrics.peak << ",\n"
                    << "    \"rms\": " << result.metrics.rms << ",\n"
                    << "    \"dcOffsetLeft\": " << result.metrics.dcOffsetLeft << ",\n"
                    << "    \"dcOffsetRight\": " << result.metrics.dcOffsetRight << ",\n"
                    << "    \"leftRightBalance\": " << result.metrics.leftRightBalance << ",\n"
                    << "    \"containsNaNOrInf\": " << (result.metrics.containsNaNOrInf ? "true" : "false") << "\n"
                    << "  }\n"
                    << "}\n";
        }
    }

    return 0;
}
