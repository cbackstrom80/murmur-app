// pybind11 bindings for pw8_core.
//
//   import patchwork_eight as pw8
//
//   patch = pw8.Patch.load("content/presets/dark-bass.pw8")
//   patch.layer_a.operator(0).engine = "wavetable"
//
//   engine = pw8.Engine(sample_rate=48000)
//   engine.load_patch(patch)
//   result = engine.render(midi="content/test_midi/bass-line.mid", bpm=105, quality="offline")
//   print(result["metrics"]["peak"], len(result["audio"]))
//
// Status: PARTIAL against the full desired API in docs/PYTHON_API.md -- covers patch
// load/save/inspect, per-operator editing on both layers, and file-driven offline
// rendering. Live note_on/note_off streaming and numpy zero-copy buffers are PLANNED.

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "pw8/core/AudioBlock.hpp"
#include "pw8/core/Version.hpp"
#include "pw8/midi/StandardMidiFile.hpp"
#include "pw8/patch/Patch.hpp"
#include "pw8/patch/PatchSerializer.hpp"
#include "pw8/render/Engine.hpp"
#include "pw8/render/Renderer.hpp"

namespace py = pybind11;
using namespace pw8;

namespace
{
    std::string readFile(const std::string& path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open())
            throw std::runtime_error("Could not open file: " + path);
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    std::vector<std::uint8_t> readFileBytes(const std::string& path)
    {
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open())
            throw std::runtime_error("Could not open file: " + path);
        return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
    }

    algorithm::EngineType engineFromString(const std::string& s)
    {
        if (s == "classic") return algorithm::EngineType::Classic;
        if (s == "wavetable") return algorithm::EngineType::Wavetable;
        if (s == "fm_pm") return algorithm::EngineType::FmPm;
        if (s == "additive") return algorithm::EngineType::Additive;
        if (s == "phase_shape") return algorithm::EngineType::PhaseShape;
        if (s == "granular") return algorithm::EngineType::Granular;
        if (s == "noise_chaos") return algorithm::EngineType::NoiseChaos;
        if (s == "resonator") return algorithm::EngineType::Resonator;
        throw std::invalid_argument("Unknown engine type: " + s);
    }

    std::string engineToString(algorithm::EngineType e)
    {
        switch (e)
        {
            case algorithm::EngineType::Classic: return "classic";
            case algorithm::EngineType::Wavetable: return "wavetable";
            case algorithm::EngineType::FmPm: return "fm_pm";
            case algorithm::EngineType::Additive: return "additive";
            case algorithm::EngineType::PhaseShape: return "phase_shape";
            case algorithm::EngineType::Granular: return "granular";
            case algorithm::EngineType::NoiseChaos: return "noise_chaos";
            case algorithm::EngineType::Resonator: return "resonator";
        }
        return "unknown";
    }

    oscillator::ClassicWaveform waveformFromString(const std::string& s)
    {
        if (s == "sine") return oscillator::ClassicWaveform::Sine;
        if (s == "triangle") return oscillator::ClassicWaveform::Triangle;
        if (s == "saw") return oscillator::ClassicWaveform::Saw;
        if (s == "square") return oscillator::ClassicWaveform::Square;
        throw std::invalid_argument("Unknown classic waveform: " + s);
    }

    std::string waveformToString(oscillator::ClassicWaveform w)
    {
        switch (w)
        {
            case oscillator::ClassicWaveform::Sine: return "sine";
            case oscillator::ClassicWaveform::Triangle: return "triangle";
            case oscillator::ClassicWaveform::Saw: return "saw";
            case oscillator::ClassicWaveform::Square: return "square";
        }
        return "unknown";
    }

    render::QualityMode qualityFromString(const std::string& s)
    {
        if (s == "eco") return render::QualityMode::Eco;
        if (s == "normal") return render::QualityMode::Normal;
        if (s == "high") return render::QualityMode::High;
        if (s == "ultra") return render::QualityMode::Ultra;
        if (s == "offline") return render::QualityMode::Offline;
        throw std::invalid_argument("Unknown quality mode: " + s);
    }

    /// Non-owning proxy for a single operator slot. Kept alive by pybind11's
    /// keep_alive<> policy tying its lifetime to the owning Patch object.
    struct PyOperator
    {
        patch::OperatorPatch* op;
    };

    /// Non-owning proxy for a layer. Flattens the amplitude envelope onto the layer
    /// for a shorter API surface in this pass (a dedicated Envelope proxy is PLANNED
    /// once modulation-system bindings land).
    struct PyLayer
    {
        patch::LayerPatch* layer;
    };

    struct PyPatch
    {
        patch::Patch data = patch::Patch::makeInit();
    };

} // namespace

