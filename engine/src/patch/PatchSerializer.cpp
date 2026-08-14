#include "pw8/patch/PatchSerializer.hpp"

#include <algorithm>

#include <nlohmann/json.hpp>

// Untrusted-input hardening (docs/PATCH_FORMAT.md "Security / Robustness"):
//   - A hard ceiling on input size before we even attempt to parse.
//   - All fixed-capacity containers (FixedVector, std::array<..., 8>) silently clamp
//     writes at capacity rather than growing, so a malicious `edges` array with
//     10,000 entries cannot cause unbounded allocation -- extras are simply dropped.
//   - Every numeric field is range-clamped on the way in.
//   - Parse errors and structurally-invalid documents return PatchLoadResult{ok=false},
///    never a partially-constructed Patch presented as valid.

namespace pw8::patch
{
    namespace
    {
        using nlohmann::json;

        constexpr std::size_t kMaxJsonInputBytes = 8ull * 1024 * 1024; // 8 MB sanity ceiling.

        template <typename T>
        [[nodiscard]] T clampNum(T v, T lo, T hi) noexcept
        {
            return v < lo ? lo : (v > hi ? hi : v);
        }

        void toJson(json& j, const envelope::DahdsrParams& e)
        {
            j = json{
                {"delaySeconds", e.delaySeconds},     {"attackSeconds", e.attackSeconds},
                {"holdSeconds", e.holdSeconds},        {"decaySeconds", e.decaySeconds},
                {"sustainLevel", e.sustainLevel},      {"releaseSeconds", e.releaseSeconds},
                {"curveShape", e.curveShape},          {"legato", e.legato},
            };
        }

        void fromJson(const json& j, envelope::DahdsrParams& e)
        {
            e.delaySeconds = clampNum(j.value("delaySeconds", e.delaySeconds), 0.0f, 60.0f);
            e.attackSeconds = clampNum(j.value("attackSeconds", e.attackSeconds), 0.0f, 60.0f);
            e.holdSeconds = clampNum(j.value("holdSeconds", e.holdSeconds), 0.0f, 60.0f);
            e.decaySeconds = clampNum(j.value("decaySeconds", e.decaySeconds), 0.0f, 60.0f);
            e.sustainLevel = clampNum(j.value("sustainLevel", e.sustainLevel), 0.0f, 1.0f);
            e.releaseSeconds = clampNum(j.value("releaseSeconds", e.releaseSeconds), 0.0f, 60.0f);
            e.curveShape = clampNum(j.value("curveShape", e.curveShape), 0.0f, 16.0f);
            e.legato = j.value("legato", e.legato);
        }

        void toJson(json& j, const algorithm::AlgorithmGraphDefinition& g)
        {
            json nodes = json::array();
            for (const auto& n : g.nodes)
                nodes.push_back(json{{"id", n.id.get()}, {"engine", static_cast<int>(n.engine)}, {"isOutput", n.isOutput}});

            json edges = json::array();
            for (const auto& e : g.edges)
                edges.push_back(json{{"source", e.source.get()},
                                      {"destination", e.destination.get()},
                                      {"type", static_cast<int>(e.type)},
                                      {"amount", e.amount}});

            j = json{{"nodes", nodes}, {"edges", edges}};
        }

        void fromJson(const json& j, algorithm::AlgorithmGraphDefinition& g)
        {
            g = algorithm::AlgorithmGraphDefinition{};
            if (j.contains("nodes") && j.at("nodes").is_array())
            {
                for (const auto& jn : j.at("nodes"))
                {
                    if (g.nodes.size() >= core::kNodesPerLayer)
                        break;
                    algorithm::AlgorithmNode n;
                    n.id = core::NodeId(static_cast<std::uint8_t>(clampNum(jn.value("id", 0), 0, 255)));
                    n.engine = static_cast<algorithm::EngineType>(clampNum(jn.value("engine", 0), 0, 7));
                    n.isOutput = jn.value("isOutput", false);
                    g.nodes.push_back(n);
                }
            }
            if (j.contains("edges") && j.at("edges").is_array())
            {
                for (const auto& je : j.at("edges"))
                {
                    if (g.edges.size() >= core::kMaxAlgorithmEdges)
                        break;
                    algorithm::AlgorithmEdge e;
                    e.source = core::NodeId(static_cast<std::uint8_t>(clampNum(je.value("source", 0), 0, 255)));
                    e.destination = core::NodeId(static_cast<std::uint8_t>(clampNum(je.value("destination", 0), 0, 255)));
                    e.type = static_cast<algorithm::EdgeType>(clampNum(je.value("type", 0), 0, 6));
                    e.amount = clampNum(je.value("amount", 1.0f), -1000.0f, 1000.0f);
                    g.edges.push_back(e);
                }
            }
        }

        void toJson(json& j, const OperatorPatch& o)
        {
            j = json{
                {"engine", static_cast<int>(o.engine)},
                {"classicWaveform", static_cast<int>(o.classicWaveform)},
                {"classicMorph", o.classicMorph},
                {"pulseWidth", o.pulseWidth},
                {"wavetableFramePosition", o.wavetableFramePosition},
                {"wavetableId", o.wavetableId},
                {"frequencyRatio", o.frequencyRatio},
                {"fixedFrequencyHz", o.fixedFrequencyHz},
                {"keyTrack", o.keyTrack},
                {"level", o.level},
                {"pan", o.pan},
                {"fmModulatorRatio", o.fmModulatorRatio},
                {"fmModulatorIndex", o.fmModulatorIndex},
                {"fmModulatorFeedback", o.fmModulatorFeedback},
                {"fmModulatorWaveform", static_cast<int>(o.fmModulatorWaveform)},
                {"noiseVariant", o.noiseVariant},
                {"noiseRate", o.noiseRate},
                {"phaseBend", o.phaseBend},
                {"phaseFold", o.phaseFold},
                {"phaseAsymmetry", o.phaseAsymmetry},
                {"phaseShape", o.phaseShape},
                {"additivePartialCount", o.additivePartialCount},
                {"additiveTilt", o.additiveTilt},
                {"additiveOddEven", o.additiveOddEven},
                {"additiveStretch", o.additiveStretch},
                {"resonatorStructure", o.resonatorStructure},
                {"resonatorDecay", o.resonatorDecay},
                {"resonatorDamping", o.resonatorDamping},
                {"resonatorBrightness", o.resonatorBrightness},
                {"resonatorModeCount", o.resonatorModeCount},
                {"grainDensity", o.grainDensity},
                {"grainSizeMs", o.grainSizeMs},
                {"grainPositionJitter", o.grainPositionJitter},
                {"grainPitchJitter", o.grainPitchJitter},
                {"wtBend", o.wtBend},
                {"wtAsymmetry", o.wtAsymmetry},
                {"wtSyncRatio", o.wtSyncRatio},
                {"wtSyncAmount", o.wtSyncAmount},
                {"wtFormantShift", o.wtFormantShift},
                {"filter1", json{{"enabled", o.filter1.enabled},
                                 {"mode", static_cast<int>(o.filter1.mode)},
                                 {"cutoffHz", o.filter1.cutoffHz},
                                 {"resonance", o.filter1.resonance},
                                 {"keyTrack", o.filter1.keyTrack}}},
            };
        }

