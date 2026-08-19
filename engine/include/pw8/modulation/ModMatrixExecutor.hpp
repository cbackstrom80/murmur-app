#pragma once

#include <array>

#include "pw8/core/Types.hpp"
#include "pw8/effects/EffectTypes.hpp"
#include "pw8/modulation/ModCurveShaping.hpp"
#include "pw8/modulation/ModMatrixTypes.hpp"

// Mod matrix execution (docs/MODULATION.md). Runs once per voice per sample inside
// pw8::voice::Voice::renderSample() -- see ModMatrixExecutor::apply(). No
// allocation: routes live in a fixed-capacity container
// (`core::FixedVector<ModRoute, core::kMaxModRoutes>` on LayerPatch), and this
// function only ever writes into a stack-allocated ModOutputs.
//
// Destination semantics:
//   FilterCutoff / OperatorFilterCutoff -- semitones of cutoff shift per unit source
//                       value; global vs per-engine selected by destination.
//   FilterResonance / OperatorFilterResonance -- additive resonance offset (0..1).
//   OperatorLevel   -- multiplicative: level *= (1 + sourceValue * amount), per
//                       route, targeting `targetIndex` (0..7). Multiple routes to
//                       the same operator compose multiplicatively.
//   Pan             -- additive offset to layer pan (-1..1), summed then clamped.
//   OperatorWavetablePosition -- additive offset to wavetable frame position
//                       (0..1), per route, targeting `targetIndex` (0..7). Only
//                       meaningful for operators on the Wavetable engine.
//   OperatorWavetableBend / Asymmetry / Formant -- additive offset (-1..1), per
//                       route, targeting `targetIndex` (0..7). Wavetable only.
//   OperatorWavetableSyncRatio -- additive offset to wt sync ratio (1..16), per
//                       route, targeting `targetIndex` (0..7). Wavetable only.
//   OperatorWavetableSyncAmount -- additive offset to wt sync blend (0..1), per
//                       route, targeting `targetIndex` (0..7). Wavetable only.
//
// Master-bus destinations (Global/Layer scope; targetIndex = master FX slot 0..3):
//   MasterFxMix / MasterReverbMix -- additive mix offset, clamped 0..1 at apply time.
//   MasterReverbSize -- additive offset to reverbSizeParam.
//   MasterReverbDecay -- additive seconds offset to reverbDecaySeconds.
//   MasterReverbPreDelay -- additive ms offset to reverbPreDelayMs.
//   MasterReverbDiffusion / MasterReverbModDepth -- additive 0..1 offsets.
//   MasterGain -- additive offset to voiceSettings.masterGain multiplier at sum stage.
//
// Scope: see ModScope's doc comment in ModMatrixTypes.hpp for the full rationale.
// In short -- LFO sources read a shared, layer-wide tick when scope is Layer or
// Global instead of the calling voice's own independent LFO; every other source
// type ignores the route's declared scope (envelopes/performance sources are
// inherently per-voice, macros are inherently global already).

namespace pw8::modulation
{
    struct ModSourceValues
    {
        std::array<float, 8> voiceLfos{}; ///< -1..1 each, this voice's own independently-phased LFOs.
        std::array<float, 8> layerLfos{}; ///< -1..1 each, one shared tick per LFO index for the whole layer.
        std::array<float, 8> envelopes{}; ///< 0..1 each; envelopes[0] is conventionally the amp envelope.
        float velocity = 0.0f;         ///< 0..1
        float channelPressure = 0.0f;  ///< 0..1
        float polyAftertouch = 0.0f;   ///< 0..1
        float mpeSlide = 0.0f;         ///< 0..1
        float modWheel = 0.0f;         ///< 0..1, MIDI CC1
        float expression = 0.0f;       ///< 0..1, MIDI CC11
        float sidechain = 0.0f;        ///< 0..1, AU sidechain envelope follower
        std::array<float, 8> macros{}; ///< 0..1 each
        /// Marbles generative outputs (layer-wide tick, -1..1).
        float randomT = 0.0f;
        float randomX = 0.0f;
        std::array<float, 4> randomOutputs{};
    };

