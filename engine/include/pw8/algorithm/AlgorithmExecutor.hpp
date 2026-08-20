#pragma once

#include <array>

#include "pw8/algorithm/AlgorithmGraphCompiler.hpp"
#include "pw8/core/Types.hpp"
#include "pw8/dsp/Math.hpp"
#include "pw8/filter/StateVariableFilter.hpp"
#include "pw8/operator/OperatorNode.hpp"

// Executes a CompiledAlgorithm, one sample at a time, over a voice's 8 operator nodes.
// This is the only code that runs on the audio thread for algorithm graph processing --
// everything upstream (definition -> compile) happens on the UI/background thread.
//
// Edge semantics (MVP -- see docs/ALGORITHM_GRAPH.md for the full writeup and known
// simplifications):
//   AUDIO          destination's output += source's finished output * amount
//   PHASE_MOD      destination oscillator's phase is offset by source's output * amount
//   FREQUENCY_MOD  destination oscillator's frequency is offset (Hz) by source's output * amount
//   AMPLITUDE_MOD  destination's output *= (1 + source's output * amount)
//   RING_MOD       destination's output *= (source's output * amount)  [multiplies with AMPLITUDE_MOD if both present]
//   SYNC           destination oscillator's phase is reset to 0 the sample after source wraps
//   FEEDBACK       always one-sample-delayed, soft-saturated, applied as phase modulation
//
// AMPLITUDE_MOD and RING_MOD are applied to a node's own output before that output is
// used by any downstream edge, so shaping composes the way a patcher would expect.