        void fromJson(const json& j, OperatorPatch& o)
        {
            o.engine = static_cast<algorithm::EngineType>(clampNum(j.value("engine", 0), 0, 7));
            o.classicWaveform = static_cast<oscillator::ClassicWaveform>(clampNum(j.value("classicWaveform", 2), 0, 3));
            o.classicMorph = clampNum(j.value("classicMorph", -1.0f), -1.0f, 1.0f);
            o.pulseWidth = clampNum(j.value("pulseWidth", 0.5f), 0.01f, 0.99f);
            o.wavetableFramePosition = clampNum(j.value("wavetableFramePosition", 0.0f), 0.0f, 1.0f);
            o.wavetableId = j.value("wavetableId", std::string{});
            o.frequencyRatio = clampNum(j.value("frequencyRatio", 1.0f), 0.001f, 128.0f);
            o.fixedFrequencyHz = clampNum(j.value("fixedFrequencyHz", 440.0f), 0.01f, 24000.0f);
            o.keyTrack = j.value("keyTrack", true);
            o.level = clampNum(j.value("level", 1.0f), 0.0f, 4.0f);
            o.pan = clampNum(j.value("pan", 0.0f), -1.0f, 1.0f);
            o.fmModulatorRatio = clampNum(j.value("fmModulatorRatio", 1.0f), 0.001f, 32.0f);
            o.fmModulatorIndex = clampNum(j.value("fmModulatorIndex", 0.5f), 0.0f, 2.0f);
            o.fmModulatorFeedback = clampNum(j.value("fmModulatorFeedback", 0.0f), 0.0f, 1.0f);
            o.fmModulatorWaveform =
                static_cast<oscillator::ClassicWaveform>(clampNum(j.value("fmModulatorWaveform", 0), 0, 3));
            o.noiseVariant = clampNum(j.value("noiseVariant", 0.0f), 0.0f, 6.0f);
            o.noiseRate = clampNum(j.value("noiseRate", 200.0f), 0.5f, 2000.0f);
            o.phaseBend = clampNum(j.value("phaseBend", 0.0f), -1.0f, 1.0f);
            o.phaseFold = clampNum(j.value("phaseFold", 0.0f), 0.0f, 1.0f);
            o.phaseAsymmetry = clampNum(j.value("phaseAsymmetry", 0.0f), -1.0f, 1.0f);
            o.phaseShape = clampNum(j.value("phaseShape", 0.0f), 0.0f, 1.0f);
            o.additivePartialCount = clampNum(j.value("additivePartialCount", 32.0f), 1.0f, 64.0f);
            o.additiveTilt = clampNum(j.value("additiveTilt", 0.0f), -1.0f, 1.0f);
            o.additiveOddEven = clampNum(j.value("additiveOddEven", 0.5f), 0.0f, 1.0f);
            o.additiveStretch = clampNum(j.value("additiveStretch", 0.0f), -1.0f, 1.0f);
            o.resonatorStructure = clampNum(j.value("resonatorStructure", 0.3f), 0.0f, 1.0f);
            o.resonatorDecay = clampNum(j.value("resonatorDecay", 0.5f), 0.0f, 1.0f);
            o.resonatorDamping = clampNum(j.value("resonatorDamping", 0.5f), 0.0f, 1.0f);
            o.resonatorBrightness = clampNum(j.value("resonatorBrightness", 0.5f), 0.0f, 1.0f);
            o.resonatorModeCount = clampNum(j.value("resonatorModeCount", 6.0f), 2.0f, 8.0f);
            o.grainDensity = clampNum(j.value("grainDensity", 20.0f), 0.5f, 200.0f);
            o.grainSizeMs = clampNum(j.value("grainSizeMs", 60.0f), 1.0f, 500.0f);
            o.grainPositionJitter = clampNum(j.value("grainPositionJitter", 0.1f), 0.0f, 1.0f);
            o.grainPitchJitter = clampNum(j.value("grainPitchJitter", 0.0f), 0.0f, 1.0f);
            o.wtBend = clampNum(j.value("wtBend", 0.0f), -1.0f, 1.0f);
            o.wtAsymmetry = clampNum(j.value("wtAsymmetry", 0.0f), -1.0f, 1.0f);
            o.wtSyncRatio = clampNum(j.value("wtSyncRatio", 1.0f), 1.0f, 16.0f);
            o.wtSyncAmount = clampNum(j.value("wtSyncAmount", 0.0f), 0.0f, 1.0f);
            o.wtFormantShift = clampNum(j.value("wtFormantShift", 0.0f), -1.0f, 1.0f);
            if (j.contains("filter1"))
            {
                const auto& jf = j.at("filter1");
                o.filter1.enabled = jf.value("enabled", false);
                o.filter1.mode = static_cast<filter::FilterMode>(clampNum(jf.value("mode", 0), 0, 4));
                o.filter1.cutoffHz = clampNum(jf.value("cutoffHz", 8000.0f), 10.0f, 24000.0f);
                o.filter1.resonance = clampNum(jf.value("resonance", 0.2f), 0.0f, 1.0f);
                o.filter1.keyTrack = clampNum(jf.value("keyTrack", 0.0f), -1.0f, 1.0f);
            }
        }

        void toJson(json& j, const UnisonSettings& u)
        {
            j = json{{"mode", static_cast<int>(u.mode)},   {"voices", u.voices},
                     {"detuneCents", u.detuneCents},        {"spread", u.spread},
                     {"phaseRandom", u.phaseRandom},         {"blend", u.blend}};
        }

        void fromJson(const json& j, UnisonSettings& u)
        {
            u.mode = static_cast<UnisonMode>(clampNum(j.value("mode", 0), 0, 5));
            u.voices = clampNum(j.value("voices", 1), 1, static_cast<int>(core::kMaxUnisonVoices));
            u.detuneCents = clampNum(j.value("detuneCents", 0.0f), 0.0f, 200.0f);
            u.spread = clampNum(j.value("spread", 0.0f), 0.0f, 1.0f);
            u.phaseRandom = clampNum(j.value("phaseRandom", 0.0f), 0.0f, 1.0f);
            u.blend = clampNum(j.value("blend", 1.0f), 0.0f, 1.0f);
        }

        void toJson(json& j, const filter::FilterParams& f)
        {
            j = json{{"enabled", f.enabled},     {"mode", static_cast<int>(f.mode)},
                     {"cutoffHz", f.cutoffHz},     {"resonance", f.resonance},
                     {"keyTrack", f.keyTrack}};
        }

        void fromJson(const json& j, filter::FilterParams& f)
        {
            f.enabled = j.value("enabled", false);
            f.mode = static_cast<filter::FilterMode>(clampNum(j.value("mode", 0), 0, 4));
            f.cutoffHz = clampNum(j.value("cutoffHz", 8000.0f), 10.0f, 24000.0f);
            f.resonance = clampNum(j.value("resonance", 0.2f), 0.0f, 1.0f);
            f.keyTrack = clampNum(j.value("keyTrack", 0.0f), -1.0f, 1.0f);
        }