    struct ModOutputs
    {
        float filterCutoffSemitones = 0.0f;
        float filterResonanceOffset = 0.0f;
        float filterModeMorphOffset = 0.0f;
        float filterRoutingOffset = 0.0f;
        float filterDriveOffset = 0.0f;
        std::array<float, core::kNodesPerLayer> operatorFilterCutoffSemitones{};
        std::array<float, core::kNodesPerLayer> operatorFilterResonanceOffset{};
        std::array<float, core::kNodesPerLayer> operatorLevelMultiplier{};
        std::array<float, core::kNodesPerLayer> operatorWavetablePositionOffset{};
        std::array<float, core::kNodesPerLayer> operatorWavetableBendOffset{};
        std::array<float, core::kNodesPerLayer> operatorWavetableAsymmetryOffset{};
        std::array<float, core::kNodesPerLayer> operatorWavetableSyncRatioOffset{};
        std::array<float, core::kNodesPerLayer> operatorWavetableFormantOffset{};
        std::array<float, core::kNodesPerLayer> operatorWavetableSyncAmountOffset{};
        std::array<float, core::kNodesPerLayer> operatorFmModulatorRatioOffset{};
        std::array<float, core::kNodesPerLayer> operatorFmModulatorIndexOffset{};
        std::array<float, core::kNodesPerLayer> operatorFmModulatorFeedbackOffset{};
        std::array<float, core::kNodesPerLayer> operatorFreqRatioOffset{};
        std::array<float, core::kNodesPerLayer> operatorPhaseBendOffset{};
        std::array<float, core::kNodesPerLayer> operatorPhaseFoldOffset{};
        std::array<float, core::kNodesPerLayer> operatorPhaseAsymmetryOffset{};
        std::array<float, core::kNodesPerLayer> operatorAdditivePartialCountOffset{};
        std::array<float, core::kNodesPerLayer> operatorAdditiveTiltOffset{};
        std::array<float, core::kNodesPerLayer> operatorAdditiveOddEvenOffset{};
        std::array<float, core::kNodesPerLayer> operatorAdditiveStretchOffset{};
        std::array<float, core::kNodesPerLayer> operatorResonatorStructureOffset{};
        std::array<float, core::kNodesPerLayer> operatorResonatorDecayOffset{};
        std::array<float, core::kNodesPerLayer> operatorResonatorDampingOffset{};
        std::array<float, core::kNodesPerLayer> operatorResonatorBrightnessOffset{};
        std::array<float, core::kNodesPerLayer> operatorResonatorModeCountOffset{};
        std::array<float, core::kNodesPerLayer> operatorGrainDensityOffset{};
        std::array<float, core::kNodesPerLayer> operatorGrainSizeMsOffset{};
        std::array<float, core::kNodesPerLayer> operatorGrainPositionJitterOffset{};
        std::array<float, core::kNodesPerLayer> operatorGrainPitchJitterOffset{};
        float unisonVoicesOffset = 0.0f;
        float unisonDetuneOffset = 0.0f;
        float unisonSpreadOffset = 0.0f;
        float panOffset = 0.0f;

        ModOutputs() noexcept { operatorLevelMultiplier.fill(1.0f); }
    };

