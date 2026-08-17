#include "AlgorithmGraphView.h"

#include <array>
#include <cmath>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "EdgeIconGrid.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr float kNodeRadius = 25.0f;
        constexpr float kCaptionStripHeight = 46.0f;

        /// Plain-language verb for the caption sentence -- e.g. "Node 2
        /// phase-modulates Node 1". Matches the vocabulary the HTML mockup this
        /// was validated in used, chosen for a non-technical reader over the
        /// enum's own name (a player doesn't need to know "FrequencyMod" is the
        /// wire term to understand "frequency-modulates").
        const char* edgeVerb(algorithm::EdgeType type) noexcept
        {
            switch (type)
            {
                case algorithm::EdgeType::PhaseMod: return "phase-modulates";
                case algorithm::EdgeType::FrequencyMod: return "frequency-modulates";
                case algorithm::EdgeType::AmplitudeMod: return "amplitude-modulates";
                case algorithm::EdgeType::RingMod: return "ring-modulates";
                case algorithm::EdgeType::Sync: return "syncs";
                case algorithm::EdgeType::Feedback: return "feeds back into";
                case algorithm::EdgeType::Audio: return "sums into";
            }
            return "connects to";
        }

        const char* edgeShortLabel(algorithm::EdgeType type) noexcept
        {
            switch (type)
            {
                case algorithm::EdgeType::PhaseMod: return "PHASE";
                case algorithm::EdgeType::FrequencyMod: return "FREQ";
                case algorithm::EdgeType::AmplitudeMod: return "AMP";
                case algorithm::EdgeType::RingMod: return "RING";
                case algorithm::EdgeType::Sync: return "SYNC";
                case algorithm::EdgeType::Feedback: return "FEEDBACK";
                case algorithm::EdgeType::Audio: return "AUDIO";
            }
            return "?";
        }
    } // namespace

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
            case algorithm::EngineType::External: return "EXT";
        }
        return "?";
    }

    AlgorithmGraphView::AlgorithmGraphView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(24);
        setInterceptsMouseClicks(true, false);
    }

    AlgorithmGraphView::~AlgorithmGraphView()
    {
        stopTimer();
    }

    void AlgorithmGraphView::setGraphPreview(const algorithm::AlgorithmGraphDefinition* preview) noexcept
    {
        previewGraph_ = preview;
        repaint();
    }

    void AlgorithmGraphView::setSelectedNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, static_cast<int>(core::kNodesPerLayer) - 1, nodeIndex);
        repaint();
    }

    void AlgorithmGraphView::setActiveOperator(int nodeIndex)
    {
        activeOperator_ = juce::jlimit(0, static_cast<int>(core::kNodesPerLayer) - 1, nodeIndex);
        repaint();
    }

    const algorithm::AlgorithmGraphDefinition& AlgorithmGraphView::activeGraph() const noexcept
    {
        if (previewGraph_ != nullptr)
            return *previewGraph_;
        return processor_.getCurrentPatch().layerA.algorithm;
    }

    void AlgorithmGraphView::timerCallback()
    {
        pulsePhase_ += 1.0f / 24.0f * 0.35f; // one full lap roughly every ~3 seconds.
        if (pulsePhase_ > 1.0f)
            pulsePhase_ -= 1.0f;
        repaint();
    }

    juce::Rectangle<float> AlgorithmGraphView::circleArea() const
    {
        auto bounds = getLocalBounds().toFloat();
        bounds.removeFromBottom(kCaptionStripHeight);
        return bounds.reduced(28.0f);
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

    void AlgorithmGraphView::mouseDown(const juce::MouseEvent& event)
    {
        const auto& algo = activeGraph();
        const auto area = circleArea();
        const std::size_t nodeCount = algo.nodes.size();
        const auto pos = event.position;

        for (std::size_t i = 0; i < nodeCount; ++i)
        {
            const auto p = nodePosition(i, nodeCount, area);
            if (p.getDistanceFrom(pos) <= kNodeRadius)
            {
                selectedNode_ = static_cast<int>(i);
                repaint();
                if (onNodeSelected)
                    onNodeSelected(selectedNode_);
                return;
            }
        }
    }

    juce::String AlgorithmGraphView::buildCaption() const
    {
        const auto& algo = activeGraph();
        if (algo.edges.empty())
            return "All 8 operators sum directly to the output -- no custom routing in this patch.";

        const std::size_t nodeCount = algo.nodes.size();
        juce::StringArray parts;
        for (const auto& edge : algo.edges)
        {
            std::size_t srcIdx = nodeCount, dstIdx = nodeCount;
            for (std::size_t i = 0; i < nodeCount; ++i)
            {
                if (algo.nodes[i].id == edge.source) srcIdx = i;
                if (algo.nodes[i].id == edge.destination) dstIdx = i;
            }
            if (srcIdx == nodeCount || dstIdx == nodeCount)
                continue;

            if (srcIdx == dstIdx)
                parts.add("Node " + juce::String(static_cast<int>(srcIdx)) + " feeds back into itself");
            else
                parts.add("Node " + juce::String(static_cast<int>(srcIdx)) + " " + edgeVerb(edge.type) + " Node " +
                           juce::String(static_cast<int>(dstIdx)));
        }
        if (parts.isEmpty())
            return "All 8 operators sum directly to the output -- no custom routing in this patch.";
        return parts.joinIntoString("; ") + ".";
    }

    void AlgorithmGraphView::paint(juce::Graphics& g)
    {
        const auto& algo = activeGraph();
        const auto area = circleArea();
        const std::size_t nodeCount = algo.nodes.size();

        // Which nodes are actually "live" in this patch -- either summed straight
        // to the output, or reached/reaching some other node via an edge. A node
        // that's neither is truly unused (e.g. a node awaiting a DESIGN-mode edge
        // that hasn't been drawn yet), and gets dimmed rather than drawn at full
        // strength -- feedback from the HTML mockup review ("II dont understand
        // the graph") that an idle node reading identically to a live one was
        // part of what made the diagram confusing.
        std::array<bool, core::kNodesPerLayer> active{};
        for (std::size_t i = 0; i < nodeCount && i < core::kNodesPerLayer; ++i)
            active[i] = algo.nodes[i].isOutput;
        for (const auto& edge : algo.edges)
        {
            for (std::size_t i = 0; i < nodeCount && i < core::kNodesPerLayer; ++i)
            {
                if (algo.nodes[i].id == edge.source || algo.nodes[i].id == edge.destination)
                    active[i] = true;
            }
        }

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
            const bool activeEdge = [&] {
                if (activeOperator_ < 0 || static_cast<std::size_t>(activeOperator_) >= nodeCount)
                    return false;
                const auto nodeId = algo.nodes[static_cast<std::size_t>(activeOperator_)].id;
                return edge.source == nodeId || edge.destination == nodeId;
            }();
            const float edgeAlpha = activeEdge ? 0.85f : 0.55f;
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

            g.setColour(colour.withAlpha(edgeAlpha));
            g.strokePath(path, juce::PathStrokeType(activeEdge ? 2.0f : 1.4f));

            // Directional arrowhead, stopped just outside the destination node's
            // circle rather than pointing into its centre -- the other half of the
            // "II dont understand the graph" fix: an edge on its own doesn't say
            // which end is upstream, an arrow does.
            {
                const float pathLen = path.getLength();
                const float arrowT = juce::jmax(0.0f, pathLen - (kNodeRadius + 3.0f));
                const auto tip = path.getPointAlongPath(arrowT);
                const auto behind = path.getPointAlongPath(juce::jmax(0.0f, arrowT - 6.0f));
                auto dir = tip - behind;
                const float dirLen = dir.getDistanceFromOrigin();
                const juce::Point<float> unit = dirLen > 0.0001f ? dir / dirLen : juce::Point<float>(1.0f, 0.0f);
                const juce::Point<float> normal(-unit.y, unit.x);
                constexpr float kArrowLength = 8.0f, kArrowWidth = 5.0f;
                juce::Path arrow;
                arrow.startNewSubPath(tip);
                arrow.lineTo(tip - unit * kArrowLength + normal * kArrowWidth);
                arrow.lineTo(tip - unit * kArrowLength - normal * kArrowWidth);
                arrow.closeSubPath();
                g.setColour(colour);
                g.fillPath(arrow);
            }

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
            const bool isIdle = i < core::kNodesPerLayer && !active[i];
            const float alphaMul = isIdle ? 0.4f : 1.0f;

            // A slow ambient "breathing" glow behind output nodes -- the graph
            // reads as alive even with the default no-edge patch (see the
            // screenshot-driven review this was added from), not just when a
            // pulse happens to be traveling an edge. Structural/decorative, like
            // the edge pulse itself -- not driven by real audio levels.
            if (node.isOutput)
            {
                const float breath = 0.5f + 0.5f * std::sin(pulsePhase_ * juce::MathConstants<float>::twoPi * 2.15f);
                const float glowRadius = kNodeRadius * (1.35f + 0.25f * breath);
                g.setColour(palette::kAccent.withAlpha((0.10f + 0.08f * breath) * alphaMul));
                g.fillEllipse(p.x - glowRadius, p.y - glowRadius, glowRadius * 2.0f, glowRadius * 2.0f);
            }

            // Selection halo: a soft ring one size up from the node, drawn before
            // the node body so it reads as a glow behind it rather than a hard
            // outline competing with the implemented/not-implemented ring.
            if (static_cast<int>(i) == selectedNode_)
            {
                const float haloRadius = kNodeRadius * 1.28f;
                g.setColour(palette::kAccent.withAlpha(0.35f));
                g.drawEllipse(p.x - haloRadius, p.y - haloRadius, haloRadius * 2.0f, haloRadius * 2.0f, 2.0f);
            }

            g.setColour(palette::kPanelRaised.withAlpha(alphaMul));
            g.fillEllipse(p.x - kNodeRadius, p.y - kNodeRadius, kNodeRadius * 2.0f, kNodeRadius * 2.0f);

            g.setColour((node.isOutput ? palette::kAccent : palette::kBorderBright).withAlpha(alphaMul));
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

            g.setColour((implemented ? palette::kTextPrimary : palette::kTextDim).withAlpha(alphaMul));
            g.setFont(fonts::label(10.5f));
            g.drawText(engineShortName(node.engine), juce::Rectangle<float>(p.x - kNodeRadius, p.y - 6.0f,
                                                                             kNodeRadius * 2.0f, 12.0f),
                       juce::Justification::centred);
            g.setColour(palette::kTextDim.withAlpha(alphaMul));
            g.setFont(fonts::value(8.0f));
            g.drawText(juce::String(static_cast<int>(node.id.get())),
                       juce::Rectangle<float>(p.x - kNodeRadius, p.y + 6.0f, kNodeRadius * 2.0f, 10.0f),
                       juce::Justification::centred);
        }

        // Caption + legend strip, reserved at the bottom of this component's own
        // bounds (circleArea() already excludes it from the node layout above).
        {
            auto strip = getLocalBounds().toFloat().removeFromBottom(kCaptionStripHeight);
            g.setColour(palette::kBorder);
            g.drawLine(strip.getX(), strip.getY(), strip.getRight(), strip.getY(), 1.0f);
            strip.removeFromTop(6.0f);

            auto captionRow = strip.removeFromTop(20.0f);
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::value(11.5f));
            g.drawFittedText(buildCaption(), captionRow.toNearestInt(), juce::Justification::centredLeft, 2);

            // Legend: one dot + short label per edge type actually present in this
            // patch -- not the full fixed set of 7 types, so a simple patch's legend
            // stays short instead of listing 6 colors it isn't using.
            std::array<bool, 7> typePresent{};
            for (const auto& edge : algo.edges)
                typePresent[static_cast<std::size_t>(edge.type)] = true;

            auto legendRow = strip.removeFromTop(16.0f);
            g.setFont(fonts::value(9.5f));
            for (int t = 0; t < static_cast<int>(typePresent.size()); ++t)
            {
                if (!typePresent[static_cast<std::size_t>(t)])
                    continue;
                const auto type = static_cast<algorithm::EdgeType>(t);
                const auto colour = palette::edgeColour(t);
                auto iconBounds = legendRow.removeFromLeft(14.0f).withSizeKeepingCentre(12.0f, 12.0f).toFloat();
                edgeicons::drawEdgeIcon(g, type, iconBounds, colour, 1.1f);
                legendRow.removeFromLeft(2.0f);
                const auto label = juce::String(edgeShortLabel(type));
                juce::GlyphArrangement glyphs;
                glyphs.addLineOfText(g.getCurrentFont(), label, 0.0f, 0.0f);
                const float textWidth = glyphs.getBoundingBox(0, -1, true).getWidth() + 4.0f;
                g.setColour(palette::kTextDim);
                g.drawText(label, legendRow.removeFromLeft(textWidth).toNearestInt(), juce::Justification::centredLeft);
                legendRow.removeFromLeft(10.0f);
            }
        }
    }

} // namespace pw8::plugin::ui