        void toJson(json& j, const filter::CharacterFilterParams& f)
        {
            j = json{{"enabled", f.enabled},     {"cutoffHz", f.cutoffHz}, {"resonance", f.resonance},
                     {"drive", f.drive},         {"keyTrack", f.keyTrack}};
        }

        void fromJson(const json& j, filter::CharacterFilterParams& f)
        {
            f.enabled = j.value("enabled", false);
            f.cutoffHz = clampNum(j.value("cutoffHz", 4000.0f), 20.0f, 24000.0f);
            f.resonance = clampNum(j.value("resonance", 0.3f), 0.0f, 1.0f);
            f.drive = clampNum(j.value("drive", 0.0f), 0.0f, 1.0f);
            f.keyTrack = clampNum(j.value("keyTrack", 0.0f), -1.0f, 1.0f);
        }

        void toJson(json& j, const lfo::LfoParams& l)
        {
            j = json{{"waveform", static_cast<int>(l.waveform)}, {"mode", static_cast<int>(l.mode)},
                     {"rateHz", l.rateHz},                        {"syncDivisionIndex", l.syncDivisionIndex},
                     {"phaseOffset", l.phaseOffset}};
        }

        void fromJson(const json& j, lfo::LfoParams& l)
        {
            l.waveform = static_cast<lfo::LfoWaveform>(clampNum(j.value("waveform", 0), 0, 5));
            l.mode = static_cast<lfo::LfoMode>(clampNum(j.value("mode", 0), 0, 3));
            l.rateHz = clampNum(j.value("rateHz", 2.0f), 0.001f, 50.0f);
            l.syncDivisionIndex = clampNum(j.value("syncDivisionIndex", 4), 0, 9);
            l.phaseOffset = clampNum(j.value("phaseOffset", 0.0f), 0.0f, 1.0f);
        }

        void toJson(json& j, const modulation::ModRoute& r)
        {
            j = json{{"source", static_cast<int>(r.source)}, {"destination", static_cast<int>(r.destination)},
                     {"targetIndex", r.targetIndex},           {"amount", r.amount},
                     {"scope", static_cast<int>(r.scope)}};
        }

        void fromJson(const json& j, modulation::ModRoute& r)
        {
            r.source = static_cast<modulation::ModSource>(
                clampNum(j.value("source", 0), 0, static_cast<int>(modulation::ModSource::Expression)));
            r.destination = static_cast<modulation::ModDestination>(clampNum(j.value("destination", 0), 0, 20));
            r.targetIndex = static_cast<std::uint8_t>(clampNum(j.value("targetIndex", 0), 0, 255));
            r.amount = clampNum(j.value("amount", 0.0f), -1000.0f, 1000.0f);
            r.scope = static_cast<modulation::ModScope>(clampNum(j.value("scope", 0), 0, 2));
        }

        void toJson(json& j, const effects::DelayNodeParams& n)
        {
            j = json{{"enabled", n.enabled},         {"parentIndex", n.parentIndex},
                     {"delayMs", n.delayMs},           {"feedback", n.feedback},
                     {"pan", n.pan},                   {"distortion", n.distortion},
                     {"level", n.level}};
        }

        void fromJson(const json& j, effects::DelayNodeParams& n)
        {
            n.enabled = j.value("enabled", true);
            n.parentIndex = clampNum(j.value("parentIndex", -1), -1, static_cast<int>(effects::kMaxDelayNodes) - 1);
            n.delayMs = clampNum(j.value("delayMs", 250.0f), 1.0f, effects::kMaxTreeNodeDelaySeconds * 1000.0f);
            n.feedback = clampNum(j.value("feedback", 0.35f), 0.0f, 0.95f);
            n.pan = clampNum(j.value("pan", 0.0f), -1.0f, 1.0f);
            n.distortion = clampNum(j.value("distortion", 0.0f), 0.0f, 1.0f);
            n.level = clampNum(j.value("level", 0.7f), 0.0f, 1.0f);
        }

        void toJson(json& j, const effects::EffectSlotParams& e)
        {
            json nodes = json::array();
            for (const auto& n : e.nodes)
            {
                json jn;
                toJson(jn, n);
                nodes.push_back(jn);
            }

            j = json{
                {"type", static_cast<int>(e.type)},                 {"mix", e.mix},
                {"saturationDriveDb", e.saturationDriveDb},
                {"chorusRateHz", e.chorusRateHz},                   {"chorusDepthMs", e.chorusDepthMs},
                {"chorusBaseDelayMs", e.chorusBaseDelayMs},
                {"tapeDelayMs", e.tapeDelayMs},                     {"tapeFeedback", e.tapeFeedback},
                {"tapeDriveDb", e.tapeDriveDb},                     {"tapeDuckAmount", e.tapeDuckAmount},
                {"tapeDriftDepthMs", e.tapeDriftDepthMs},           {"tapeDriftRateHz", e.tapeDriftRateHz},
                {"tapePanMode", static_cast<int>(e.tapePanMode)},
                {"nodes", nodes},                                   {"nodeInsanity", e.nodeInsanity},
                {"freqShiftHz", e.freqShiftHz},                     {"freqShiftDelayMs", e.freqShiftDelayMs},
                {"freqShiftFeedback", e.freqShiftFeedback},         {"freqShiftLowCutHz", e.freqShiftLowCutHz},
                {"freqShiftHighCutHz", e.freqShiftHighCutHz},
                {"fractalSeedA", e.fractalSeedA},                   {"fractalSeedB", e.fractalSeedB},
                {"fractalMorph", e.fractalMorph},                   {"fractalBaseDelayMs", e.fractalBaseDelayMs},
                {"fractalRatio", e.fractalRatio},                   {"fractalSpreadMs", e.fractalSpreadMs},
                {"reverbSizeParam", e.reverbSizeParam},             {"reverbDecaySeconds", e.reverbDecaySeconds},
                {"reverbPreDelayMs", e.reverbPreDelayMs},
                {"reverbHighRatio", e.reverbHighRatio},             {"reverbHighCrossoverHz", e.reverbHighCrossoverHz},
                {"reverbLowRatio", e.reverbLowRatio},               {"reverbLowCrossoverHz", e.reverbLowCrossoverHz},
                {"reverbDiffusion", e.reverbDiffusion},             {"reverbDensity", e.reverbDensity},
                {"reverbModDepth", e.reverbModDepth},               {"reverbModRateHz", e.reverbModRateHz},
                {"reverbEarlyLevel", e.reverbEarlyLevel},           {"reverbLateLevel", e.reverbLateLevel},
                {"reverbRollOffHz", e.reverbRollOffHz},             {"reverbVlfCutDb", e.reverbVlfCutDb},
                {"eqLowFreqHz", e.eqLowFreqHz},                     {"eqLowGainDb", e.eqLowGainDb},
                {"eqMidFreqHz", e.eqMidFreqHz},                     {"eqMidGainDb", e.eqMidGainDb},
                {"eqMidQ", e.eqMidQ},
                {"eqHighFreqHz", e.eqHighFreqHz},                   {"eqHighGainDb", e.eqHighGainDb},
                {"compThresholdDb", e.compThresholdDb},             {"compRatio", e.compRatio},
                {"compAttackMs", e.compAttackMs},                   {"compReleaseMs", e.compReleaseMs},
                {"compKneeDb", e.compKneeDb},                       {"compMakeupDb", e.compMakeupDb},
                {"compTransformerCore", e.compTransformerCore},   {"compTransformerBrand", e.compTransformerBrand},
                {"compTransformerAmount", e.compTransformerAmount},
                {"limiterCeilingDb", e.limiterCeilingDb},           {"limiterLookaheadMs", e.limiterLookaheadMs},
                {"limiterReleaseMs", e.limiterReleaseMs},
            };
        }

