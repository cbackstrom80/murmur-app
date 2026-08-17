#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"

#include <array>
#include <cmath>

namespace pw8::algorithm
{
    const char* toString(CompileStatus status) noexcept
    {
        switch (status)
        {
            case CompileStatus::Ok: return "Ok";
            case CompileStatus::WrongNodeCount: return "WrongNodeCount";
            case CompileStatus::DuplicateNodeId: return "DuplicateNodeId";
            case CompileStatus::InvalidNodeId: return "InvalidNodeId";
            case CompileStatus::InvalidEdgeReference: return "InvalidEdgeReference";
            case CompileStatus::TooManyEdges: return "TooManyEdges";
            case CompileStatus::FeedForwardCycle: return "FeedForwardCycle";
            case CompileStatus::NoOutputNodes: return "NoOutputNodes";
        }
        return "Unknown";
    }

    namespace
    {
        constexpr float kZeroEdgeThreshold = 1.0e-6f;

        [[nodiscard]] bool isFeedbackType(EdgeType type) noexcept
        {
            return type == EdgeType::Feedback;
        }
    } // namespace

    CompileStatus AlgorithmGraphCompiler::compile(const AlgorithmGraphDefinition& definition,
                                                    CompiledAlgorithm& out) noexcept
    {
        out = CompiledAlgorithm{};

        if (definition.nodes.size() != core::kNodesPerLayer)
            return CompileStatus::WrongNodeCount;

        // --- Validate node IDs: must be exactly {0, 1, ..., 7}, no duplicates. ---
        std::array<bool, core::kNodesPerLayer> seenNode{};
        seenNode.fill(false);
        std::array<EngineType, core::kNodesPerLayer> engineByNode{};

        for (const auto& node : definition.nodes)
        {
            const auto idx = node.id.get();
            if (!node.id.isValid() || idx >= core::kNodesPerLayer)
                return CompileStatus::InvalidNodeId;
            if (seenNode[idx])
                return CompileStatus::DuplicateNodeId;
            seenNode[idx] = true;
            engineByNode[idx] = node.engine;
        }

        if (definition.edges.size() > core::kMaxAlgorithmEdges)
            return CompileStatus::TooManyEdges;

        // --- Validate edges and split into feed-forward vs. feedback. ---
        // Build adjacency (feed-forward only) for cycle detection via Kahn's algorithm.
        std::array<std::array<float, core::kNodesPerLayer>, core::kNodesPerLayer> feedForwardAdjacency{};
        for (auto& row : feedForwardAdjacency)
            row.fill(0.0f);

        core::FixedVector<AlgorithmEdge, core::kMaxAlgorithmEdges> feedForwardSource;
        core::FixedVector<AlgorithmEdge, core::kMaxAlgorithmEdges> feedbackSource;

        for (const auto& edge : definition.edges)
        {
            const auto s = edge.source.get();
            const auto d = edge.destination.get();
            if (!edge.source.isValid() || !edge.destination.isValid() ||
                s >= core::kNodesPerLayer || d >= core::kNodesPerLayer)
                return CompileStatus::InvalidEdgeReference;

            if (std::abs(edge.amount) < kZeroEdgeThreshold)
                continue; // Zero-value edges are eliminated at compile time.

            if (isFeedbackType(edge.type))
            {
                feedbackSource.push_back(edge);
            }
            else
            {
                if (s != d)
                    feedForwardAdjacency[s][d] += 1.0f;
                feedForwardSource.push_back(edge);
            }
        }

        // --- Topological sort (Kahn's algorithm) over feed-forward edges only. ---
        std::array<int, core::kNodesPerLayer> inDegree{};
        inDegree.fill(0);
        for (std::size_t s = 0; s < core::kNodesPerLayer; ++s)
            for (std::size_t d = 0; d < core::kNodesPerLayer; ++d)
                if (feedForwardAdjacency[s][d] > 0.0f)
                    ++inDegree[d];

        core::FixedVector<core::NodeId, core::kNodesPerLayer> queue;
        for (std::uint8_t i = 0; i < core::kNodesPerLayer; ++i)
            if (inDegree[i] == 0)
                queue.push_back(core::NodeId(i));

        core::FixedVector<core::NodeId, core::kNodesPerLayer> order;
        std::size_t queueHead = 0;
        while (queueHead < queue.size())
        {
            const auto n = queue[queueHead++];
            order.push_back(n);
            for (std::uint8_t d = 0; d < core::kNodesPerLayer; ++d)
            {
                if (feedForwardAdjacency[n.get()][d] > 0.0f)
                {
                    if (--inDegree[d] == 0)
                        queue.push_back(core::NodeId(d));
                }
            }
        }

        if (order.size() != core::kNodesPerLayer)
            return CompileStatus::FeedForwardCycle;

        // --- Determine output nodes. ---
        core::FixedVector<core::NodeId, core::kNodesPerLayer> outputNodes;
        for (const auto& node : definition.nodes)
            if (node.isOutput)
                outputNodes.push_back(node.id);

        if (outputNodes.empty())
            return CompileStatus::NoOutputNodes;

        if (engineByNode[0] == EngineType::External)
        {
            for (const auto outId : outputNodes)
            {
                if (outId.get() == 0)
                {
                    out.externalOp0DirectOutput = true;
                    break;
                }
            }
        }

        // --- Build final compiled edges (execution-order-relative for feed-forward). ---
        out.executionOrder = order;
        out.outputNodes = outputNodes;
        out.nodeEngines = engineByNode;

        for (const auto& e : feedForwardSource)
            out.feedForwardEdges.push_back(CompiledEdge{e.source, e.destination, e.type, e.amount});

        for (const auto& e : feedbackSource)
        {
            // Feedback gain guard: clamp depth so self-modulation can't run away.
            constexpr float kMaxFeedbackAmount = 2.0f;
            float amount = e.amount;
            if (amount > kMaxFeedbackAmount) amount = kMaxFeedbackAmount;
            if (amount < -kMaxFeedbackAmount) amount = -kMaxFeedbackAmount;
            out.feedbackEdges.push_back(CompiledEdge{e.source, e.destination, e.type, amount});
        }

        out.isValid = true;
        return CompileStatus::Ok;
    }

} // namespace pw8::algorithm
