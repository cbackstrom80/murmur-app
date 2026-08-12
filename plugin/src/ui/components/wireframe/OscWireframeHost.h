#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../WavetableStackView.h"
#include "EngineWireframeViews.h"
#include "processor/PatchworkEightProcessor.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    /// Swaps the active engine wireframe preview for the selected operator.
    class OscWireframeHost : public juce::Component
    {
    public:
        explicit OscWireframeHost(PatchworkEightProcessor& processor);

        void resized() override;
        void showNode(int nodeIndex);
        void setEngine(algorithm::EngineType engine);

        void ensureDefaultWavetableLoaded();
        void setGranularOverlay(bool enabled) { wavetableView_.setGranularOverlay(enabled); }

    private:
        void showViewForEngine(algorithm::EngineType engine);

        PatchworkEightProcessor& processor_;
        int nodeIndex_ = 0;
        algorithm::EngineType engine_ = algorithm::EngineType::Classic;

        WavetableStackView wavetableView_;
        wireframe::ClassicWireframeView classicView_;
        wireframe::FmWireframeView fmView_;
        wireframe::AdditiveWireframeView additiveView_;
        wireframe::PhaseShapeWireframeView phaseView_;
        wireframe::ResonatorWireframeView resonatorView_;
        wireframe::NoiseWireframeView noiseView_;
    };

} // namespace pw8::plugin::ui