        void fromJson(const json& j, effects::EffectSlotParams& e)
        {
            e.type = static_cast<effects::EffectType>(clampNum(j.value("type", 0), 0, 10));
            e.mix = clampNum(j.value("mix", 1.0f), 0.0f, 1.0f);

            e.saturationDriveDb = clampNum(j.value("saturationDriveDb", 6.0f), 0.0f, 48.0f);

            e.chorusRateHz = clampNum(j.value("chorusRateHz", 0.5f), 0.01f, 10.0f);
            e.chorusDepthMs = clampNum(j.value("chorusDepthMs", 4.0f), 0.0f, 20.0f);
            e.chorusBaseDelayMs = clampNum(j.value("chorusBaseDelayMs", 12.0f), 1.0f, 40.0f);

            e.tapeDelayMs = clampNum(j.value("tapeDelayMs", 350.0f), 1.0f, effects::kMaxEffectDelaySeconds * 1000.0f);
            e.tapeFeedback = clampNum(j.value("tapeFeedback", 0.4f), 0.0f, 0.98f);
            e.tapeDriveDb = clampNum(j.value("tapeDriveDb", 3.0f), 0.0f, 48.0f);
            e.tapeDuckAmount = clampNum(j.value("tapeDuckAmount", 0.0f), 0.0f, 1.0f);
            e.tapeDriftDepthMs = clampNum(j.value("tapeDriftDepthMs", 1.5f), 0.0f, 20.0f);
            e.tapeDriftRateHz = clampNum(j.value("tapeDriftRateHz", 0.3f), 0.0f, 10.0f);
            e.tapePanMode = static_cast<effects::DelayPanMode>(clampNum(j.value("tapePanMode", 0), 0, 2));

            e.nodes = std::array<effects::DelayNodeParams, effects::kMaxDelayNodes>{};
            if (j.contains("nodes") && j.at("nodes").is_array())
            {
                std::size_t i = 0;
                for (const auto& jn : j.at("nodes"))
                {
                    if (i >= effects::kMaxDelayNodes)
                        break;
                    fromJson(jn, e.nodes[i]);
                    ++i;
                }
            }
            e.nodeInsanity = clampNum(j.value("nodeInsanity", 0.0f), 0.0f, 1.0f);

            e.freqShiftHz = clampNum(j.value("freqShiftHz", 7.0f), -2000.0f, 2000.0f);
            e.freqShiftDelayMs =
                clampNum(j.value("freqShiftDelayMs", 280.0f), 1.0f, effects::kMaxEffectDelaySeconds * 1000.0f);
            e.freqShiftFeedback = clampNum(j.value("freqShiftFeedback", 0.55f), 0.0f, 0.98f);
            e.freqShiftLowCutHz = clampNum(j.value("freqShiftLowCutHz", 120.0f), 5.0f, 20000.0f);
            e.freqShiftHighCutHz = clampNum(j.value("freqShiftHighCutHz", 8000.0f), 20.0f, 20000.0f);

            e.fractalSeedA = j.value("fractalSeedA", static_cast<std::uint64_t>(1));
            e.fractalSeedB = j.value("fractalSeedB", static_cast<std::uint64_t>(2));
            e.fractalMorph = clampNum(j.value("fractalMorph", 0.0f), 0.0f, 1.0f);
            e.fractalBaseDelayMs =
                clampNum(j.value("fractalBaseDelayMs", 180.0f), 1.0f, effects::kMaxTreeNodeDelaySeconds * 1000.0f);
            e.fractalRatio = clampNum(j.value("fractalRatio", 0.62f), 0.1f, 0.95f);
            e.fractalSpreadMs = clampNum(j.value("fractalSpreadMs", 15.0f), 0.0f, 100.0f);

            e.reverbSizeParam = clampNum(j.value("reverbSizeParam", 1.0f), 0.2f, 3.0f);
            e.reverbDecaySeconds = clampNum(j.value("reverbDecaySeconds", 2.0f), 0.05f, 20.0f);
            e.reverbPreDelayMs =
                clampNum(j.value("reverbPreDelayMs", 20.0f), 0.0f, effects::kMaxReverbPreDelaySeconds * 1000.0f);

            // GATE 11 replaced the single one-pole "reverbDampingHz" feedback filter
            // with an independent HF/LF multiband decay pair. A pre-GATE-11
            // document only ever has "reverbDampingHz"; read it as the seed for the
            // new HF crossover (it already sat at roughly "where highs start
            // rolling off," a reasonable crossover point) with a fixed 0.5 ratio
            // approximating what a single one-pole damping filter sounded like --
            // not an exact match, an honest approximation, same spirit as the
            // v1->v2 migration's documented compatibility decisions. A document
            // that already has the new keys (or omits both) just uses their own
            // defaults below, untouched.
            const float legacyDampingHz = j.value("reverbDampingHz", 6000.0f);
            e.reverbHighRatio = clampNum(j.value("reverbHighRatio", 0.6f), 0.2f, 1.0f);
            e.reverbHighCrossoverHz =
                clampNum(j.value("reverbHighCrossoverHz", legacyDampingHz), 200.0f, 16000.0f);
            e.reverbLowRatio = clampNum(j.value("reverbLowRatio", 1.3f), 0.2f, 4.0f);
            e.reverbLowCrossoverHz = clampNum(j.value("reverbLowCrossoverHz", 400.0f), 80.0f, 4800.0f);
            e.reverbDiffusion = clampNum(j.value("reverbDiffusion", 0.65f), 0.0f, 1.0f);
            e.reverbDensity = clampNum(j.value("reverbDensity", 0.85f), 0.0f, 1.0f);
            e.reverbModDepth = clampNum(j.value("reverbModDepth", 0.35f), 0.0f, 1.0f);
            e.reverbModRateHz = clampNum(j.value("reverbModRateHz", 0.4f), 0.05f, 2.0f);
            e.reverbEarlyLevel = clampNum(j.value("reverbEarlyLevel", 0.5f), 0.0f, 1.0f);
            e.reverbLateLevel = clampNum(j.value("reverbLateLevel", 1.0f), 0.0f, 1.0f);
            e.reverbRollOffHz = clampNum(j.value("reverbRollOffHz", 12000.0f), 80.0f, 20000.0f);
            e.reverbVlfCutDb = clampNum(j.value("reverbVlfCutDb", 0.0f), -18.0f, 0.0f);

            e.eqLowFreqHz = clampNum(j.value("eqLowFreqHz", 200.0f), 20.0f, 20000.0f);
            e.eqLowGainDb = clampNum(j.value("eqLowGainDb", 0.0f), -24.0f, 24.0f);
            e.eqMidFreqHz = clampNum(j.value("eqMidFreqHz", 1000.0f), 20.0f, 20000.0f);
            e.eqMidGainDb = clampNum(j.value("eqMidGainDb", 0.0f), -24.0f, 24.0f);
            e.eqMidQ = clampNum(j.value("eqMidQ", 0.8f), 0.1f, 10.0f);
            e.eqHighFreqHz = clampNum(j.value("eqHighFreqHz", 6000.0f), 20.0f, 20000.0f);
            e.eqHighGainDb = clampNum(j.value("eqHighGainDb", 0.0f), -24.0f, 24.0f);

            e.compThresholdDb = clampNum(j.value("compThresholdDb", -18.0f), -60.0f, 0.0f);
            e.compRatio = clampNum(j.value("compRatio", 3.0f), 1.0f, 20.0f);
            e.compAttackMs = clampNum(j.value("compAttackMs", 8.0f), 0.1f, 500.0f);
            e.compReleaseMs = clampNum(j.value("compReleaseMs", 120.0f), 1.0f, 2000.0f);
            e.compKneeDb = clampNum(j.value("compKneeDb", 6.0f), 0.0f, 24.0f);
            e.compMakeupDb = clampNum(j.value("compMakeupDb", 0.0f), 0.0f, 24.0f);
            e.compTransformerCore = clampNum(j.value("compTransformerCore", 0.0f), 0.0f, 3.0f);
            e.compTransformerBrand = clampNum(j.value("compTransformerBrand", 0.0f), 0.0f, 3.0f);
            e.compTransformerAmount = clampNum(j.value("compTransformerAmount", 1.0f), 0.0f, 1.0f);

            e.limiterCeilingDb = clampNum(j.value("limiterCeilingDb", -0.3f), -12.0f, 0.0f);
            e.limiterLookaheadMs = clampNum(j.value("limiterLookaheadMs", 5.0f),
                                             0.5f, effects::kMaxLimiterLookaheadSeconds * 1000.0f);
            e.limiterReleaseMs = clampNum(j.value("limiterReleaseMs", 60.0f), 1.0f, 2000.0f);
        }

