#include "AlgorithmGraphView.h"

#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        const char* engineShortName(algorithm::EngineType engine) noexcept
        {
            switch (engine)
            {
                case algorithm::EngineType::Classic: return "CLSC";
                case algorithm::EngineType::Wavetable: return "WT";
                case algorithm::EngineType::FmPm: return "FM";
                case algorithm::EngineType::Additive: return "ADD";
                case algorithm::EngineType::PhaseShape: return "PHSH";
                case algorithm::EngineType::Granular: return "GRAN";
                case algorithm::EngineType::NoiseChaos: return "NOIS";
                case algorithm::EngineType::Resonator: return "RESO";
            }
            return "?";
        }
    } // namespace

    AlgorithmGraphView::AlgorithmGraphView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(24);
    }

    AlgorithmGraphView::~AlgorithmGraphView()
    {
        stopTimer();
    }

    void AlgorithmGraphView::timerCallback()
    {
        pulsePhase_ += 1.0f / 24.0f * 0.35f; // one full lap roughly every ~3 seconds.
        if (pulsePhase_ > 1.0f)
            pulsePhase_ -= 1.0f;
        repaint();
    }

    juce::Point<float> AlgorithmGraphView::nodePosition(std::size_t index, std::size_t nodeCount,
                                                          juce::Rectangle<float> area) const
    {
        const float radius = juce::jmin(area.getWidth(), area.getHeight()) * 0.5f * 0.92f;
        const auto centre = area.getCentre();
        if (nodeCount == 0)
            return centre;
        const float angle =
            -juce::MathConstants<float>::halfPi + (static_cast<float>(index) / static_cast<float>(nodeCount)) *
                                                       juce::MathConstants<float>::twoPi;
        return {centre.x + radius * std::cos(angle), centre.y + radius * std::sin(angle)};
    }

    void AlgorithmGraphView::paint(juce::Graphics& g)
    {
        const auto& algo = processor_.getCurrentPatch().layerA.algorithm;
        const auto area = getLocalBounds().toFloat().reduced(28.0f);
        const std::size_t nodeCount = algo.nodes.size();
        constexpr float kNodeRadius = 25.0f;

        // Edges first, so node circles draw cleanly on top of the lines feeding them.
        for (std::size_t e = 0; e < algo.edges.size(); ++e)
        {
            const auto& edge = algo.edges[e];
            std::size_t srcIdx = nodeCount, dstIdx = nodeCount;
            for (std::size_t i = 0; i < nodeCount; ++i)
            {
                if (algo.nodes[i].id == edge.source) srcIdx = i;
                if (algo.nodes[i].id == edge.destination) dstIdx = i;
            }
            if (srcIdx == nodeCount || dstIdx == nodeCount)
                continue; // Defensive: a malformed edge shouldn't crash the editor.

            const auto colour = palette::edgeColour(static_cast<int>(edge.type));
            const float edgePhase = std::fmod(pulsePhase_ + static_cast<float>(e) * 0.13f, 1.0f);

            if (srcIdx == dstIdx)
            {
                // Self-feedback: a small loop just outside the node itself.
                const auto p = nodePosition(srcIdx, nodeCount, area);
                const auto centre = area.getCentre();
                const juce::Point<float> dir = (p - centre).isOrigin() ? juce::Point<float>(1.0f, 0.0f)
                                                                        : (p - centre) / p.getDistanceFrom(centre);
                const auto loopCentre = p + dir * (kNodeRadius * 1.6f);
                juce::Path loop;
                loop.addEllipse(loopCentre.x - kNodeRadius * 0.7f, loopCentre.y - kNodeRadius * 0.7f,
                                 kNodeRadius * 1.4f, kNodeRadius * 1.4f);
                g.setColour(colour.withAlpha(0.7f));
                g.strokePath(loop, juce::PathStrokeType(1.6f));

                const auto dot = loop.getPointAlongPath(edgePhase * loop.getLength());
                g.setColour(colour);
                g.fillEllipse(dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
                continue;
            }

            const auto p1 = nodePosition(srcIdx, nodeCount, area);
            const auto p2 = nodePosition(dstIdx, nodeCount, area);
            const auto centre = area.getCentre();
            // Bow the edge slightly away from the centre so overlapping edges between
            // non-adjacent nodes stay visually separable rather than stacking as
            // straight lines through the middle of the circle.
            const auto mid = (p1 + p2) * 0.5f;
            const auto bow = (mid - centre).isOrigin() ? mid : mid + (mid - centre) * 0.18f;

            juce::Path path;
            path.startNewSubPath(p1);
            path.quadraticTo(bow, p2);

            g.setColour(colour.withAlpha(0.55f));
            g.strokePath(path, juce::PathStrokeType(1.4f));

            const auto dot = path.getPointAlongPath(edgePhase * path.getLength());
            g.setColour(colour);
            g.fillEllipse(dot.x - 2.5f, dot.y - 2.5f, 5.0f, 5.0f);
        }

        // Nodes on top.
        for (std::size_t i = 0; i < nodeCount; ++i)
        {
            const auto& node = algo.nodes[i];
            const auto p = nodePosition(i, nodeCount, area);
            const bool implemented = algorithm::isEngineImplemented(node.engine);

            g.setColour(palette::kPanelRaised);
            g.fillEllipse(p.x - kNodeRadius, p.y - kNodeRadius, kNodeRadius * 2.0f, kNodeRadius * 2.0f);

            g.setColour(node.isOutput ? palette::kAccent : palette::kBorderBright);
            if (!implemented)
            {
                // Renders silence today (docs/DSP_ENGINE.md) -- an honest visual cue,
                // not hidden behind a control that looks fully live.
                float dashLengths[] = {3.0f, 2.5f};
                juce::Path ring;
                ring.addEllipse(p.x - kNodeRadius, p.y - kNodeRadius, kNodeRadius * 2.0f, kNodeRadius * 2.0f);
                juce::Path dashed;
                juce::PathStrokeType(1.5f).createDashedStroke(dashed, ring, dashLengths, 2);
                g.fillPath(dashed);
            }
            else
            {
                g.drawEllipse(p.x - kNodeRadius, p.y - kNodeRadius, kNodeRadius * 2.0f, kNodeRadius * 2.0f, 1.6f);
            }

            g.setColour(implemented ? palette::kTextPrimary : palette::kTextDim);
            g.setFont(juce::Font(juce::FontOptions(10.5f)));
            g.drawText(engineShortName(node.engine), juce::Rectangle<float>(p.x - kNodeRadius, p.y - 6.0f,
                                                                             kNodeRadius * 2.0f, 12.0f),
                       juce::Justification::centred);
            g.setColour(palette::kTextDim);
            g.setFont(juce::Font(juce::FontOptions(8.0f)));
            g.drawText(juce::String(static_cast<int>(node.id.get())),
                       juce::Rectangle<float>(p.x - kNodeRadius, p.y + 6.0f, kNodeRadius * 2.0f, 10.0f),
                       juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui
