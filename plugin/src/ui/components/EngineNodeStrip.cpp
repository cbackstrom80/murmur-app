#include "EngineNodeStrip.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "EngineIconGrid.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kNumNodes = 8;
        constexpr int kGlobalWidth = 84;
    } // namespace

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

    EngineNodeStrip::EngineNodeStrip(PatchworkEightProcessor& processor) : processor_(processor) {}

    void EngineNodeStrip::setGlobalPillVisible(bool visible)
    {
        globalPillVisible_ = visible;
        repaint();
    }

    void EngineNodeStrip::setSelectedNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, kNumNodes - 1, nodeIndex);
        globalScope_ = false;
        repaint();
    }

    void EngineNodeStrip::setGlobalScope(bool global)
    {
        globalScope_ = global;
        repaint();
    }

    juce::Rectangle<int> EngineNodeStrip::pillBounds(int nodeIndex) const
    {
        auto row = getLocalBounds().reduced(2, 4);
        if (globalPillVisible_)
            row.removeFromRight(kGlobalWidth + 6);
        const int pillWidth = row.getWidth() / kNumNodes;
        return row.withX(row.getX() + pillWidth * nodeIndex).withWidth(pillWidth).reduced(2, 0);
    }

    juce::Rectangle<int> EngineNodeStrip::globalPillBounds() const
    {
        auto row = getLocalBounds().reduced(2, 4);
        return row.removeFromRight(kGlobalWidth);
    }

    void EngineNodeStrip::paint(juce::Graphics& g)
    {
        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(fonts::kBodyLabelSize));
        g.drawText("OPERATORS", getLocalBounds().removeFromLeft(72), juce::Justification::centredLeft);

        const auto& patch = processor_.getCurrentPatch();
        const auto& algo = patch.layerA.algorithm;
        for (int i = 0; i < kNumNodes; ++i)
        {
            const auto bounds = pillBounds(i);
            const auto engine = patch.layerA.operators[static_cast<std::size_t>(i)].engine;
            const bool isOutput =
                i < static_cast<int>(algo.nodes.size()) ? algo.nodes[static_cast<std::size_t>(i)].isOutput : true;
            const auto boundsF = bounds.toFloat();
            const bool selected = !globalScope_ && i == selectedNode_;
            const bool implemented = algorithm::isEngineImplemented(engine);
            const float alphaMul = isOutput ? 1.0f : 0.45f;

            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.18f * alphaMul));
                g.fillRoundedRectangle(boundsF, 4.0f);
                draw::strokeGlowPath(g, draw::roundedRectPath(boundsF.reduced(0.5f), 4.0f), 0.95f, 1.3f, true);
            }
            else
            {
                draw::fillRecessedRoundedRect(g, boundsF.reduced(0.5f), 4.0f);
                g.setColour(palette::kBorder.withAlpha(alphaMul));
                g.drawRoundedRectangle(boundsF.reduced(0.5f), 4.0f, 1.0f);
            }

            if (isOutput)
            {
                g.setColour(palette::kAccent.withAlpha(0.85f));
                g.fillEllipse(boundsF.getX() + 4.0f, boundsF.getCentreY() - 2.0f, 4.0f, 4.0f);
            }

            g.setColour((implemented ? palette::kTextPrimary : palette::kTextDim).withAlpha(alphaMul));
            g.setFont(fonts::label(10.0f));

            auto textBounds = bounds;
            const auto iconBounds = textBounds.removeFromLeft(18).toFloat().reduced(2.0f, 4.0f);
            engineicons::drawEngineIcon(g, engine, iconBounds, palette::kAccent.withAlpha(0.85f * alphaMul), 1.2f);

            const juce::String label = juce::String(i) + " " + engineLabel(engine);
            g.drawText(label, textBounds, juce::Justification::centred);
        }

        if (globalPillVisible_)
        {
            const auto bounds = globalPillBounds();
            const auto boundsF = bounds.toFloat();
            const bool selected = globalScope_;
            if (selected)
            {
                g.setColour(palette::kAccentWarm.withAlpha(0.2f));
                g.fillRoundedRectangle(boundsF, 4.0f);
                juce::Path outline;
                outline.addRoundedRectangle(boundsF.reduced(0.5f), 4.0f);
                g.setColour(palette::kAccentWarm.withAlpha(0.25f));
                g.strokePath(outline, juce::PathStrokeType(2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour(palette::kAccentWarm.withAlpha(0.95f));
                g.strokePath(outline, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
            else
            {
                draw::fillRecessedRoundedRect(g, boundsF.reduced(0.5f), 4.0f);
                g.setColour(palette::kBorder);
                g.drawRoundedRectangle(boundsF.reduced(0.5f), 4.0f, 1.0f);
            }
            g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
            g.setFont(fonts::label(10.0f));
            g.drawText("GLOBAL", bounds, juce::Justification::centred);
        }
    }

    void EngineNodeStrip::resized() {}

    void EngineNodeStrip::mouseDown(const juce::MouseEvent& event)
    {
        if (globalPillVisible_ && globalPillBounds().contains(event.getPosition()))
        {
            globalScope_ = true;
            repaint();
            if (onGlobalSelected)
                onGlobalSelected();
            return;
        }

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