        void toJson(json& j, const LayerPatch& l)
        {
            json ops = json::array();
            for (const auto& o : l.operators)
            {
                json jo;
                toJson(jo, o);
                ops.push_back(jo);
            }
            json algo, uni, filt, filt2;
            toJson(algo, l.algorithm);
            toJson(uni, l.unison);
            toJson(filt, l.filter1);
            toJson(filt2, l.filter2);

            json envelopes = json::array();
            for (const auto& e : l.envelopes)
            {
                json je;
                toJson(je, e);
                envelopes.push_back(je);
            }

            json lfos = json::array();
            for (const auto& lf : l.lfos)
            {
                json jl;
                toJson(jl, lf);
                lfos.push_back(jl);
            }

            json routes = json::array();
            for (const auto& r : l.modRoutes)
            {
                json jr;
                toJson(jr, r);
                routes.push_back(jr);
            }

            json inserts = json::array();
            for (const auto& fx : l.insertEffects)
            {
                json jfx;
                toJson(jfx, fx);
                inserts.push_back(jfx);
            }

            j = json{{"operators", ops},       {"algorithm", algo},           {"envelopes", envelopes},
                     {"unison", uni},           {"filter1", filt},             {"filter2", filt2},
                     {"lfos", lfos},            {"modRoutes", routes},         {"gain", l.gain},
                     {"pan", l.pan},            {"width", l.width},            {"centerGravity", l.centerGravity},
                     {"insertEffects", inserts}};
        }

        void fromJson(const json& j, LayerPatch& l)
        {
            if (j.contains("operators") && j.at("operators").is_array())
            {
                std::size_t i = 0;
                for (const auto& jo : j.at("operators"))
                {
                    if (i >= core::kNodesPerLayer)
                        break;
                    fromJson(jo, l.operators[i]);
                    ++i;
                }
            }
            if (j.contains("algorithm"))
                fromJson(j.at("algorithm"), l.algorithm);

            // By the time fromJson runs, migrateToCurrentSchema() has already rewritten
            // any v1 singular "ampEnvelope"/"lfo1" into "envelopes"/"lfos" arrays -- this
            // only ever needs to read the current (v2+) array shape.
            l.envelopes = std::array<envelope::DahdsrParams, core::kNumEnvelopesPerLayer>{};
            if (j.contains("envelopes") && j.at("envelopes").is_array())
            {
                std::size_t i = 0;
                for (const auto& je : j.at("envelopes"))
                {
                    if (i >= core::kNumEnvelopesPerLayer)
                        break;
                    fromJson(je, l.envelopes[i]);
                    ++i;
                }
            }

            if (j.contains("unison"))
                fromJson(j.at("unison"), l.unison);
            if (j.contains("filter1"))
                fromJson(j.at("filter1"), l.filter1);
            if (j.contains("filter2"))
                fromJson(j.at("filter2"), l.filter2);

            l.lfos = std::array<lfo::LfoParams, core::kNumLfosPerLayer>{};
            if (j.contains("lfos") && j.at("lfos").is_array())
            {
                std::size_t i = 0;
                for (const auto& jl : j.at("lfos"))
                {
                    if (i >= core::kNumLfosPerLayer)
                        break;
                    fromJson(jl, l.lfos[i]);
                    ++i;
                }
            }

            l.modRoutes.clear();
            if (j.contains("modRoutes") && j.at("modRoutes").is_array())
            {
                for (const auto& jr : j.at("modRoutes"))
                {
                    if (l.modRoutes.size() >= core::kMaxModRoutes)
                        break;
                    modulation::ModRoute r;
                    fromJson(jr, r);
                    l.modRoutes.push_back(r);
                }
            }

            l.gain = clampNum(j.value("gain", 1.0f), 0.0f, 4.0f);
            l.pan = clampNum(j.value("pan", 0.0f), -1.0f, 1.0f);
            l.width = clampNum(j.value("width", 1.0f), 0.0f, 2.0f);
            l.centerGravity = clampNum(j.value("centerGravity", 0.5f), 0.0f, 1.0f);

            l.insertEffects = std::array<effects::EffectSlotParams, effects::kNumLayerInsertSlots>{};
            if (j.contains("insertEffects") && j.at("insertEffects").is_array())
            {
                std::size_t i = 0;
                for (const auto& jfx : j.at("insertEffects"))
                {
                    if (i >= effects::kNumLayerInsertSlots)
                        break;
                    fromJson(jfx, l.insertEffects[i]);
                    ++i;
                }
            }
        }

        void toJson(json& j, const PatchMetadata& m)
        {
            j = json{
                {"id", m.id},           {"name", m.name},                 {"author", m.author},
                {"description", m.description}, {"category", m.category}, {"moods", m.moods},
                {"genres", m.genres},   {"tags", m.tags},                 {"createdAt", m.createdAt},
                {"engineVersion", m.engineVersion}, {"schemaVersion", m.schemaVersion},
                {"seed", m.seed},       {"lineage", m.lineage},
            };
        }