    struct MasterModOutputs
    {
        std::array<float, effects::kNumMasterSlots> mixOffset{};
        std::array<float, effects::kNumMasterSlots> reverbSizeOffset{};
        std::array<float, effects::kNumMasterSlots> reverbDecayOffset{};
        std::array<float, effects::kNumMasterSlots> reverbPreDelayOffset{};
        std::array<float, effects::kNumMasterSlots> reverbDiffusionOffset{};
        std::array<float, effects::kNumMasterSlots> reverbModDepthOffset{};
        float masterGainOffset = 0.0f;
        std::array<float, effects::kNumLayerInsertSlots> vocoderMixOffset{};
        std::array<float, effects::kNumLayerInsertSlots> vocoderFormantOffset{};
        std::array<float, effects::kNumMasterSlots> masterVocoderMixOffset{};
        std::array<float, effects::kNumMasterSlots> masterVocoderFormantOffset{};
        std::array<float, effects::kNumMasterSlots> compThresholdOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr1AngleOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr2AngleOffset{};
        std::array<float, effects::kNumMasterSlots> quasarRoomAmountOffset{};
        std::array<float, effects::kNumMasterSlots> quasarCrossfeedOffset{};
        std::array<float, effects::kNumMasterSlots> quasarDelayVolumeOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr1DistanceOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr2DistanceOffset{};
        std::array<float, effects::kNumMasterSlots> quasarDelayTimeOffset{};
        std::array<float, effects::kNumMasterSlots> quasarDelayFeedbackOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr1HeightOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr2HeightOffset{};
        std::array<float, effects::kNumMasterSlots> quasarCntrLevelOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr1LevelOffset{};
        std::array<float, effects::kNumMasterSlots> quasarQsr2LevelOffset{};
        float masterDynamicsMixOffset = 0.0f;
        float sidechainDepthOffset = 0.0f;
    };

    class ModMatrixExecutor
    {
    public:
        template <typename RouteContainer>
        [[nodiscard]] static bool hasActiveVoiceLfoRoutes(const RouteContainer& routes) noexcept
        {
            for (const auto& route : routes)
            {
                if (!route.isActive() || route.scope != ModScope::Voice)
                    continue;
                if (route.source >= ModSource::Lfo1 && route.source <= ModSource::Lfo8)
                    return true;
            }
            return false;
        }

        template <typename RouteContainer>
        [[nodiscard]] static bool hasActiveLayerLfoRoutes(const RouteContainer& routes) noexcept
        {
            for (const auto& route : routes)
            {
                if (!route.isActive() || route.scope == ModScope::Voice)
                    continue;
                if (route.source >= ModSource::Lfo1 && route.source <= ModSource::Lfo8)
                    return true;
            }
            return false;
        }

        template <typename RouteContainer>
        [[nodiscard]] static bool hasActiveGenerativeRoutes(const RouteContainer& routes) noexcept
        {
            for (const auto& route : routes)
            {
                if (!route.isActive())
                    continue;
                if (route.source >= ModSource::Random1 && route.source <= ModSource::RandomX)
                    return true;
            }
            return false;
        }

        template <typename RouteContainer>
        [[nodiscard]] static bool hasAudioRateModRoutes(const RouteContainer& routes) noexcept
        {
            for (const auto& route : routes)
            {
                if (!route.isActive())
                    continue;
                if (route.source >= ModSource::Lfo1 && route.source <= ModSource::Lfo8)
                    return true;
                if (route.source >= ModSource::Env1 && route.source <= ModSource::Env8)
                    return true;
            }
            return false;
        }

        template <typename RouteContainer, typename MetaRouteContainer>
        [[nodiscard]] static ModOutputs apply(const RouteContainer& routes, const ModSourceValues& sources,
                                              const MetaRouteContainer& metaRoutes) noexcept
        {
            ModOutputs out;
            std::size_t routeIndex = 0;
            for (const auto& route : routes)
            {
                if (!route.isActive())
                {
                    ++routeIndex;
                    continue;
                }

                float amount = route.amount;
                for (const auto& meta : metaRoutes)
                {
                    if (!meta.isActive() || meta.targetRouteIndex != routeIndex)
                        continue;
                    if (meta.source == route.source)
                        continue;
                    amount += resolveSource(meta.source, meta.scope, sources) * meta.amount;
                }

                const float sourceValue =
                    shapeModSource(resolveSource(route.source, route.scope, sources), route.curve);
                applyRouteContribution(out, route.destination, route.targetIndex, sourceValue, amount);
                ++routeIndex;
            }
            return out;
        }

