// pw8-graph -- developer tool for inspecting a patch's algorithm graph before any
// graphical UI exists.
//
//   pw8-graph inspect content/presets/fm-bell.pw8

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/patch/PatchSerializer.hpp"

namespace
{
    const char* engineName(pw8::algorithm::EngineType e)
    {
        using pw8::algorithm::EngineType;
        switch (e)
        {
            case EngineType::Classic: return "classic";
            case EngineType::Wavetable: return "wavetable";
            case EngineType::FmPm: return "fm_pm";
            case EngineType::Additive: return "additive";
            case EngineType::PhaseShape: return "phase_shape";
            case EngineType::Granular: return "granular";
            case EngineType::NoiseChaos: return "noise_chaos";
            case EngineType::Resonator: return "resonator";
        }
        return "?";
    }

    const char* edgeName(pw8::algorithm::EdgeType t)
    {
        using pw8::algorithm::EdgeType;
        switch (t)
        {
            case EdgeType::Audio: return "AUDIO";
            case EdgeType::PhaseMod: return "PM";
            case EdgeType::FrequencyMod: return "FM";
            case EdgeType::AmplitudeMod: return "AM";
            case EdgeType::RingMod: return "RM";
            case EdgeType::Sync: return "SYNC";
            case EdgeType::Feedback: return "FB";
        }
        return "?";
    }

    void inspectLayer(const char* layerName, const pw8::patch::LayerPatch& layer)
    {
        pw8::algorithm::CompiledAlgorithm compiled;
        const auto status = pw8::algorithm::AlgorithmGraphCompiler::compile(layer.algorithm, compiled);

        std::cout << "Layer " << layerName << ":\n";
        if (status != pw8::algorithm::CompileStatus::Ok)
        {
            std::cout << "  ** COMPILE FAILED: " << pw8::algorithm::toString(status) << " **\n\n";
            return;
        }

        std::cout << "  Nodes (execution order):\n";
        for (const auto nodeId : compiled.executionOrder)
        {
            const auto idx = nodeId.get();
            std::cout << "    OP" << static_cast<int>(idx) << " [" << engineName(compiled.nodeEngines[idx]) << "]";

            bool isOutput = false;
            for (const auto out : compiled.outputNodes)
                if (out.get() == idx)
                    isOutput = true;
            if (isOutput)
                std::cout << "  -> OUT";
            std::cout << "\n";
        }

        std::cout << "  Feed-forward edges:\n";
        if (compiled.feedForwardEdges.empty())
            std::cout << "    (none)\n";
        for (const auto& e : compiled.feedForwardEdges)
        {
            std::cout << "    OP" << static_cast<int>(e.source.get()) << " --" << edgeName(e.type) << "("
                      << e.amount << ")--> OP" << static_cast<int>(e.destination.get()) << "\n";
        }

        std::cout << "  Feedback edges:\n";
        if (compiled.feedbackEdges.empty())
            std::cout << "    (none)\n";
        for (const auto& e : compiled.feedbackEdges)
        {
            std::cout << "    OP" << static_cast<int>(e.source.get()) << " ==" << edgeName(e.type) << "("
                      << e.amount << ")==> OP" << static_cast<int>(e.destination.get()) << "  [one-sample delayed]\n";
        }
        std::cout << "\n";
    }
} // namespace

int main(int argc, char** argv)
{
    if (argc < 3 || std::string(argv[1]) != "inspect")
    {
        std::cerr << "Usage: pw8-graph inspect <preset.pw8>\n";
        return 2;
    }

    std::ifstream f(argv[2], std::ios::binary);
    if (!f.is_open())
    {
        std::cerr << "Could not open: " << argv[2] << "\n";
        return 1;
    }
    std::ostringstream ss;
    ss << f.rdbuf();

    const auto result = pw8::patch::loadPatchFromJson(ss.str());
    if (!result.ok)
    {
        std::cerr << "Failed to parse patch: " << result.error << "\n";
        return 1;
    }

    std::cout << "Patch: " << result.patch.metadata.name << " (" << result.patch.metadata.category << ")\n";
    std::cout << "========================================\n\n";

    inspectLayer("A", result.patch.layerA);
    inspectLayer("B", result.patch.layerB);

    return 0;
}