        void fromJson(const json& j, PatchMetadata& m)
        {
            m.id = j.value("id", std::string{});
            m.name = j.value("name", std::string{"Init"});
            m.author = j.value("author", std::string{});
            m.description = j.value("description", std::string{});
            m.category = j.value("category", std::string{});
            m.moods = j.value("moods", std::vector<std::string>{});
            m.genres = j.value("genres", std::vector<std::string>{});
            m.tags = j.value("tags", std::vector<std::string>{});
            m.createdAt = j.value("createdAt", std::string{});
            m.engineVersion = j.value("engineVersion", std::string(core::EngineVersion::string()));
            m.schemaVersion = clampNum(j.value("schemaVersion", core::kPatchSchemaVersion), 1, 1000);
            m.seed = j.value("seed", static_cast<std::uint64_t>(0));
            m.lineage = j.value("lineage", std::vector<std::string>{});

            // Bound collection sizes so a hostile document can't force huge allocations
            // via thousands of tiny tag strings.
            constexpr std::size_t kMaxListEntries = 64;
            if (m.moods.size() > kMaxListEntries) m.moods.resize(kMaxListEntries);
            if (m.genres.size() > kMaxListEntries) m.genres.resize(kMaxListEntries);
            if (m.tags.size() > kMaxListEntries) m.tags.resize(kMaxListEntries);
            if (m.lineage.size() > kMaxListEntries) m.lineage.resize(kMaxListEntries);
        }

        void toJson(json& j, const Macro& m)
        {
            j = json{{"id", m.id}, {"name", m.name}, {"description", m.description}, {"value", m.value}};
        }

        void fromJson(const json& j, Macro& m)
        {
            m.id = j.value("id", std::string{});
            m.name = j.value("name", std::string{});
            m.description = j.value("description", std::string{});
            m.value = clampNum(j.value("value", 0.0f), 0.0f, 1.0f);
        }

        void toJson(json& j, const UiFocusKnob& knob)
        {
            const char* kindStr = "macro";
            if (knob.kind == UiFocusKnobKind::Param)
                kindStr = "param";
            else if (knob.kind == UiFocusKnobKind::Morph)
                kindStr = "morph";
            j = json{{"kind", kindStr}};
            if (knob.kind == UiFocusKnobKind::Macro)
                j["index"] = knob.macroIndex;
            else if (knob.kind == UiFocusKnobKind::Param)
                j["paramId"] = knob.paramId;
            if (!knob.label.empty())
                j["label"] = knob.label;
        }

        void fromJson(const json& j, UiFocusKnob& knob)
        {
            const auto kindStr = j.value("kind", std::string{"macro"});
            if (kindStr == "param")
            {
                knob.kind = UiFocusKnobKind::Param;
                knob.paramId = j.value("paramId", std::string{});
            }
            else if (kindStr == "morph")
            {
                knob.kind = UiFocusKnobKind::Morph;
            }
            else
            {
                knob.kind = UiFocusKnobKind::Macro;
                knob.macroIndex = static_cast<std::size_t>(clampNum(j.value("index", 0), 0, 7));
            }
            knob.label = j.value("label", std::string{});
        }

        void toJson(json& j, const MorphKoinKeyframe& kf)
        {
            j = json{{"name", kf.name}};
            if (kf.position >= 0.0f && kf.position <= 1.0f)
                j["position"] = kf.position;
            if (kf.hasMacroValues)
            {
                json mv = json::array();
                for (float v : kf.macroValues)
                    mv.push_back(v);
                j["macroValues"] = mv;
            }
            if (!kf.paramOverrides.empty())
            {
                json po = json::object();
                for (const auto& [key, val] : kf.paramOverrides)
                    po[key] = val;
                j["paramOverrides"] = po;
            }
        }

        void fromJson(const json& j, MorphKoinKeyframe& kf)
        {
            kf.name = j.value("name", std::string{});
            kf.position = clampNum(j.value("position", 0.0f), 0.0f, 1.0f);
            kf.hasMacroValues = false;
            if (j.contains("macroValues") && j.at("macroValues").is_array())
            {
                kf.hasMacroValues = true;
                std::size_t i = 0;
                for (const auto& v : j.at("macroValues"))
                {
                    if (i >= kf.macroValues.size())
                        break;
                    kf.macroValues[i] = clampNum(v.get<float>(), 0.0f, 1.0f);
                    ++i;
                }
            }
            kf.paramOverrides.clear();
            if (j.contains("paramOverrides") && j.at("paramOverrides").is_object())
            {
                for (const auto& [key, val] : j.at("paramOverrides").items())
                    kf.paramOverrides[key] = val.get<float>();
            }
        }

        void toJson(json& j, const MorphKoin& morph)
        {
            j = json{{"label", morph.label},
                     {"defaultPosition", morph.defaultPosition},
                     {"position", morph.position},
                     {"curve", morph.curve},
                     {"wrap", morph.wrap}};
            if (!morph.description.empty())
                j["description"] = morph.description;
            json keyframes = json::array();
            for (const auto& kf : morph.keyframes)
            {
                json jk;
                toJson(jk, kf);
                keyframes.push_back(jk);
            }
            j["keyframes"] = keyframes;
        }

        void fromJson(const json& j, MorphKoin& morph)
        {
            morph.label = j.value("label", std::string{});
            morph.description = j.value("description", std::string{});
            morph.defaultPosition = clampNum(j.value("defaultPosition", 0.0f), 0.0f, 1.0f);
            morph.position = clampNum(j.value("position", morph.defaultPosition), 0.0f, 1.0f);
            morph.curve = j.value("curve", std::string{"linear"});
            morph.wrap = j.value("wrap", false);
            morph.keyframes.clear();
            if (!j.contains("keyframes") || !j.at("keyframes").is_array())
                return;
            for (const auto& jk : j.at("keyframes"))
            {
                if (morph.keyframes.size() >= 4)
                    break;
                MorphKoinKeyframe kf;
                fromJson(jk, kf);
                if (kf.name.empty())
                    continue;
                morph.keyframes.push_back(kf);
            }
        }

        void toJson(json& j, const PatchUiFocus& focus)
        {
            j = json{{"maxKnobs", focus.maxKnobs}};
            json knobs = json::array();
            for (const auto& knob : focus.knobs)
            {
                json jk;
                toJson(jk, knob);
                knobs.push_back(jk);
            }
            j["knobs"] = knobs;
        }

        void fromJson(const json& j, PatchUiFocus& focus)
        {
            focus.maxKnobs = static_cast<std::size_t>(clampNum(j.value("maxKnobs", 3), 1, 3));
            focus.knobs.clear();
            if (!j.contains("knobs") || !j.at("knobs").is_array())
                return;
            for (const auto& jk : j.at("knobs"))
            {
                UiFocusKnob knob;
                fromJson(jk, knob);
                if (knob.kind == UiFocusKnobKind::Param && knob.paramId.empty())
                    continue;
                focus.knobs.push_back(knob);
            }
        }