        template <typename RouteContainer>
        [[nodiscard]] static ModOutputs apply(const RouteContainer& routes, const ModSourceValues& sources) noexcept
        {
            ModOutputs out;
            for (const auto& route : routes)
            {
                if (!route.isActive())
                    continue;
                const float sourceValue =
                    shapeModSource(resolveSource(route.source, route.scope, sources), route.curve);
                applyRouteContribution(out, route.destination, route.targetIndex, sourceValue, route.amount);
            }
            return out;
        }

        template <typename RouteContainer>
        [[nodiscard]] static ModOutputs applyControlRateOnly(const RouteContainer& routes,
                                                              const ModSourceValues& sources) noexcept
        {
            ModSourceValues controlRateSources;
            controlRateSources.velocity = sources.velocity;
            controlRateSources.channelPressure = sources.channelPressure;
            controlRateSources.polyAftertouch = sources.polyAftertouch;
            controlRateSources.mpeSlide = sources.mpeSlide;
            controlRateSources.modWheel = sources.modWheel;
            controlRateSources.expression = sources.expression;
            controlRateSources.macros = sources.macros;
            controlRateSources.randomT = sources.randomT;
            controlRateSources.randomX = sources.randomX;
            controlRateSources.randomOutputs = sources.randomOutputs;
            return apply(routes, controlRateSources);
        }

        template <typename RouteContainer, typename MetaRouteContainer>
        [[nodiscard]] static ModOutputs applyControlRateOnly(const RouteContainer& routes,
                                                              const ModSourceValues& sources,
                                                              const MetaRouteContainer& metaRoutes) noexcept
        {
            ModSourceValues controlRateSources;
            controlRateSources.velocity = sources.velocity;
            controlRateSources.channelPressure = sources.channelPressure;
            controlRateSources.polyAftertouch = sources.polyAftertouch;
            controlRateSources.mpeSlide = sources.mpeSlide;
            controlRateSources.modWheel = sources.modWheel;
            controlRateSources.expression = sources.expression;
            controlRateSources.macros = sources.macros;
            controlRateSources.randomT = sources.randomT;
            controlRateSources.randomX = sources.randomX;
            controlRateSources.randomOutputs = sources.randomOutputs;
            return apply(routes, controlRateSources, metaRoutes);
        }

        template <typename RouteContainer>
        [[nodiscard]] static float computeMorphPositionOffset(const RouteContainer& routes,
                                                              const ModSourceValues& sources) noexcept
        {
            float offset = 0.0f;
            for (const auto& route : routes)
            {
                if (!route.isActive() || route.destination != ModDestination::MorphPosition)
                    continue;
                if (route.scope == ModScope::Voice)
                    continue;
                const float sourceValue =
                    shapeModSource(resolveSource(route.source, route.scope, sources), route.curve);
                offset += sourceValue * route.amount;
            }
            return offset;
        }

        template <typename RouteContainer>
        [[nodiscard]] static ModOutputs computeLayerUnisonMod(const RouteContainer& routes,
                                                              const ModSourceValues& sources) noexcept
        {
            ModOutputs out;
            for (const auto& route : routes)
            {
                if (!route.isActive())
                    continue;
                if (route.scope == ModScope::Voice)
                    continue;
                if (route.destination != ModDestination::UnisonVoices &&
                    route.destination != ModDestination::UnisonDetune &&
                    route.destination != ModDestination::UnisonSpread)
                    continue;
                const float sourceValue =
                    shapeModSource(resolveSource(route.source, route.scope, sources), route.curve);
                applyRouteContribution(out, route.destination, route.targetIndex, sourceValue, route.amount);
            }
            return out;
        }