namespace pw8::algorithm
{
    class AlgorithmExecutor
    {
    public:
        [[nodiscard]] float processSample(const CompiledAlgorithm& compiled,
                                           std::array<op::OperatorParams, core::kNodesPerLayer>& params,
                                           std::array<op::OperatorState, core::kNodesPerLayer>& states,
                                           const std::array<const oscillator::WavetableTable*, core::kNodesPerLayer>& wavetableTables,
                                           float baseFrequencyHz,
                                           const std::array<filter::FilterParams, core::kNodesPerLayer>& operatorFilterParams,
                                           std::array<filter::StateVariableFilter, core::kNodesPerLayer>& operatorFilters,
                                           const std::array<float, core::kNodesPerLayer>& operatorFilterCutoffSemitones,
                                           const std::array<float, core::kNodesPerLayer>& operatorFilterResonanceOffset,
                                           float externalSampleL = 0.0f, float externalSampleR = 0.0f,
                                           int nonlinearOsFactor = 1,
                                           std::array<float, core::kNodesPerLayer>* operatorPeakScratch = nullptr) noexcept
        {
            if (!compiled.isValid)
                return 0.0f;

            if (operatorPeakScratch != nullptr)
                operatorPeakScratch->fill(0.0f);

            std::array<float, core::kNodesPerLayer> phaseModAcc{};
            std::array<float, core::kNodesPerLayer> freqModAcc{};
            std::array<float, core::kNodesPerLayer> ampMulAcc{};
            std::array<float, core::kNodesPerLayer> ringMulAcc{};
            std::array<float, core::kNodesPerLayer> audioAcc{};
            std::array<bool, core::kNodesPerLayer> pendingSyncReset{};

            phaseModAcc.fill(0.0f);
            freqModAcc.fill(0.0f);
            ampMulAcc.fill(1.0f);
            ringMulAcc.fill(1.0f);
            audioAcc.fill(0.0f);
            pendingSyncReset.fill(false);

            // Apply feedback edges first: always use the PREVIOUS sample's output, so no
            // same-sample cycle is ever formed even for a node feeding back into itself.
            for (const auto& edge : compiled.feedbackEdges)
            {
                const auto src = edge.source.get();
                const auto dst = edge.destination.get();
                const float prev = states[src].lastOutput;
                const float prevPrev = states[src].prevLastOutput;
                const float fed = dsp::tanhFeedbackOs(prev, prevPrev, edge.amount, nonlinearOsFactor);
                phaseModAcc[dst] += fed;
            }

            std::array<float, core::kNodesPerLayer> finalOutputs{};
            finalOutputs.fill(0.0f);

            for (const auto nodeId : compiled.executionOrder)
            {
                const auto i = nodeId.get();

                if (pendingSyncReset[i])
                    states[i].reset(0.0f);

                const float raw = states[i].render(params[i], wavetableTables[i], baseFrequencyHz,
                                                     phaseModAcc[i], freqModAcc[i], externalSampleL, externalSampleR,
                                                     nonlinearOsFactor);

                const float ampMul = dsp::clamp(dsp::flushIfNotFinite(ampMulAcc[i]), 0.0f, 32.0f);
                const float ringMul = dsp::clamp(dsp::flushIfNotFinite(ringMulAcc[i]), -32.0f, 32.0f);
                float shaped = raw * ampMul * ringMul;
                shaped = dsp::clamp(dsp::flushIfNotFinite(shaped), -16.0f, 16.0f);

                if (operatorFilterParams[i].enabled)
                {
                    const float keyTrackFactor =
                        dsp::filterKeyTrackFactor(baseFrequencyHz, operatorFilterParams[i].keyTrack);
                    // operatorFilterCutoffSemitones[i] is an exact 0.0f whenever no active
                    // route targets this operator's filter cutoff -- skip pow() then.
                    const float cutoffModMultiplier = operatorFilterCutoffSemitones[i] != 0.0f
                                                           ? std::pow(2.0f, operatorFilterCutoffSemitones[i] / 12.0f)
                                                           : 1.0f;
                    const float cutoffHz =
                        operatorFilterParams[i].cutoffHz * keyTrackFactor * cutoffModMultiplier;
                    const float resonance =
                        dsp::clamp(operatorFilterParams[i].resonance + operatorFilterResonanceOffset[i], 0.0f, 1.0f);
                    shaped = operatorFilters[i].renderSample(shaped, operatorFilterParams[i].mode,
                                                             operatorFilterParams[i].modeMorph, cutoffHz, resonance);
                }

                const float finalOut = dsp::clamp(dsp::flushIfNotFinite(shaped + audioAcc[i]), -16.0f, 16.0f);
                states[i].prevLastOutput = states[i].lastOutput;
                finalOutputs[i] = finalOut;
                states[i].lastOutput = finalOut;

                if (operatorPeakScratch != nullptr)
                {
                    const float peak = std::abs(finalOut);
                    if (peak > (*operatorPeakScratch)[i])
                        (*operatorPeakScratch)[i] = peak;
                }

                // Propagate this node's finished output along its outgoing feed-forward edges.
                for (const auto& edge : compiled.feedForwardEdges)
                {
                    if (edge.source.get() != i)
                        continue;

                    const auto d = edge.destination.get();
                    switch (edge.type)
                    {
                        case EdgeType::Audio:
                            audioAcc[d] += finalOut * edge.amount;
                            break;
                        case EdgeType::PhaseMod:
                            phaseModAcc[d] += finalOut * edge.amount;
                            break;
                        case EdgeType::FrequencyMod:
                            freqModAcc[d] += finalOut * edge.amount;
                            break;
                        case EdgeType::AmplitudeMod:
                            ampMulAcc[d] *= (1.0f + finalOut * edge.amount);
                            break;
                        case EdgeType::RingMod:
                            ringMulAcc[d] *= (finalOut * edge.amount);
                            break;
                        case EdgeType::Sync:
                            if (states[i].didWrapThisSample(params[i].engine))
                                pendingSyncReset[d] = true;
                            break;
                        case EdgeType::Feedback:
                            break; // handled separately, above, before this loop.
                    }
                }
            }

            float output = 0.0f;
            for (const auto nodeId : compiled.outputNodes)
            {
                if (compiled.externalOp0DirectOutput && nodeId.get() == 0)
                    continue;
                output += finalOutputs[nodeId.get()];
            }

            return dsp::clamp(dsp::flushIfNotFinite(output), -16.0f, 16.0f);
        }
    };

} // namespace pw8::algorithm