        void toJson(json& j, const sequencer::ArpStep& s)
        {
            j = json{{"enabled", s.enabled},               {"octaveOffset", s.octaveOffset},
                     {"gate", s.gate},                       {"probability", s.probability},
                     {"ratchetCount", s.ratchetCount},       {"tie", s.tie},
                     {"velocityScale", s.velocityScale},     {"accent", s.accent}};
        }

        void fromJson(const json& j, sequencer::ArpStep& s)
        {
            s.enabled = j.value("enabled", true);
            s.octaveOffset = clampNum(j.value("octaveOffset", 0), -2, 2);
            s.gate = clampNum(j.value("gate", 0.8f), 0.0f, 1.0f);
            s.probability = clampNum(j.value("probability", 1.0f), 0.0f, 1.0f);
            s.ratchetCount = clampNum(j.value("ratchetCount", 1), 1, sequencer::kMaxRatchet);
            s.tie = j.value("tie", false);
            s.velocityScale = clampNum(j.value("velocityScale", 1.0f), 0.0f, 1.0f);
            s.accent = j.value("accent", false);
        }

        void toJson(json& j, const sequencer::ArpeggiatorParams& a)
        {
            json steps = json::array();
            for (std::size_t i = 0; i < a.numSteps && i < sequencer::kMaxArpSteps; ++i)
            {
                json js;
                toJson(js, a.steps[i]);
                steps.push_back(js);
            }
            j = json{{"enabled", a.enabled},                 {"mode", static_cast<int>(a.mode)},
                     {"rateMode", static_cast<int>(a.rateMode)}, {"rateHz", a.rateHz},
                     {"syncDivisionIndex", a.syncDivisionIndex}, {"octaveRange", a.octaveRange},
                     {"numSteps", a.numSteps},                 {"latch", a.latch},
                     {"steps", steps}};
        }

        void fromJson(const json& j, sequencer::ArpeggiatorParams& a)
        {
            a.enabled = j.value("enabled", false);
            a.mode = static_cast<sequencer::ArpMode>(clampNum(j.value("mode", 0), 0, 6));
            a.rateMode = static_cast<sequencer::ArpRateMode>(clampNum(j.value("rateMode", 1), 0, 1));
            a.rateHz = clampNum(j.value("rateHz", 8.0f), 0.1f, 100.0f);
            a.syncDivisionIndex = clampNum(j.value("syncDivisionIndex", 6), 0, 9);
            a.octaveRange = clampNum(j.value("octaveRange", 1), 1, 4);
            a.latch = j.value("latch", false);

            a.steps = std::array<sequencer::ArpStep, sequencer::kMaxArpSteps>{};
            std::size_t stepCount = 0;
            if (j.contains("steps") && j.at("steps").is_array())
            {
                for (const auto& js : j.at("steps"))
                {
                    if (stepCount >= sequencer::kMaxArpSteps)
                        break;
                    fromJson(js, a.steps[stepCount]);
                    ++stepCount;
                }
            }
            const std::size_t requestedNumSteps = static_cast<std::size_t>(clampNum(
                j.value("numSteps", static_cast<int>(stepCount > 0 ? stepCount : 1)), 1,
                static_cast<int>(sequencer::kMaxArpSteps)));
            a.numSteps = std::max<std::size_t>(1, requestedNumSteps);
        }

    } // namespace

    std::string savePatchToJson(const Patch& patch, int indent) noexcept
    {
        try
        {
            json j;
            j["schemaVersion"] = patch.schemaVersion;

            json meta;
            toJson(meta, patch.metadata);
            j["metadata"] = meta;

            j["layerMode"] = static_cast<int>(patch.layerMode);
            j["layerMorph"] = patch.layerMorph;

            json a, b;
            toJson(a, patch.layerA);
            toJson(b, patch.layerB);
            j["layerA"] = a;
            j["layerB"] = b;

            j["voiceSettings"] = json{{"polyphony", patch.voiceSettings.polyphony},
                                       {"masterGain", patch.voiceSettings.masterGain},
                                       {"a4Hz", patch.voiceSettings.a4Hz},
                                       {"portamentoSeconds", patch.voiceSettings.portamentoSeconds},
                                       {"macroDissemination", patch.voiceSettings.macroDissemination},
                                       {"disseminationDepth", patch.voiceSettings.disseminationDepth}};

            j["locks"] = json{
                {"lockSources", patch.locks.lockSources},       {"lockAlgorithm", patch.locks.lockAlgorithm},
                {"lockFilters", patch.locks.lockFilters},       {"lockModulation", patch.locks.lockModulation},
                {"lockEffects", patch.locks.lockEffects},       {"lockSequence", patch.locks.lockSequence},
            };

            json macros = json::array();
            for (const auto& m : patch.macros)
            {
                json jm;
                toJson(jm, m);
                macros.push_back(jm);
            }
            j["macros"] = macros;

            if (!patch.morphKoin.keyframes.empty())
            {
                json morph;
                toJson(morph, patch.morphKoin);
                j["morphKoin"] = morph;
            }

            if (!patch.uiFocus.knobs.empty() || patch.uiFocus.maxKnobs != 3)
            {
                json focus;
                toJson(focus, patch.uiFocus);
                j["uiFocus"] = focus;
            }

            json arp;
            toJson(arp, patch.arpeggiator);
            j["arpeggiator"] = arp;

            json masterFx = json::array();
            for (const auto& fx : patch.masterEffects)
            {
                json jfx;
                toJson(jfx, fx);
                masterFx.push_back(jfx);
            }
            j["masterEffects"] = masterFx;

            j["seed"] = patch.seed;

            return indent >= 0 ? j.dump(indent) : j.dump();
        }
        catch (const std::exception&)
        {
            return {};
        }
    }