    private:
        static void applyRouteContribution(ModOutputs& out, ModDestination destination, std::uint8_t targetIndex,
                                           float sourceValue, float amount) noexcept
        {
            switch (destination)
            {
                case ModDestination::FilterCutoff:
                    out.filterCutoffSemitones += sourceValue * amount;
                    break;
                case ModDestination::FilterResonance:
                    out.filterResonanceOffset += sourceValue * amount;
                    break;
                case ModDestination::FilterModeMorph:
                    out.filterModeMorphOffset += sourceValue * amount;
                    break;
                case ModDestination::FilterRouting:
                    out.filterRoutingOffset += sourceValue * amount;
                    break;
                case ModDestination::FilterDrive:
                    out.filterDriveOffset += sourceValue * amount;
                    break;
                case ModDestination::OperatorFilterCutoff:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorFilterCutoffSemitones[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorFilterResonance:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorFilterResonanceOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorLevel:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorLevelMultiplier[idx] *= (1.0f + sourceValue * amount);
                    break;
                }
                case ModDestination::Pan:
                    out.panOffset += sourceValue * amount;
                    break;
                case ModDestination::OperatorWavetablePosition:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorWavetablePositionOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorWavetableBend:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorWavetableBendOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorWavetableAsymmetry:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorWavetableAsymmetryOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorWavetableSyncRatio:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorWavetableSyncRatioOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorWavetableFormant:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorWavetableFormantOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorWavetableSyncAmount:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorWavetableSyncAmountOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorFmModulatorRatio:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorFmModulatorRatioOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorFmModulatorIndex:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorFmModulatorIndexOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorFmModulatorFeedback:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorFmModulatorFeedbackOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorFreqRatio:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorFreqRatioOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorPhaseBend:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorPhaseBendOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorPhaseFold:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorPhaseFoldOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorPhaseAsymmetry:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorPhaseAsymmetryOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorAdditivePartialCount:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorAdditivePartialCountOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorAdditiveTilt:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorAdditiveTiltOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorAdditiveOddEven:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorAdditiveOddEvenOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorAdditiveStretch:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorAdditiveStretchOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorResonatorStructure:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorResonatorStructureOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorResonatorDecay:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorResonatorDecayOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorResonatorDamping:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorResonatorDampingOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorResonatorBrightness:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorResonatorBrightnessOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorResonatorModeCount:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorResonatorModeCountOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorGrainDensity:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorGrainDensityOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorGrainSizeMs:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorGrainSizeMsOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorGrainPositionJitter:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorGrainPositionJitterOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::OperatorGrainPitchJitter:
                {
                    const std::uint8_t idx = targetIndex < core::kNodesPerLayer ? targetIndex : std::uint8_t{0};
                    out.operatorGrainPitchJitterOffset[idx] += sourceValue * amount;
                    break;
                }
                case ModDestination::UnisonVoices:
                    out.unisonVoicesOffset += sourceValue * amount;
                    break;
                case ModDestination::UnisonDetune:
                    out.unisonDetuneOffset += sourceValue * amount;
                    break;
                case ModDestination::UnisonSpread:
                    out.unisonSpreadOffset += sourceValue * amount;
                    break;
                case ModDestination::MasterFxMix:
                case ModDestination::MasterReverbMix:
                case ModDestination::MasterReverbSize:
                case ModDestination::MasterReverbDecay:
                case ModDestination::MasterReverbPreDelay:
                case ModDestination::MasterReverbDiffusion:
                case ModDestination::MasterReverbModDepth:
                case ModDestination::MasterGain:
                case ModDestination::MasterDynamicsMix:
                case ModDestination::SidechainDepth:
                case ModDestination::VocoderMix:
                case ModDestination::VocoderFormant:
                case ModDestination::QuasarQsr1Angle:
                case ModDestination::QuasarQsr2Angle:
                case ModDestination::QuasarRoomAmount:
                case ModDestination::QuasarCrossfeed:
                case ModDestination::QuasarDelayVolume:
                case ModDestination::QuasarQsr1Distance:
                case ModDestination::QuasarQsr2Distance:
                case ModDestination::QuasarDelayTime:
                case ModDestination::QuasarDelayFeedback:
                case ModDestination::QuasarQsr1Height:
                case ModDestination::QuasarQsr2Height:
                case ModDestination::QuasarCntrLevel:
                case ModDestination::QuasarQsr1Level:
                case ModDestination::QuasarQsr2Level:
                case ModDestination::ModRouteDepth:
                case ModDestination::MorphPosition:
                    break;
                case ModDestination::None:
                    break;
            }
        }

