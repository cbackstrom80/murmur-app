#pragma once

#include <cstdint>

#include "pw8/core/Types.hpp"

// Data-driven description of an 8-node algorithm graph. This is the type that gets
// loaded from a .pw8 patch / content/algorithms/*.json template. It is NOT executable
// directly -- AlgorithmGraphCompiler turns it into a CompiledAlgorithm first.

namespace pw8::algorithm
{
    /// Which synthesis engine a node runs. Only Classic (and a minimal Wavetable) are
    /// implemented in this pass; the rest render silence and are reserved for later phases.
    enum class EngineType : std::uint8_t
    {
        Classic = 0,
        Wavetable = 1,
        FmPm = 2,
        Additive = 3,
        PhaseShape = 4,
        Granular = 5,
        NoiseChaos = 6,
        Resonator = 7,
    };

    [[nodiscard]] constexpr bool isEngineImplemented(EngineType type) noexcept
    {
        return type == EngineType::Classic || type == EngineType::Wavetable || type == EngineType::PhaseShape;
    }

    enum class EdgeType : std::uint8_t
    {
        Audio = 0,
        PhaseMod = 1,
        FrequencyMod = 2,
        AmplitudeMod = 3,
        RingMod = 4,
        Sync = 5,
        Feedback = 6,
    };

    struct AlgorithmNode
    {
        core::NodeId id{};
        EngineType engine = EngineType::Classic;
        /// Whether this node's rendered output is summed into the layer's audio output.
        bool isOutput = false;
    };

    struct AlgorithmEdge
    {
        core::NodeId source{};
        core::NodeId destination{};
        EdgeType type = EdgeType::Audio;
        /// Modulation depth / mix amount. Semantics depend on `type`:
        ///   Audio         -- linear mix gain
        ///   PhaseMod      -- phase offset depth in cycles
        ///   FrequencyMod  -- frequency offset depth in Hz
        ///   AmplitudeMod  -- (1 + amount * modulator) multiplier
        ///   RingMod       -- direct bipolar multiply, `amount` scales modulator first
        ///   Sync          -- 0 or nonzero enables sync-on-wrap
        ///   Feedback      -- phase-feedback depth in cycles, always one-sample delayed
        float amount = 1.0f;
    };

    struct AlgorithmGraphDefinition
    {
        core::FixedVector<AlgorithmNode, core::kNodesPerLayer> nodes;
        core::FixedVector<AlgorithmEdge, core::kMaxAlgorithmEdges> edges;

        [[nodiscard]] static AlgorithmGraphDefinition makeDefaultParallel8() noexcept
        {
            AlgorithmGraphDefinition def;
            for (std::uint8_t i = 0; i < core::kNodesPerLayer; ++i)
                def.nodes.push_back(AlgorithmNode{core::NodeId(i), EngineType::Classic, /*isOutput=*/i == 0});
            return def;
        }
    };

} // namespace pw8::algorithm
