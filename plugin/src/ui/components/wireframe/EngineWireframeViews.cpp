#include "EngineWireframeViews.h"

#include "state/PluginState.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] float loadParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& id,
                                       float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(id))
                return raw->load();
            return fallback;
        }
    } // namespace

    ClassicWireframeView::ClassicWireframeView(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(8);
    }

    ClassicWireframeView::~ClassicWireframeView() { stopTimer(); }

    void ClassicWireframeView::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        refresh();
        repaint();
    }

    void ClassicWireframeView::timerCallback()
    {
        refresh();
        repaint();
    }

    void ClassicWireframeView::refresh()
    {
        activeShape_ = static_cast<int>(loadParam(processor_.apvts,
                                                   operatorParamId(static_cast<std::size_t>(nodeIndex_), "Waveform")));
        for (int w = 0; w < 4; ++w)
        {
            ClassicPreviewParams p;
            p.waveform = OscPreviewSampler::waveformFromOrdinal(w);
            OscPreviewSampler::sampleClassicCycle(p, shapeRows_[static_cast<std::size_t>(w)]);
        }
        setCaption("CLASSIC · cycle stack");
        setSubCaption("Front row = active waveform");
    }

    void ClassicWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        MeshDrawConfig config;
        config.meshRows = 4;
        config.liveRowIndex = juce::jlimit(0, 3, activeShape_);
        paintDepthMesh(g, bounds, config,
                       [&](float depthT, int p)
                       {
                           const int row = juce::jlimit(0, 3, static_cast<int>(depthT * 3.0f + 0.5f));
                           return shapeRows_[static_cast<std::size_t>(row)][static_cast<std::size_t>(p)];
                       });
    }

    FmWireframeView::FmWireframeView(MurmurProcessor& processor) : processor_(processor) { startTimerHz(8); }

    FmWireframeView::~FmWireframeView() { stopTimer(); }

    void FmWireframeView::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        refresh();
        repaint();
    }

    void FmWireframeView::timerCallback()
    {
        refresh();
        repaint();
    }

    void FmWireframeView::refresh()
    {
        const auto op = static_cast<std::size_t>(nodeIndex_);
        FmPreviewParams p;
        p.carrier.waveform =
            OscPreviewSampler::waveformFromOrdinal(static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "Waveform"))));
        p.modulator.waveform = OscPreviewSampler::waveformFromOrdinal(
            static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "FmModulatorWaveform"))));
        p.modRatio = loadParam(processor_.apvts, operatorParamId(op, "FmModulatorRatio"), 1.0f);
        p.modIndex = loadParam(processor_.apvts, operatorParamId(op, "FmModulatorIndex"), 1.0f);
        OscPreviewSampler::sampleFmLayers(p, carrier_, mod_);
        setCaption("FM/PM · carrier + modulator");
        setSubCaption("Mod index scales phase depth in preview");
    }

    void FmWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        auto top = bounds.removeFromTop(bounds.getHeight() * 0.48f);
        auto bottom = bounds;

        MeshDrawConfig modCfg;
        modCfg.meshRows = 1;
        modCfg.liveRowIndex = 0;
        paintDepthMesh(g, top, modCfg,
                       [&](float, int p) { return mod_[static_cast<std::size_t>(p)]; });

        MeshDrawConfig carCfg;
        carCfg.meshRows = 1;
        carCfg.liveRowIndex = 0;
        paintDepthMesh(g, bottom, carCfg,
                       [&](float, int p) { return carrier_[static_cast<std::size_t>(p)]; });
    }

    AdditiveWireframeView::AdditiveWireframeView(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(8);
    }

    AdditiveWireframeView::~AdditiveWireframeView() { stopTimer(); }

    void AdditiveWireframeView::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        refresh();
        repaint();
    }

    void AdditiveWireframeView::timerCallback()
    {
        refresh();
        repaint();
    }

    void AdditiveWireframeView::refresh()
    {
        const auto op = static_cast<std::size_t>(nodeIndex_);
        AdditivePreviewParams p;
        p.partialCount = static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "AdditivePartialCount"), 32.0f));
        p.tilt = loadParam(processor_.apvts, operatorParamId(op, "AdditiveTilt"));
        p.oddEven = loadParam(processor_.apvts, operatorParamId(op, "AdditiveOddEven"), 0.5f);
        p.stretch = loadParam(processor_.apvts, operatorParamId(op, "AdditiveStretch"));
        OscPreviewSampler::computeAdditiveHeights(p, heights_, partialCount_);
        setCaption("ADDITIVE · partial landscape");
        setSubCaption(juce::String(partialCount_) + " partials · tilt/odd-even/stretch");
    }

    void AdditiveWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        paintBarLandscape(g, bounds, partialCount_,
                          [&](int i) { return heights_[static_cast<std::size_t>(i)]; });
    }

    PhaseShapeWireframeView::PhaseShapeWireframeView(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(8);
    }

    PhaseShapeWireframeView::~PhaseShapeWireframeView() { stopTimer(); }

    void PhaseShapeWireframeView::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        refresh();
        repaint();
    }

    void PhaseShapeWireframeView::timerCallback()
    {
        refresh();
        repaint();
    }

    void PhaseShapeWireframeView::refresh()
    {
        const auto op = static_cast<std::size_t>(nodeIndex_);
        PhaseShapePreviewParams p;
        p.phaseBend = loadParam(processor_.apvts, operatorParamId(op, "PhaseBend"));
        p.phaseFold = loadParam(processor_.apvts, operatorParamId(op, "PhaseFold"));
        p.phaseAsymmetry = loadParam(processor_.apvts, operatorParamId(op, "PhaseAsymmetry"));
        p.phaseShape = loadParam(processor_.apvts, operatorParamId(op, "PhaseShape"));
        OscPreviewSampler::samplePhaseShapeCycle(p, output_, warp_);
        setCaption("PHASE SHAPE · warp + output");
        setSubCaption("Top: phase map · bottom: folded output");
    }

    void PhaseShapeWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        auto warpArea = bounds.removeFromTop(bounds.getHeight() * 0.42f).reduced(2.0f);
        auto outArea = bounds.reduced(2.0f);

        paintFlatWaveform(g, warpArea, kPreviewPoints,
                          [&](float t)
                          {
                              const int idx = juce::jlimit(0, kPreviewPoints - 1,
                                                            static_cast<int>(t * static_cast<float>(kPreviewPoints - 1)));
                              return warp_[static_cast<std::size_t>(idx)];
                          },
                          false);
        paintFlatWaveform(g, outArea, kPreviewPoints,
                          [&](float t)
                          {
                              const int idx = juce::jlimit(0, kPreviewPoints - 1,
                                                            static_cast<int>(t * static_cast<float>(kPreviewPoints - 1)));
                              return output_[static_cast<std::size_t>(idx)];
                          });
    }

    ResonatorWireframeView::ResonatorWireframeView(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(8);
    }

    ResonatorWireframeView::~ResonatorWireframeView() { stopTimer(); }

    void ResonatorWireframeView::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        refresh();
        repaint();
    }

    void ResonatorWireframeView::timerCallback()
    {
        refresh();
        repaint();
    }

    void ResonatorWireframeView::refresh()
    {
        const auto op = static_cast<std::size_t>(nodeIndex_);
        ResonatorPreviewParams p;
        p.structure = loadParam(processor_.apvts, operatorParamId(op, "ResonatorStructure"), 0.3f);
        p.decay = loadParam(processor_.apvts, operatorParamId(op, "ResonatorDecay"), 0.5f);
        p.damping = loadParam(processor_.apvts, operatorParamId(op, "ResonatorDamping"), 0.5f);
        p.brightness = loadParam(processor_.apvts, operatorParamId(op, "ResonatorBrightness"), 0.5f);
        p.modeCount = static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "ResonatorModeCount"), 6.0f));
        OscPreviewSampler::computeResonatorHeights(p, heights_, modeCount_);
        setCaption("RESONATOR · modal towers");
        setSubCaption(juce::String(modeCount_) + " modes · structure/decay/damping");
    }

    void ResonatorWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        paintBarLandscape(g, bounds, modeCount_,
                          [&](int i) { return heights_[static_cast<std::size_t>(i)]; });
    }

    NoiseWireframeView::NoiseWireframeView(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(8);
    }

    NoiseWireframeView::~NoiseWireframeView() { stopTimer(); }

    void NoiseWireframeView::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        refresh();
        repaint();
    }

    void NoiseWireframeView::timerCallback()
    {
        refresh();
        repaint();
    }

    void NoiseWireframeView::refresh()
    {
        const auto op = static_cast<std::size_t>(nodeIndex_);
        NoisePreviewParams p;
        p.variant = OscPreviewSampler::noiseVariantFromOrdinal(
            static_cast<int>(loadParam(processor_.apvts, operatorParamId(op, "NoiseVariant"))));
        p.rateHz = loadParam(processor_.apvts, operatorParamId(op, "NoiseRate"), 10.0f);
        p.seed = static_cast<std::uint64_t>(nodeIndex_) * 0x9E3779B97F4A7C15ULL + 1ULL;
        OscPreviewSampler::sampleNoiseTrace(p, samples_);
        setCaption("NOISE/CHAOS · stochastic trace");
        setSubCaption("Representative preview (seed-stable per operator)");
    }

    void NoiseWireframeView::paint(juce::Graphics& g)
    {
        auto bounds = meshBounds();
        paintSubCaption(g, bounds);
        paintFlatWaveform(g, bounds, kNoisePoints,
                          [&](float t)
                          {
                              const int idx =
                                  juce::jlimit(0, kNoisePoints - 1, static_cast<int>(t * static_cast<float>(kNoisePoints - 1)));
                              return samples_[static_cast<std::size_t>(idx)];
                          });
    }

} // namespace pw8::plugin::ui::wireframe
