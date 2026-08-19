#include "OscWireframeHost.h"

namespace pw8::plugin::ui
{
    OscWireframeHost::OscWireframeHost(MurmurProcessor& processor)
        : processor_(processor),
          wavetableView_(processor),
          classicView_(processor),
          fmView_(processor),
          additiveView_(processor),
          phaseView_(processor),
          resonatorView_(processor),
          noiseView_(processor)
    {
        addAndMakeVisible(wavetableView_);
        addAndMakeVisible(classicView_);
        addAndMakeVisible(fmView_);
        addAndMakeVisible(additiveView_);
        addAndMakeVisible(phaseView_);
        addAndMakeVisible(resonatorView_);
        addAndMakeVisible(noiseView_);
        showViewForEngine(algorithm::EngineType::Classic);
    }

    void OscWireframeHost::resized()
    {
        const auto bounds = getLocalBounds();
        wavetableView_.setBounds(bounds);
        classicView_.setBounds(bounds);
        fmView_.setBounds(bounds);
        additiveView_.setBounds(bounds);
        phaseView_.setBounds(bounds);
        resonatorView_.setBounds(bounds);
        noiseView_.setBounds(bounds);
    }

    void OscWireframeHost::showNode(int nodeIndex)
    {
        nodeIndex_ = juce::jlimit(0, 7, nodeIndex);
        wavetableView_.showNode(nodeIndex_);
        classicView_.showNode(nodeIndex_);
        fmView_.showNode(nodeIndex_);
        additiveView_.showNode(nodeIndex_);
        phaseView_.showNode(nodeIndex_);
        resonatorView_.showNode(nodeIndex_);
        noiseView_.showNode(nodeIndex_);
    }

    void OscWireframeHost::setEngine(algorithm::EngineType engine)
    {
        engine_ = engine;
        showViewForEngine(engine);
    }

    void OscWireframeHost::ensureDefaultWavetableLoaded()
    {
        wavetableView_.ensureDefaultWavetableLoaded();
    }

    void OscWireframeHost::showViewForEngine(algorithm::EngineType engine)
    {
        wavetableView_.setVisible(engine == algorithm::EngineType::Wavetable || engine == algorithm::EngineType::Granular);
        classicView_.setVisible(engine == algorithm::EngineType::Classic);
        fmView_.setVisible(engine == algorithm::EngineType::FmPm);
        additiveView_.setVisible(engine == algorithm::EngineType::Additive);
        phaseView_.setVisible(engine == algorithm::EngineType::PhaseShape);
        resonatorView_.setVisible(engine == algorithm::EngineType::Resonator);
        noiseView_.setVisible(engine == algorithm::EngineType::NoiseChaos);

        setGranularOverlay(engine == algorithm::EngineType::Granular);
    }

} // namespace pw8::plugin::ui
