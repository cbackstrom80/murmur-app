#include "NodeSelectorRow.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNumNodes = 8;

        juce::String engineLabel(algorithm::EngineType engine)
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic: return "CLS";
                case algorithm::EngineType::Wavetable: return "WT";
                case algorithm::EngineType::FmPm: return "FM";
                case algorithm::EngineType::Additive: return "ADD";
                case algorithm::EngineType::PhaseShape: return "PHS";
                case algorithm::EngineType::Granular: return "GRN";
                case algorithm::EngineType::NoiseChaos: return "NSE";
                case algorithm::EngineType::Resonator: return "RES";
                default: return "?";
            }
        }
    } // namespace

    NodeSelectorRow::NodeSelectorRow(PatchworkEightProcessor& processor) : processor_(processor) {}

    void NodeSelectorRow::setSelectedNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, kNumNodes - 1, nodeIndex);
        repaint();
    }

    juce::Rectangle<int> NodeSelectorRow::pillBounds(int nodeIndex) const
    {
        auto row = getLocalBounds().reduced(2, 4);
        const int pillWidth = row.getWidth() / kNumNodes;
        return row.withX(row.getX() + pillWidth * nodeIndex).withWidth(pillWidth).reduced(2, 0);
    }

    void NodeSelectorRow::paint(juce::Graphics& g)
    {
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText("OPERATORS", getLocalBounds().removeFromLeft(72), juce::Justification::centredLeft);

        const auto& patch = processor_.getCurrentPatch();
        for (int i = 0; i < kNumNodes; ++i)
        {
            const auto bounds = pillBounds(i);
            const auto engine = patch.layerA.operators[static_cast<std::size_t>(i)].engine;
            const bool selected = i == selectedNode_;
            const bool implemented = algorithm::isEngineImplemented(engine);

            g.setColour(selected ? palette::kAccent.withAlpha(0.25f) : palette::kPanelRaised);
            g.fillRoundedRectangle(bounds.toFloat(), 4.0f);
            g.setColour(selected ? palette::kAccent : palette::kBorderBright);
            g.drawRoundedRectangle(bounds.toFloat(), 4.0f, selected ? 1.5f : 1.0f);

            g.setColour(implemented ? palette::kTextPrimary : palette::kTextDim);
            g.setFont(fonts::label(10.0f));
            const juce::String label = juce::String(i) + " " + engineLabel(engine);
            g.drawText(label, bounds, juce::Justification::centred);
        }
    }

    void NodeSelectorRow::resized() {}

    void NodeSelectorRow::mouseDown(const juce::MouseEvent& event)
    {
        for (int i = 0; i < kNumNodes; ++i)
        {
            if (!pillBounds(i).contains(event.getPosition()))
                continue;
            setSelectedNode(i);
            if (onNodeSelected)
                onNodeSelected(i);
            return;
        }
    }

} // namespace pw8::plugin::ui
