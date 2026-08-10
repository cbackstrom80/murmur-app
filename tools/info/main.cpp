// pw8-info -- prints engine build/version information.

#include <iostream>

#include "pw8/core/Types.hpp"
#include "pw8/core/Version.hpp"

int main()
{
    using namespace pw8;

    std::cout << "Patchwork Eight -- pw8-info\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Engine version:          " << core::EngineVersion::string() << "\n";
    std::cout << "Patch schema version:    " << core::kPatchSchemaVersion << "\n";
    std::cout << "Algorithm schema version:" << " " << core::kAlgorithmSchemaVersion << "\n";
    std::cout << "\n";
    std::cout << "Supported sample rates:  44100, 48000, 88200, 96000 (44.1k-384k accepted)\n";
    std::cout << "Nodes per layer:         " << core::kNodesPerLayer << "\n";
    std::cout << "Layers per patch:        " << core::kLayerCount << "\n";
    std::cout << "Default polyphony:       " << core::kDefaultVoices << "\n";
    std::cout << "Max polyphony:           " << core::kMaxVoices << "\n";
    std::cout << "Max algorithm edges:     " << core::kMaxAlgorithmEdges << "\n";
    std::cout << "\n";
    std::cout << "Engine types:\n";
    std::cout << "  [IMPLEMENTED] classic     -- PolyBLEP band-limited sine/tri/saw/square/pulse\n";
    std::cout << "  [PARTIAL]     wavetable   -- multi-frame linear interpolation, no mip-mapping yet\n";
    std::cout << "  [PLANNED]     fm_pm       -- dedicated phase-modulation engine (algorithm graph PM edges work today)\n";
    std::cout << "  [PLANNED]     additive\n";
    std::cout << "  [PLANNED]     phase_shape\n";
    std::cout << "  [PLANNED]     granular\n";
    std::cout << "  [PLANNED]     noise_chaos\n";
    std::cout << "  [PLANNED]     resonator\n";
    std::cout << "\n";
    std::cout << "Algorithm graph edge types: AUDIO, PHASE_MOD, FREQUENCY_MOD, AMPLITUDE_MOD, RING_MOD, SYNC, FEEDBACK\n";
    std::cout << "\n";
#if defined(NDEBUG)
    std::cout << "Build configuration:     Release\n";
#else
    std::cout << "Build configuration:     Debug\n";
#endif
    std::cout << "C++ standard:            " << __cplusplus << "\n";

    return 0;
}
