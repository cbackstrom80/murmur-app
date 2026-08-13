#include <catch2/catch_test_macros.hpp>
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

    [[nodiscard]] std::vector<std::string> defaultPresetPaths()
    {
        return {
            // Core + arp (4) + factory representatives (85 = 89 total)
            "content/presets/init-saw.pw8",
            "content/presets/fm-bell.pw8",
            "content/presets/wt-morph.pw8",
            "content/presets/arp-pluck.pw8",
            "content/presets/factory/Basses/01-slack-drive.pw8",
            "content/presets/factory/Basses/04-ratio-pulse.pw8",
            "content/presets/factory/Basses/07-coil-low.pw8",
            "content/presets/factory/Basses/10-ladder-low.pw8",
            "content/presets/factory/Basses/13-bronze-line.pw8",
            "content/presets/factory/Basses/16-sludge-depth.pw8",
            "content/presets/factory/Basses/19-slack-bass.pw8",
            "content/presets/factory/Basses/22-trench-sub.pw8",
            "content/presets/factory/Basses/25-squelch-line.pw8",
            "content/presets/factory/Basses/28-growl-drive.pw8",
            "content/presets/factory/Basses/31-fathom-low.pw8",
            "content/presets/factory/Basses/34-accent-sub.pw8",
            "content/presets/factory/Basses/37-accent-low.pw8",
            "content/presets/factory/Basses/40-grind-drive.pw8",
            "content/presets/factory/Basses/43-thud-bass.pw8",
            "content/presets/factory/Basses/46-bronze-growl.pw8",
            "content/presets/factory/Basses/49-root-low.pw8",
            "content/presets/factory/Leads/01-comet-spike.pw8",
            "content/presets/factory/Leads/04-pixel-sync.pw8",
            "content/presets/factory/Leads/07-prism-cut.pw8",
            "content/presets/factory/Leads/10-glass-line.pw8",
            "content/presets/factory/Leads/13-flare-sync.pw8",
            "content/presets/factory/Leads/16-comet-edge.pw8",
            "content/presets/factory/Leads/19-vector-lead.pw8",
            "content/presets/factory/Leads/22-blade-spike.pw8",
            "content/presets/factory/Leads/25-solar-wave.pw8",
            "content/presets/factory/Leads/28-glass-beam.pw8",
            "content/presets/factory/Leads/31-spike-line.pw8",
            "content/presets/factory/Leads/34-pixel-beam.pw8",
            "content/presets/factory/Leads/37-fifth-section.pw8",
            "content/presets/factory/Leads/40-voltage-wave.pw8",
            "content/presets/factory/Leads/43-voltage-sync.pw8",
            "content/presets/factory/Leads/46-vector-wave.pw8",
            "content/presets/factory/Leads/49-voltage-line.pw8",
            "content/presets/factory/Pads/01-amber-pad.pw8",
            "content/presets/factory/Pads/04-feather-drift.pw8",
            "content/presets/factory/Pads/07-pale-sky.pw8",
            "content/presets/factory/Pads/10-velvet-veil.pw8",
            "content/presets/factory/Pads/13-wistful-pad.pw8",
            "content/presets/factory/Pads/16-halcyon-field.pw8",
            "content/presets/factory/Pads/19-dawn-pad.pw8",
            "content/presets/factory/Pads/22-faded-pad.pw8",
            "content/presets/factory/Pads/25-swell-wash.pw8",
            "content/presets/factory/Pads/28-bucket-glow.pw8",
            "content/presets/factory/Pads/31-quiet-glow.pw8",
            "content/presets/factory/Pads/34-quiet-bloom.pw8",
            "content/presets/factory/Pads/37-distant-sky.pw8",
            "content/presets/factory/Pads/40-chorus-stack.pw8",
            "content/presets/factory/Pads/43-feather-bloom.pw8",
            "content/presets/factory/Pads/46-marble-hymn.pw8",
            "content/presets/factory/Pads/49-haze-stack.pw8",
            "content/presets/factory/Ambient/01-nebula-expanse.pw8",
            "content/presets/factory/Ambient/04-ash-wash.pw8",
            "content/presets/factory/Ambient/07-drift-cloud.pw8",
            "content/presets/factory/Ambient/10-sietch-drone.pw8",
            "content/presets/factory/Ambient/13-resonant-drift.pw8",
            "content/presets/factory/Ambient/16-nebula-drift.pw8",
            "content/presets/factory/Ambient/19-sietch-hymn.pw8",
            "content/presets/factory/Ambient/22-sere-echo.pw8",
            "content/presets/factory/Ambient/25-threnody-hymn.pw8",
            "content/presets/factory/Ambient/28-fen-hymn.pw8",
            "content/presets/factory/Ambient/31-ochre-expanse.pw8",
            "content/presets/factory/Ambient/34-resonant-riser.pw8",
            "content/presets/factory/Ambient/37-sietch-echo.pw8",
            "content/presets/factory/Ambient/40-fen-drone.pw8",
            "content/presets/factory/Ambient/43-vapor-echo.pw8",
            "content/presets/factory/Ambient/46-fen-drift.pw8",
            "content/presets/factory/Ambient/49-resonant-cave.pw8",
            "content/presets/factory/Sequences/01-stutter-motif.pw8",
            "content/presets/factory/Sequences/04-loom-cycle.pw8",
            "content/presets/factory/Sequences/07-glass-cycle.pw8",
            "content/presets/factory/Sequences/10-relay-step.pw8",
            "content/presets/factory/Sequences/13-motor-motor.pw8",
            "content/presets/factory/Sequences/16-stab-chime.pw8",
            "content/presets/factory/Sequences/19-stutter-loop.pw8",
            "content/presets/factory/Sequences/22-pendulum-sequence.pw8",
            "content/presets/factory/Sequences/25-chime-motor.pw8",
            "content/presets/factory/Sequences/28-ratchet-motor.pw8",
            "content/presets/factory/Sequences/31-gate-step.pw8",
            "content/presets/factory/Sequences/34-tumble-sequence.pw8",
            "content/presets/factory/Sequences/37-accent-step.pw8",
            "content/presets/factory/Sequences/40-ratchet-sequence.pw8",
            "content/presets/factory/Sequences/43-tine-step.pw8",
            "content/presets/factory/Sequences/46-chime-step.pw8",
            "content/presets/factory/Sequences/49-loom-run.pw8",
        };
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