PYBIND11_MODULE(patchwork_eight, m)
{
    m.doc() = "Patchwork Eight -- pw8_core Python bindings (PARTIAL, see docs/PYTHON_API.md)";
    m.attr("__engine_version__") = std::string(core::EngineVersion::string());
    m.attr("__patch_schema_version__") = core::kPatchSchemaVersion;

    py::class_<PyOperator>(m, "Operator")
        .def_property(
            "engine", [](const PyOperator& o) { return engineToString(o.op->engine); },
            [](PyOperator& o, const std::string& v) { o.op->engine = engineFromString(v); })
        .def_property(
            "classic_waveform", [](const PyOperator& o) { return waveformToString(o.op->classicWaveform); },
            [](PyOperator& o, const std::string& v) { o.op->classicWaveform = waveformFromString(v); })
        .def_property(
            "classic_morph", [](const PyOperator& o) { return o.op->classicMorph; },
            [](PyOperator& o, float v) { o.op->classicMorph = v; })
        .def_property(
            "frequency_ratio", [](const PyOperator& o) { return o.op->frequencyRatio; },
            [](PyOperator& o, float v) { o.op->frequencyRatio = v; })
        .def_property(
            "fixed_frequency_hz", [](const PyOperator& o) { return o.op->fixedFrequencyHz; },
            [](PyOperator& o, float v) { o.op->fixedFrequencyHz = v; })
        .def_property(
            "key_track", [](const PyOperator& o) { return o.op->keyTrack; },
            [](PyOperator& o, bool v) { o.op->keyTrack = v; })
        .def_property(
            "level", [](const PyOperator& o) { return o.op->level; },
            [](PyOperator& o, float v) { o.op->level = v; });

    py::class_<PyLayer>(m, "Layer")
        .def(
            "operator", [](PyLayer& self, int index) {
                if (index < 0 || index >= static_cast<int>(core::kNodesPerLayer))
                    throw std::out_of_range("operator index must be 0..7");
                return PyOperator{&self.layer->operators[static_cast<std::size_t>(index)]};
            },
            py::arg("index"), py::keep_alive<0, 1>())
        .def_property(
            "gain", [](const PyLayer& l) { return l.layer->gain; },
            [](PyLayer& l, float v) { l.layer->gain = v; })
        .def_property(
            "pan", [](const PyLayer& l) { return l.layer->pan; }, [](PyLayer& l, float v) { l.layer->pan = v; })
        .def_property(
            "attack", [](const PyLayer& l) { return l.layer->ampEnvelope.attackSeconds; },
            [](PyLayer& l, float v) { l.layer->ampEnvelope.attackSeconds = v; })
        .def_property(
            "decay", [](const PyLayer& l) { return l.layer->ampEnvelope.decaySeconds; },
            [](PyLayer& l, float v) { l.layer->ampEnvelope.decaySeconds = v; })
        .def_property(
            "sustain", [](const PyLayer& l) { return l.layer->ampEnvelope.sustainLevel; },
            [](PyLayer& l, float v) { l.layer->ampEnvelope.sustainLevel = v; })
        .def_property(
            "release", [](const PyLayer& l) { return l.layer->ampEnvelope.releaseSeconds; },
            [](PyLayer& l, float v) { l.layer->ampEnvelope.releaseSeconds = v; });

    py::class_<PyPatch>(m, "Patch")
        .def(py::init<>())
        .def_static(
            "load", [](const std::string& path) {
                PyPatch p;
                const auto result = patch::loadPatchFromJson(readFile(path));
                if (!result.ok)
                    throw std::runtime_error("Failed to load patch: " + result.error);
                p.data = result.patch;
                return p;
            },
            py::arg("path"))
        .def_static(
            "from_json", [](const std::string& jsonText) {
                PyPatch p;
                const auto result = patch::loadPatchFromJson(jsonText);
                if (!result.ok)
                    throw std::runtime_error("Failed to parse patch: " + result.error);
                p.data = result.patch;
                return p;
            },
            py::arg("json_text"))
        .def(
            "to_json", [](const PyPatch& p, int indent) { return patch::savePatchToJson(p.data, indent); },
            py::arg("indent") = 2)
        .def(
            "save", [](const PyPatch& p, const std::string& path) {
                std::ofstream f(path);
                if (!f.is_open())
                    throw std::runtime_error("Could not open output file: " + path);
                f << patch::savePatchToJson(p.data);
            },
            py::arg("path"))
        .def_property(
            "name", [](const PyPatch& p) { return p.data.metadata.name; },
            [](PyPatch& p, const std::string& v) { p.data.metadata.name = v; })
        .def_property(
            "category", [](const PyPatch& p) { return p.data.metadata.category; },
            [](PyPatch& p, const std::string& v) { p.data.metadata.category = v; })
        .def_property(
            "seed", [](const PyPatch& p) { return p.data.seed; },
            [](PyPatch& p, std::uint64_t v) { p.data.seed = v; })
        .def_property_readonly(
            "layer_a", [](PyPatch& p) { return PyLayer{&p.data.layerA}; }, py::keep_alive<0, 1>())
        .def_property_readonly(
            "layer_b", [](PyPatch& p) { return PyLayer{&p.data.layerB}; }, py::keep_alive<0, 1>());

    py::class_<render::Engine>(m, "Engine")
        .def(py::init([](double sampleRate) {
                 auto e = std::make_unique<render::Engine>();
                 e->prepare(sampleRate);
                 return e;
             }),
             py::arg("sample_rate") = 48000.0)
        .def("load_patch", [](render::Engine& e, PyPatch& p) { return e.loadPatch(p.data); }, py::arg("patch"))
        .def("note_on", &render::Engine::noteOn, py::arg("note"), py::arg("channel") = 0, py::arg("velocity") = 100)
        .def("note_off", &render::Engine::noteOff, py::arg("note"), py::arg("channel") = 0, py::arg("velocity") = 0)
        .def("all_notes_off", &render::Engine::allNotesOff)
        .def(
            "process", [](render::Engine& e, int numFrames) {
                std::vector<float> left(static_cast<std::size_t>(numFrames));
                std::vector<float> right(static_cast<std::size_t>(numFrames));
                core::StereoBlockView view(left.data(), right.data(), static_cast<std::size_t>(numFrames));
                e.process(view);
                std::vector<float> interleaved(static_cast<std::size_t>(numFrames) * 2);
                for (int i = 0; i < numFrames; ++i)
                {
                    interleaved[static_cast<std::size_t>(i) * 2] = left[static_cast<std::size_t>(i)];
                    interleaved[static_cast<std::size_t>(i) * 2 + 1] = right[static_cast<std::size_t>(i)];
                }
                return interleaved;
            },
            py::arg("num_frames"),
            "Renders num_frames of audio from the currently loaded patch's live state "
            "(after note_on/note_off calls) and returns it as an interleaved [L, R, L, R, ...] list.")
        .def(
            "render", [](render::Engine& e, const std::string& midiPath, double bpm, double durationSeconds,
                          const std::string& quality, std::uint64_t seed) {
                const auto smf = midi::readStandardMidiFile(readFileBytes(midiPath));
                if (!smf.ok)
                    throw std::runtime_error("Failed to parse MIDI file: " + smf.error);

                render::RenderOptions options;
                options.sampleRate = e.getSampleRate();
                options.bpm = bpm;
                options.durationSecondsOverride = durationSeconds;
                options.quality = qualityFromString(quality);
                options.seed = seed;

                const auto result = render::renderWithEngine(e, smf.sequence, options);
                if (!result.ok)
                    throw std::runtime_error("Render failed: " + result.error);

                py::dict out;
                out["audio"] = result.interleavedStereo; // interleaved L, R, L, R, ...
                out["sample_rate"] = result.sampleRate;
                py::dict metrics;
                metrics["peak"] = result.metrics.peak;
                metrics["rms"] = result.metrics.rms;
                metrics["dc_offset_left"] = result.metrics.dcOffsetLeft;
                metrics["dc_offset_right"] = result.metrics.dcOffsetRight;
                metrics["contains_nan_or_inf"] = result.metrics.containsNaNOrInf;
                metrics["duration_seconds"] = result.metrics.durationSeconds;
                metrics["left_right_balance"] = result.metrics.leftRightBalance;
                out["metrics"] = metrics;
                return out;
            },
            py::arg("midi"), py::arg("bpm") = 120.0, py::arg("duration") = -1.0, py::arg("quality") = "offline",
            py::arg("seed") = 0,
            "Renders the currently loaded patch against a MIDI file at this engine's sample rate. "
            "Call load_patch() first.");

    m.def(
        "render", [](PyPatch& patchObj, const std::string& midiPath, double sampleRate, double bpm,
                      double durationSeconds, const std::string& quality, std::uint64_t seed) {
            const auto smf = midi::readStandardMidiFile(readFileBytes(midiPath));
            if (!smf.ok)
                throw std::runtime_error("Failed to parse MIDI file: " + smf.error);

            render::RenderOptions options;
            options.sampleRate = sampleRate;
            options.bpm = bpm;
            options.durationSecondsOverride = durationSeconds;
            options.quality = qualityFromString(quality);
            options.seed = seed;

            const auto result = render::render(patchObj.data, smf.sequence, options);
            if (!result.ok)
                throw std::runtime_error("Render failed: " + result.error);

            py::dict out;
            out["audio"] = result.interleavedStereo;
            out["sample_rate"] = result.sampleRate;
            py::dict metrics;
            metrics["peak"] = result.metrics.peak;
            metrics["rms"] = result.metrics.rms;
            metrics["dc_offset_left"] = result.metrics.dcOffsetLeft;
            metrics["dc_offset_right"] = result.metrics.dcOffsetRight;
            metrics["contains_nan_or_inf"] = result.metrics.containsNaNOrInf;
            metrics["duration_seconds"] = result.metrics.durationSeconds;
            metrics["left_right_balance"] = result.metrics.leftRightBalance;
            out["metrics"] = metrics;
            return out;
        },
        py::arg("patch"), py::arg("midi"), py::arg("sample_rate") = 48000.0, py::arg("bpm") = 120.0,
        py::arg("duration") = -1.0, py::arg("quality") = "offline", py::arg("seed") = 0,
        "Stateless one-shot render: builds a fresh Engine internally, loads `patch`, and renders `midi`.");
}