    public:
        template <typename RouteContainer>
        [[nodiscard]] static bool hasActiveMasterBusRoutes(const RouteContainer& routes) noexcept
        {
            for (const auto& route : routes)
            {
                if (!route.isActive() || route.scope == ModScope::Voice)
                    continue;
                switch (route.destination)
                {
                    case ModDestination::MasterFxMix:
                    case ModDestination::MasterReverbMix:
                    case ModDestination::MasterReverbSize:
                    case ModDestination::MasterReverbDecay:
                    case ModDestination::MasterReverbPreDelay:
                    case ModDestination::MasterReverbDiffusion:
                    case ModDestination::MasterReverbModDepth:
                    case ModDestination::MasterGain:
                        return true;
                    case ModDestination::MasterDynamicsMix:
                    case ModDestination::SidechainDepth:
                        return true;
                    case ModDestination::VocoderMix:
                    case ModDestination::VocoderFormant:
                    case ModDestination::QuasarQsr1Angle:
                    case ModDestination::QuasarQsr2Angle:
                    case ModDestination::QuasarRoomAmount:
                    case ModDestination::QuasarCrossfeed:
                    case ModDestination::QuasarDelayVolume:
                    case ModDestination::QuasarQsr1Distance:
                    case ModDestination::QuasarQsr2Distance:
                    case ModDestination::QuasarDelayTime:
                    case ModDestination::QuasarDelayFeedback:
                    case ModDestination::QuasarQsr1Height:
                    case ModDestination::QuasarQsr2Height:
                    case ModDestination::QuasarCntrLevel:
                    case ModDestination::QuasarQsr1Level:
                    case ModDestination::QuasarQsr2Level:
                        return true;
                    default:
                        break;
                }
            }
            return false;
        }

        template <typename RouteContainer>
        [[nodiscard]] static MasterModOutputs applyMasterBus(const RouteContainer& routes,
                                                             const ModSourceValues& sources) noexcept
        {
            MasterModOutputs out;
            for (const auto& route : routes)
            {
                if (!route.isActive() || route.scope == ModScope::Voice)
                    continue;

                const std::uint8_t slot =
                    route.targetIndex < effects::kNumMasterSlots ? route.targetIndex : std::uint8_t{0};
                const float sourceValue =
                    shapeModSource(resolveSource(route.source, route.scope, sources), route.curve);

                switch (route.destination)
                {
                    case ModDestination::MasterFxMix:
                    case ModDestination::MasterReverbMix:
                        out.mixOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterReverbSize:
                        out.reverbSizeOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterReverbDecay:
                        out.reverbDecayOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterReverbPreDelay:
                        out.reverbPreDelayOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterReverbDiffusion:
                        out.reverbDiffusionOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterReverbModDepth:
                        out.reverbModDepthOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterGain:
                        out.masterGainOffset += sourceValue * route.amount;
                        break;
                    case ModDestination::MasterDynamicsMix:
                        out.masterDynamicsMixOffset += sourceValue * route.amount;
                        break;
                    case ModDestination::SidechainDepth:
                        out.sidechainDepthOffset += sourceValue * route.amount;
                        break;
                    case ModDestination::VocoderMix:
                        out.masterVocoderMixOffset[slot] += sourceValue * route.amount;
                        if (slot < effects::kNumLayerInsertSlots)
                            out.vocoderMixOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::VocoderFormant:
                        out.masterVocoderFormantOffset[slot] += sourceValue * route.amount;
                        if (slot < effects::kNumLayerInsertSlots)
                            out.vocoderFormantOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr1Angle:
                        out.quasarQsr1AngleOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr2Angle:
                        out.quasarQsr2AngleOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarRoomAmount:
                        out.quasarRoomAmountOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarCrossfeed:
                        out.quasarCrossfeedOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarDelayVolume:
                        out.quasarDelayVolumeOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr1Distance:
                        out.quasarQsr1DistanceOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr2Distance:
                        out.quasarQsr2DistanceOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarDelayTime:
                        out.quasarDelayTimeOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarDelayFeedback:
                        out.quasarDelayFeedbackOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr1Height:
                        out.quasarQsr1HeightOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr2Height:
                        out.quasarQsr2HeightOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarCntrLevel:
                        out.quasarCntrLevelOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr1Level:
                        out.quasarQsr1LevelOffset[slot] += sourceValue * route.amount;
                        break;
                    case ModDestination::QuasarQsr2Level:
                        out.quasarQsr2LevelOffset[slot] += sourceValue * route.amount;
                        break;
                    default:
                        break;
                }
            }
            return out;
        }

