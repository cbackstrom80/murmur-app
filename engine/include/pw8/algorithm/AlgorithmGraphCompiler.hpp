#pragma once

#include <array>
#include <cstdint>

#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/core/Types.hpp"

// AlgorithmGraphCompiler turns a user/AI-editable AlgorithmGraphDefinition into a
// CompiledAlgorithm the audio thread can execute with no validation, no allocation,
// and no risk of an unbounded/zero-delay cycle. This is the ONLY path by which a
// graph reaches the audio thread -- see docs/ALGORITHM_GRAPH.md.

namespace pw8::algorithm
{
    enum class CompileStatus : std::uint8_t
    {
        Ok = 0,
        WrongNodeCount,
        DuplicateNodeId,
        InvalidNodeId,
        InvalidEdgeReference,
        TooManyEdges,
        FeedForwardCycle,
        NoOutputNodes,
    };

    [[nodiscard]] const char* toString(CompileStatus status) noexcept;

    struct CompiledEdge
    {
        core::NodeId source{};
        core::NodeId destination{};
        EdgeType type = EdgeType::Audio;
        float amount = 0.0f;
    };

    /// Precompiled, audio-thread-safe representation of an algorithm graph.
    /// Immutable once produced. The audio thread only ever reads from this.
    class CompiledAlgorithm
    {
    public:
        core::FixedVector<core::NodeId, core::kNodesPerLayer> executionOrder;
        core::FixedVector<CompiledEdge, core::kMaxAlgorithmEdges> feedForwardEdges;
        core::FixedVector<CompiledEdge, core::kMaxAlgorithmEdges> feedbackEdges;
        core::FixedVector<core::NodeId, core::kNodesPerLayer> outputNodes;
        std::array<EngineType, core::kNodesPerLayer> nodeEngines{};
        bool isValid = false;
    };

    class AlgorithmGraphCompiler
    {
    public:
        /// Validates and compiles `definition` into `out`. Returns Ok on success;
        /// on failure `out` is left default-constructed (isValid == false) and the
        /// caller should fall back to a known-safe algorithm (e.g. single sine node).
        [[nodiscard]] static CompileStatus compile(const AlgorithmGraphDefinition& definition,
                                                     CompiledAlgorithm& out) noexcept;
    };

} // namespace pw8::algorithm
