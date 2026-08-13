#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "pw8/content/ContentPaths.hpp"
#include "pw8/midi/MidiEvent.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/render/Renderer.hpp"
#include "support/Sha256.hpp"

using namespace pw8;
namespace fs = std::filesystem;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr double kDurationSeconds = 1.5;
    constexpr std::uint64_t kGoldenSeed = 0x507738ULL;

    [[nodiscard]] fs::path repoRoot() noexcept
    {
#ifdef PW8_REPO_ROOT
        return fs::path(PW8_REPO_ROOT);
#else
        return fs::current_path();
#endif
    }

    [[nodiscard]] fs::path goldenManifestPath() noexcept
    {
        return repoRoot() / "tests/golden/presets.json";
    }

    [[nodiscard]] midi::MidiSequence singleNoteMidi() noexcept
    {
        midi::MidiSequence seq;
        seq.events.push_back(midi::MidiEvent{0.0, midi::EventType::NoteOn, 0, 60, 100, 0, 0});
        seq.events.push_back(midi::MidiEvent{1.0, midi::EventType::NoteOff, 0, 60, 0, 0, 0});
        return seq;
    }

    [[nodiscard]] std::string sha256Interleaved(const std::vector<float>& interleaved) noexcept
    {
        test::Sha256 hasher;
        hasher.update(interleaved.data(), interleaved.size());
        return hasher.hexDigest();
    }

    [[nodiscard]] std::string renderPresetHash(patch::Patch patch) noexcept
    {
        render::RenderOptions options;
        options.sampleRate = kSampleRate;
        options.durationSecondsOverride = kDurationSeconds;
        options.bpm = 120.0;
        options.quality = render::QualityMode::Offline;
        options.seed = kGoldenSeed;
        patch.seed = kGoldenSeed;

        const auto result = render::render(patch, singleNoteMidi(), options);
        REQUIRE(result.ok);
        return sha256Interleaved(result.interleavedStereo);
    }

    [[nodiscard]] patch::Patch loadPresetRelative(const std::string& relPath)
    {
        const fs::path abs = repoRoot() / relPath;
        std::ifstream in(abs);
        REQUIRE(in.good());
        std::ostringstream ss;
        ss << in.rdbuf();
        const auto loaded = patch::loadPatchFromJson(ss.str());
        REQUIRE(loaded.ok);
        return loaded.patch;
    }

    [[nodiscard]] bool isGoldenFactorySlot(const std::string& filename) noexcept
    {
        if (filename.size() < 3 || !std::isdigit(static_cast<unsigned char>(filename[0]))
            || !std::isdigit(static_cast<unsigned char>(filename[1])) || filename[2] != '-')
            return false;

        const int slot = (filename[0] - '0') * 10 + (filename[1] - '0');
        return slot >= 1 && slot <= 49 && ((slot - 1) % 3) == 0;
    }

    [[nodiscard]] std::vector<std::string> defaultPresetPaths()
    {
        std::vector<std::string> paths {
            "content/presets/init-saw.pw8",
            "content/presets/fm-bell.pw8",
            "content/presets/wt-morph.pw8",
            "content/presets/arp-pluck.pw8",
        };

        const fs::path factoryRoot = repoRoot() / "content/presets/factory";
        static constexpr const char* kCategories[] = {
            "Basses", "Leads", "Pads", "Ambient", "Sequences", "Warp",
        };

        for (const char* category : kCategories)
        {
            const fs::path categoryDir = factoryRoot / category;
            if (!fs::is_directory(categoryDir))
                continue;

            std::vector<fs::path> files;
            for (const auto& entry : fs::directory_iterator(categoryDir))
            {
                if (entry.path().extension() != ".pw8")
                    continue;
                const auto filename = entry.path().filename().string();
                if (std::strcmp(category, "Warp") == 0 || isGoldenFactorySlot(filename))
                    files.push_back(entry.path());
            }

            std::sort(files.begin(), files.end());
            for (const auto& file : files)
            {
                const auto rel = fs::relative(file, repoRoot());
                paths.push_back(rel.generic_string());
            }
        }

        return paths;
    }

    void registerContentRoots() noexcept
    {
        content::resetSearchRootsForTests();
        content::addSearchRoot(repoRoot().string());
    }
} // namespace

TEST_CASE("Regenerate golden preset hash manifest", "[golden][generate]")
{
    if (std::getenv("PW8_WRITE_GOLDEN") == nullptr)
        SKIP("Set PW8_WRITE_GOLDEN=1 to regenerate tests/golden/presets.json");

    registerContentRoots();

    nlohmann::json doc;
    doc["schemaVersion"] = 1;
    nlohmann::json presets = nlohmann::json::array();

    for (const auto& rel : defaultPresetPaths())
    {
        const auto hash = renderPresetHash(loadPresetRelative(rel));
        presets.push_back({{"path", rel},
                           {"sha256", hash},
                           {"sampleRate", kSampleRate},
                           {"durationSeconds", kDurationSeconds},
                           {"seed", kGoldenSeed},
                           {"quality", "offline"}});
    }

    doc["presets"] = presets;

    const auto outPath = goldenManifestPath();
    fs::create_directories(outPath.parent_path());
    std::ofstream out(outPath);
    REQUIRE(out.good());
    out << doc.dump(2) << '\n';
}

TEST_CASE("Factory preset renders match golden SHA256 hashes", "[golden][regression]")
{
    registerContentRoots();

    const auto manifestPath = goldenManifestPath();
    if (!fs::is_regular_file(manifestPath))
        SKIP("Missing tests/golden/presets.json -- run with PW8_WRITE_GOLDEN=1 first");

    std::ifstream in(manifestPath);
    REQUIRE(in.good());
    const auto doc = nlohmann::json::parse(in);

    for (const auto& entry : doc.at("presets"))
    {
        const std::string rel = entry.at("path").get<std::string>();
        const std::string expected = entry.at("sha256").get<std::string>();
        INFO("preset: " << rel);

        const auto hash = renderPresetHash(loadPresetRelative(rel));
        REQUIRE(hash == expected);
    }
}