    private:
        [[nodiscard]] static float resolveSource(ModSource src, ModScope scope, const ModSourceValues& s) noexcept
        {
            const bool sharedLfo = scope != ModScope::Voice; // Layer or Global -> the shared layer-wide tick.

            switch (src)
            {
                case ModSource::Lfo1: return sharedLfo ? s.layerLfos[0] : s.voiceLfos[0];
                case ModSource::Lfo2: return sharedLfo ? s.layerLfos[1] : s.voiceLfos[1];
                case ModSource::Lfo3: return sharedLfo ? s.layerLfos[2] : s.voiceLfos[2];
                case ModSource::Lfo4: return sharedLfo ? s.layerLfos[3] : s.voiceLfos[3];
                case ModSource::Lfo5: return sharedLfo ? s.layerLfos[4] : s.voiceLfos[4];
                case ModSource::Lfo6: return sharedLfo ? s.layerLfos[5] : s.voiceLfos[5];
                case ModSource::Lfo7: return sharedLfo ? s.layerLfos[6] : s.voiceLfos[6];
                case ModSource::Lfo8: return sharedLfo ? s.layerLfos[7] : s.voiceLfos[7];
                case ModSource::Env1: return s.envelopes[0];
                case ModSource::Env2: return s.envelopes[1];
                case ModSource::Env3: return s.envelopes[2];
                case ModSource::Env4: return s.envelopes[3];
                case ModSource::Env5: return s.envelopes[4];
                case ModSource::Env6: return s.envelopes[5];
                case ModSource::Env7: return s.envelopes[6];
                case ModSource::Env8: return s.envelopes[7];
                case ModSource::Velocity: return s.velocity;
                case ModSource::ChannelPressure: return s.channelPressure;
                case ModSource::PolyAftertouch: return s.polyAftertouch;
                case ModSource::MpeSlide: return s.mpeSlide;
                case ModSource::ModWheel: return s.modWheel;
                case ModSource::Expression: return s.expression;
                case ModSource::Sidechain: return s.sidechain;
                case ModSource::Macro1: return s.macros[0];
                case ModSource::Macro2: return s.macros[1];
                case ModSource::Macro3: return s.macros[2];
                case ModSource::Macro4: return s.macros[3];
                case ModSource::Macro5: return s.macros[4];
                case ModSource::Macro6: return s.macros[5];
                case ModSource::Macro7: return s.macros[6];
                case ModSource::Macro8: return s.macros[7];
                case ModSource::Random1: return s.randomOutputs[0];
                case ModSource::Random2: return s.randomOutputs[1];
                case ModSource::Random3: return s.randomOutputs[2];
                case ModSource::Random4: return s.randomOutputs[3];
                case ModSource::RandomT: return s.randomT;
                case ModSource::RandomX: return s.randomX;
                case ModSource::None: return 0.0f;
            }
            return 0.0f;
        }
    };

} // namespace pw8::modulation
