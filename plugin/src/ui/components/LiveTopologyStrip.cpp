#include "LiveTopologyStrip.h"

#include <cmath>

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "EngineIconGrid.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr float kNodeRadius = 11.0f;
        constexpr int kNumNodes = 8;
    } // namespace

    LiveTopologyStrip::LiveTopologyStrip(MurmurProcessor& processor) : processor_(processor)
    {
        startTimerHz(24);
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    }

    LiveTopologyStrip::~LiveTopologyStrip()
    {
        stopTimer();
    }

    void LiveTopologyStrip::setSelectedNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, kNumNodes - 1, nodeIndex);
        repaint();
    }

    void LiveTopologyStrip::setActiveOperator(int nodeIndex)
    {
        activeOperator_ = juce::jlimit(0, kNumNodes - 1, nodeIndex);
        repaint();
    }

    void LiveTopologyStrip::triggerPerformancePulse()
    {
        performancePulse_ = 1.0f;
        repaint();
    }

    void LiveTopologyStrip::timerCallback()
    {
        pulsePhase_ += 1.0f / 24.0f * 0.45f;
        if (pulsePhase_ > 1.0f)
            pulsePhase_ -= 1.0f;

        if (performancePulse_ > 0.0f)
        {
            performancePulse_ = juce::jmax(0.0f, performancePulse_ - 0.06f);
            repaint();
        }
        else
        {
            repaint();
        }
    }

    juce::Point<float> LiveTopologyStrip::nodePosition(std::size_t index, juce::Rectangle<float> area) const
    {
        const float inset = kNodeRadius + 6.0f;
        const float usable = area.getWidth() - inset * 2.0f;
        const float x = area.getX() + inset + (static_cast<float>(index) / static_cast<float>(kNumNodes - 1)) * usable;
        return {x, area.getCentreY()};
    }

    bool LiveTopologyStrip::edgeTouchesNode(const algorithm::AlgorithmEdge& edge, int nodeIndex,
                                            const algorithm::AlgorithmGraphDefinition& algo) const
    {
        if (nodeIndex < 0 || static_cast<std::size_t>(nodeIndex) >= algo.nodes.size())
            return false;
        const auto nodeId = algo.nodes[static_cast<std::size_t>(nodeIndex)].id;
        return edge.source == nodeId || edge.destination == nodeId;
    }

    void LiveTopologyStrip::mouseDown(const juce::MouseEvent& event)
    {
        auto area = getLocalBounds().toFloat().reduced(8.0f, 6.0f);
        area.removeFromTop(14.0f);

        for (int i = 0; i < kNumNodes; ++i)
        {
            const auto p = nodePosition(static_cast<std::size_t>(i), area);
            if (p.getDistanceFrom(event.position) <= kNodeRadius + 2.0f)
            {
                setSelectedNode(i);
                if (onNodeSelected)
                    onNodeSelected(i);
                return;
            }
        }

        if (onExpandRequested)
            onExpandRequested();
    }

    void LiveTopologyStrip::paint(juce::Graphics& g)
    {
        const auto bounds = getLocalBounds().toFloat();
        const auto frame = bounds.reduced(1.0f);

        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(frame, 6.0f);
        g.setColour(palette::kAccent.withAlpha(0.22f));
        g.drawRoundedRectangle(frame, 6.0f, 1.0f);

        auto header = bounds.reduced(10.0f, 6.0f);
        header.setHeight(14.0f);
        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(10.0f));
        g.drawText("LIVE TOPOLOGY — tap to expand", header.toNearestInt(), juce::Justification::centredLeft);

        auto graphArea = bounds.reduced(8.0f, 6.0f);
        graphArea.removeFromTop(14.0f);

        const auto& patch = processor_.getCurrentPatch();
        const auto& algo = patch.layerA.algorithm;
        const float pulseBoost = performancePulse_ * 0.45f;

        for (std::size_t e = 0; e < algo.edges.size(); ++e)
        {
            const auto& edge = algo.edges[e];
            std::size_t srcIdx = algo.nodes.size(), dstIdx = algo.nodes.size();
            for (std::size_t i = 0; i < algo.nodes.size(); ++i)
            {
                if (algo.nodes[i].id == edge.source) srcIdx = i;
                if (algo.nodes[i].id == edge.destination) dstIdx = i;
            }
            if (srcIdx >= algo.nodes.size() || dstIdx >= algo.nodes.size())
                continue;

            const bool activeEdge = edgeTouchesNode(edge, activeOperator_, algo);
            const auto colour = palette::edgeColour(static_cast<int>(edge.type));
            const float alpha = (activeEdge ? 0.75f : 0.35f) + pulseBoost;
            const float edgePhase = std::fmod(pulsePhase_ + static_cast<float>(e) * 0.11f, 1.0f);

            const auto p1 = nodePosition(srcIdx, graphArea);
            const auto p2 = nodePosition(dstIdx, graphArea);
            const auto mid = (p1 + p2) * 0.5f;
            const auto bow = mid.translated(0.0f, -juce::jmin(18.0f, graphArea.getHeight() * 0.35f));

            juce::Path path;
            path.startNewSubPath(p1);
            path.quadraticTo(bow, p2);

            g.setColour(colour.withAlpha(juce::jlimit(0.0f, 1.0f, alpha)));
            g.strokePath(path, juce::PathStrokeType(activeEdge ? 2.0f : 1.2f));

            if (activeEdge || performancePulse_ > 0.05f)
            {
                const auto dot = path.getPointAlongPath(edgePhase * path.getLength());
                g.setColour(colour.withAlpha(juce::jlimit(0.0f, 1.0f, 0.55f + pulseBoost)));
                g.fillEllipse(dot.x - 2.0f, dot.y - 2.0f, 4.0f, 4.0f);
            }
        }

        for (int i = 0; i < kNumNodes; ++i)
        {
            const auto engine = patch.layerA.operators[static_cast<std::size_t>(i)].engine;
            const bool isOutput =
                i < static_cast<int>(algo.nodes.size()) ? algo.nodes[static_cast<std::size_t>(i)].isOutput : true;
            const auto p = nodePosition(static_cast<std::size_t>(i), graphArea);
            const bool selected = i == selectedNode_;
            const float alphaMul = isOutput ? 1.0f : 0.45f;

            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.28f * alphaMul));
                g.fillEllipse(p.x - kNodeRadius * 1.35f, p.y - kNodeRadius * 1.35f, kNodeRadius * 2.7f,
                              kNodeRadius * 2.7f);
            }

            g.setColour(palette::kPanel.withAlpha(alphaMul));
            g.fillEllipse(p.x - kNodeRadius, p.y - kNodeRadius, kNodeRadius * 2.0f, kNodeRadius * 2.0f);
            g.setColour((isOutput ? palette::kAccent : palette::kBorderBright).withAlpha(alphaMul));
            g.drawEllipse(p.x - kNodeRadius, p.y - kNodeRadius, kNodeRadius * 2.0f, kNodeRadius * 2.0f,
                          selected ? 1.8f : 1.2f);

            const auto iconBounds =
                juce::Rectangle<float>(p.x - 7.0f, p.y - 7.0f, 14.0f, 14.0f);
            engineicons::drawEngineIcon(g, engine, iconBounds, palette::kAccent.withAlpha(0.9f * alphaMul), 1.0f);
        }
    }

} // namespace pw8::plugin::ui
