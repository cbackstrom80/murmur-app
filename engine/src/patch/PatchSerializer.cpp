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
            r.source = static_cast<modulation::ModSource>(clampNum(j.value("source", 0), 0, 14));
            r.destination = static_cast<modulation::ModDestination>(clampNum(j.value("destination", 0), 0, 5));
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
            };
        }

        void fromJson(const json& j, effects::EffectSlotParams& e)
        {
            e.type = static_cast<effects::EffectType>(clampNum(j.value("type", 0), 0, 6));
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
            json algo, env, uni, filt, lfoJ;
            toJson(algo, l.algorithm);
            toJson(env, l.ampEnvelope);
            toJson(uni, l.unison);
            toJson(filt, l.filter1);
            toJson(lfoJ, l.lfo1);

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

            j = json{{"operators", ops},       {"algorithm", algo},           {"ampEnvelope", env},
                     {"unison", uni},           {"filter1", filt},             {"lfo1", lfoJ},
                     {"modRoutes", routes},     {"gain", l.gain},              {"pan", l.pan},
                     {"width", l.width},         {"centerGravity", l.centerGravity},
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
            if (j.contains("ampEnvelope"))
                fromJson(j.at("ampEnvelope"), l.ampEnvelope);
            if (j.contains("unison"))
                fromJson(j.at("unison"), l.unison);
            if (j.contains("filter1"))
                fromJson(j.at("filter1"), l.filter1);
            if (j.contains("lfo1"))
                fromJson(j.at("lfo1"), l.lfo1);

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
                                       {"a4Hz", patch.voiceSettings.a4Hz}};

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
        /// Currently a no-op (only schema v1 exists) but establishes the migration chain
        /// contract described in docs/PATCH_FORMAT.md: each step only ever knows how to
        /// go from version N to N+1, and the loop below applies them in sequence.
        void migrateToCurrentSchema(json& /*root*/, int /*fromVersion*/) noexcept
        {
            // v1 -> v2 migration would go here once schema v2 exists.
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