    namespace
    {
        /// Migrates an in-memory JSON document to the current schema version in-place.
        /// Each step only ever knows how to go from version N to N+1 (the chain
        /// contract described in docs/PATCH_FORMAT.md); the `if` below is written so
        /// a future v2->v3 step can simply be appended, not inserted.
        void migrateToCurrentSchema(json& root, int fromVersion) noexcept
        {
            if (fromVersion < 2)
            {
                // v1 -> v2 (docs/MODULATION.md "8 envelopes / 8 LFOs", core::Version.hpp):
                // LayerPatch's singular "ampEnvelope"/"lfo1" fields became 8-slot
                // "envelopes"/"lfos" arrays. The old single object becomes index 0; the
                // rest default. Applies independently to layerA and layerB -- if either
                // key is already the new array shape (or absent), this is a no-op for it.
                //
                // The SAME v1->v2 step also has to remap every existing "modRoutes"
                // entry's numeric "source" value: ModSource's enum ordinals shifted when
                // Lfo2-8/Env1-8 were inserted where AmpEnvelope/Velocity/etc. used to sit
                // (old: None=0,Lfo1=1,AmpEnvelope=2,Velocity=3,ChannelPressure=4,
                // PolyAftertouch=5,MpeSlide=6,Macro1-8=7-14; new:
                // None=0,Lfo1-8=1-8,Env1-8=9-16,Velocity=17,ChannelPressure=18,
                // PolyAftertouch=19,MpeSlide=20,Macro1-8=21-28). Without this, a v1
                // preset's "source": 2 (AmpEnvelope) would silently load as the new
                // enum's Lfo2, and "source": 3 (Velocity) as Lfo3 -- wrong destination,
                // not a load failure, so it would never surface as an error. Caught by
                // inspecting this project's own shipped presets (dark-bass.pw8,
                // wide-saw.pw8) before release, not by a user report.
                constexpr std::array<int, 15> kV1ToV2Source = {
                    0,  // None
                    1,  // Lfo1 -> Lfo1
                    9,  // AmpEnvelope -> Env1
                    17, // Velocity
                    18, // ChannelPressure
                    19, // PolyAftertouch
                    20, // MpeSlide
                    21, 22, 23, 24, 25, 26, 27, 28, // Macro1-8
                };

                try
                {
                    for (const char* layerKey : {"layerA", "layerB"})
                    {
                        if (!root.contains(layerKey) || !root.at(layerKey).is_object())
                            continue;
                        auto& layer = root.at(layerKey);

                        if (layer.contains("ampEnvelope") && !layer.contains("envelopes"))
                        {
                            json arr = json::array();
                            arr.push_back(layer.at("ampEnvelope"));
                            layer["envelopes"] = arr;
                        }
                        if (layer.contains("lfo1") && !layer.contains("lfos"))
                        {
                            json arr = json::array();
                            arr.push_back(layer.at("lfo1"));
                            layer["lfos"] = arr;
                        }

                        if (layer.contains("modRoutes") && layer.at("modRoutes").is_array())
                        {
                            for (auto& route : layer.at("modRoutes"))
                            {
                                if (!route.is_object() || !route.contains("source"))
                                    continue;
                                const int oldSource = route.at("source").get<int>();
                                if (oldSource >= 0 && oldSource < static_cast<int>(kV1ToV2Source.size()))
                                    route["source"] = kV1ToV2Source[static_cast<std::size_t>(oldSource)];
                            }
                        }
                    }
                }
                catch (const std::exception&)
                {
                    // A malformed layerA/layerB won't be fixed by migration -- fromJson's
                    // own defaulting (missing "envelopes"/"lfos" -> all-default array,
                    // an unparseable route -> clamped/defaulted ModRoute) still produces
                    // a valid, safe Patch either way.
                }
            }

            if (fromVersion < 3)
            {
                // v2 -> v3 (docs/DESIGN_AND_WARPS_PLAN.md §3.3): OperatorPatch wavetable
                // warp scalars (`wtBend`, `wtAsymmetry`, `wtSyncRatio`, `wtSyncAmount`,
                // `wtFormantShift`). No JSON rewrite required — fromJson defaults apply
                // on load (all zeros / wtSyncRatio=1). Existing presets remain transparent.
            }
        }
    } // namespace

    PatchLoadResult loadPatchFromJson(std::string_view jsonText) noexcept
    {
        PatchLoadResult result;

        if (jsonText.size() > kMaxJsonInputBytes)
        {
            result.error = "Patch JSON exceeds maximum accepted size";
            return result;
        }

        json root;
        try
        {
            root = json::parse(jsonText, /*cb=*/nullptr, /*allow_exceptions=*/true);
        }
        catch (const json::parse_error& e)
        {
            result.error = std::string("JSON parse error: ") + e.what();
            return result;
        }

        if (!root.is_object())
        {
            result.error = "Patch root is not a JSON object";
            return result;
        }

        try
        {
            const int originalSchema = clampNum(root.value("schemaVersion", core::kPatchSchemaVersion), 1, 1000);
            result.originalSchemaVersion = originalSchema;
            migrateToCurrentSchema(root, originalSchema);

            Patch p;
            p.schemaVersion = core::kPatchSchemaVersion;

            if (root.contains("metadata"))
                fromJson(root.at("metadata"), p.metadata);

            p.layerMode = static_cast<LayerMode>(clampNum(root.value("layerMode", 0), 0, 6));
            p.layerMorph = clampNum(root.value("layerMorph", 0.0f), 0.0f, 1.0f);

            if (root.contains("layerA"))
                fromJson(root.at("layerA"), p.layerA);
            if (root.contains("layerB"))
                fromJson(root.at("layerB"), p.layerB);

            if (root.contains("voiceSettings"))
            {
                const auto& vs = root.at("voiceSettings");
                p.voiceSettings.polyphony = static_cast<std::size_t>(
                    clampNum(vs.value("polyphony", static_cast<int>(core::kDefaultVoices)), 1,
                             static_cast<int>(core::kMaxVoices)));
                p.voiceSettings.masterGain = clampNum(vs.value("masterGain", 1.0f), 0.0f, 4.0f);
                p.voiceSettings.a4Hz = clampNum(vs.value("a4Hz", 440.0f), 220.0f, 880.0f);
                p.voiceSettings.portamentoSeconds = clampNum(vs.value("portamentoSeconds", 0.0f), 0.0f, 10.0f);
                p.voiceSettings.macroDissemination = vs.value("macroDissemination", false);
                p.voiceSettings.disseminationDepth =
                    clampNum(vs.value("disseminationDepth", 0.22f), 0.0f, 1.0f);
            }

            if (root.contains("locks"))
            {
                const auto& lk = root.at("locks");
                p.locks.lockSources = lk.value("lockSources", false);
                p.locks.lockAlgorithm = lk.value("lockAlgorithm", false);
                p.locks.lockFilters = lk.value("lockFilters", false);
                p.locks.lockModulation = lk.value("lockModulation", false);
                p.locks.lockEffects = lk.value("lockEffects", false);
                p.locks.lockSequence = lk.value("lockSequence", false);
            }

            if (root.contains("macros") && root.at("macros").is_array())
            {
                std::size_t i = 0;
                for (const auto& jm : root.at("macros"))
                {
                    if (i >= p.macros.size())
                        break;
                    fromJson(jm, p.macros[i]);
                    ++i;
                }
            }

            if (root.contains("uiFocus"))
                fromJson(root.at("uiFocus"), p.uiFocus);

            if (root.contains("morphKoin"))
                fromJson(root.at("morphKoin"), p.morphKoin);

            if (root.contains("arpeggiator"))
                fromJson(root.at("arpeggiator"), p.arpeggiator);

            p.masterEffects = std::array<effects::EffectSlotParams, effects::kNumMasterSlots>{};
            if (root.contains("masterEffects") && root.at("masterEffects").is_array())
            {
                std::size_t i = 0;
                for (const auto& jfx : root.at("masterEffects"))
                {
                    if (i >= effects::kNumMasterSlots)
                        break;
                    fromJson(jfx, p.masterEffects[i]);
                    ++i;
                }
            }

            p.seed = root.value("seed", static_cast<std::uint64_t>(0));

            result.ok = true;
            result.patch = std::move(p);
            return result;
        }
        catch (const std::exception& e)
        {
            result.error = std::string("Patch structure error: ") + e.what();
            result.ok = false;
            return result;
        }
    }

} // namespace pw8::patch
