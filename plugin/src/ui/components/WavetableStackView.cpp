#include "WavetableStackView.h"

#include <algorithm>
#include <cmath>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/dsp/Math.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        // Cap how many frames get their own ribbon -- a 64-frame morph table drawn
        // as 64 stacked cards would just be visual noise at this component's size;
        // beyond this cap, evenly-spaced samples across the table stand in for the
        // full set (still an honest depiction of the table's actual shape, just not
        // literally one path per frame).
        constexpr int kMaxDrawnFrames = 6;
        // Points per ribbon -- enough to read the waveform's real shape without
        // painting a path per sample (a 2048-sample frame doesn't need 2048 path
        // points to look identical at this pixel scale).
        constexpr int kPointsPerRibbon = 56;
        constexpr float kSkewFactor = -0.32f; // Isometric-ish shear, matches the HTML reference mockup.
    } // namespace

    WavetableStackView::WavetableStackView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(4); // Table content only changes on patch load / node reselection; slow poll is plenty.
    }

    WavetableStackView::~WavetableStackView()
    {
        stopTimer();
    }

    void WavetableStackView::showNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, static_cast<int>(pw8::core::kNodesPerLayer) - 1, nodeIndex);
        repaint();
    }

    void WavetableStackView::timerCallback()
    {
        repaint(); // Cheap: paint() below reads already-loaded table data, no allocation here.
    }

    void WavetableStackView::paint(juce::Graphics& g)
    {
        const auto* table = processor_.getActiveWavetableTable(static_cast<std::size_t>(selectedNode_));
        auto bounds = getLocalBounds().toFloat();

        if (table == nullptr || !table->isValid())
        {
            // Two genuinely different situations look identical from getActiveWavetableTable()
            // alone, and telling them apart matters: wavetableId is only ever read from
            // a file during a full Engine rebuild (Engine::loadPatch(), see that
            // function's own comment), NOT by the live "Engine" APVTS parameter this
            // view's sibling engine pill writes -- so picking Wavetable via the pill on
            // an operator that had a different engine at the last patch load will always
            // land here, and it isn't a load failure.
            const auto& wavetableId = processor_.getCurrentPatch()
                                           .layerA.operators[static_cast<std::size_t>(selectedNode_)]
                                           .wavetableId;
            g.setColour(palette::kTextDim);
            g.setFont(fonts::value(11.0f));
            g.drawText(wavetableId.empty()
                           ? "No wavetable file assigned to this operator."
                           : "Wavetable file not loaded -- switching engines live doesn't load "
                             "one; save/reload the patch to pick it up.",
                       bounds, juce::Justification::centred);
            return;
        }

        // Always the highest-fidelity mip for display -- this view is for seeing the
        // authored table's real shape, not previewing the aliasing-avoidance mip
        // selection Voice::renderSample makes at runtime for a given note.
        const auto& mip = table->mips.front();
        const int numFrames = table->numFrames;
        const int samplesPerFrame = table->samplesPerFrame;
        if (numFrames <= 0 || samplesPerFrame <= 1 ||
            mip.samples.size() < static_cast<std::size_t>(numFrames) * static_cast<std::size_t>(samplesPerFrame))
            return; // Defensive: a malformed/truncated table shouldn't crash the editor.

        const int drawnCount = juce::jmin(numFrames, kMaxDrawnFrames);

        // Which frame is "live" right now, if this operator's WavetablePos param is
        // automated/APVTS-driven -- highlighted as the front, full-accent ribbon.
        // Falls back to frame 0 if the parameter isn't found (defensive only; it's
        // always registered for every node per PluginState.h's field spec).
        const auto posParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "WavetablePos");
        float livePos = 0.0f;
        if (auto* raw = processor_.apvts.getRawParameterValue(posParamId))
            livePos = juce::jlimit(0.0f, 1.0f, raw->load());
        const int liveFrame = juce::jlimit(0, numFrames - 1, static_cast<int>(livePos * static_cast<float>(numFrames - 1) + 0.5f));

        // Layout: front ribbon (drawnCount-1, the one closest to `liveFrame`'s depth
        // rank) sits at the bottom-front; each ribbon behind it shifts up-and-right
        // and shrinks in alpha, the "deck of cards" read. Front ribbon reserves the
        // bottom ~45% of the component so its glow/fill has room.
        const float stepX = bounds.getWidth() * 0.09f;
        const float stepY = bounds.getHeight() * 0.19f;
        const float ribbonWidth = bounds.getWidth() - stepX * static_cast<float>(drawnCount - 1) - 24.0f;
        const float ribbonHeight = bounds.getHeight() * 0.32f;

        for (int depth = drawnCount - 1; depth >= 0; --depth)
        {
            // Evenly spread the drawn subset across the real frame range, and make
            // the LAST drawn ribbon (depth 0, frontmost) track the actual live frame
            // rather than always being frame 0 -- so the front-most, brightest ribbon
            // is always the one that's actually sounding.
            const int frameIndex = (depth == 0) ? liveFrame
                                                 : juce::jlimit(0, numFrames - 1,
                                                                 (numFrames - 1) * depth / juce::jmax(1, drawnCount - 1));
            const bool isFront = depth == 0;

            const float originX = 12.0f + stepX * static_cast<float>(drawnCount - 1 - depth);
            const float originY = 8.0f + stepY * static_cast<float>(drawnCount - 1 - depth);

            juce::Path ribbon;
            const std::size_t frameOffset = static_cast<std::size_t>(frameIndex) * static_cast<std::size_t>(samplesPerFrame);
            for (int p = 0; p < kPointsPerRibbon; ++p)
            {
                const float t = static_cast<float>(p) / static_cast<float>(kPointsPerRibbon - 1);
                const float srcPos = t * static_cast<float>(samplesPerFrame - 1);
                const int s0 = static_cast<int>(srcPos);
                const int s1 = juce::jmin(s0 + 1, samplesPerFrame - 1);
                const float frac = srcPos - static_cast<float>(s0);
                const float sample = pw8::dsp::lerp(mip.samples[frameOffset + static_cast<std::size_t>(s0)],
                                                     mip.samples[frameOffset + static_cast<std::size_t>(s1)], frac);

                const float x = originX + t * ribbonWidth;
                const float y = originY + ribbonHeight * 0.5f - sample * ribbonHeight * 0.42f;
                if (p == 0)
                    ribbon.startNewSubPath(x, y);
                else
                    ribbon.lineTo(x, y);
            }

            const auto shear = juce::AffineTransform::shear(kSkewFactor, 0.0f);
            ribbon.applyTransform(shear);

            // jmap() asserts/NaNs when its source range is zero-width, which happens
            // whenever there's exactly one non-front ribbon (drawnCount == 2, so both
            // the source min and max below would be 1.0f) -- fall back to the range's
            // own start value rather than mapping across an empty range.
            const int backRibbonCount = drawnCount - 1;
            const float alpha = isFront                ? 1.0f
                                 : (backRibbonCount <= 1) ? 0.55f
                                                          : juce::jmap(static_cast<float>(depth), 1.0f,
                                                                       static_cast<float>(backRibbonCount), 0.55f, 0.18f);

            if (isFront)
            {
                // Soft under-glow, the same "reads as lit, not just colored" trick
                // ObsidianLookAndFeel's knob value-arc already uses.
                g.setColour(palette::kAccent.withAlpha(0.18f));
                g.strokePath(ribbon, juce::PathStrokeType(4.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }

            g.setColour((isFront ? palette::kAccent : palette::kBorderBright).withAlpha(alpha));
            g.strokePath(ribbon, juce::PathStrokeType(isFront ? 1.8f : 1.2f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

            g.setColour(palette::kTextDim.withAlpha(alpha));
            g.setFont(fonts::value(8.5f));
            g.drawText(juce::String(frameIndex), juce::Rectangle<float>(originX - 16.0f, originY + ribbonHeight * 0.5f - 6.0f, 14.0f, 12.0f)
                           .transformedBy(shear),
                       juce::Justification::centredRight);
        }

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::value(10.0f));
        g.drawText(juce::String(numFrames) + " frames, mip 0 (" + juce::String(mip.maxHarmonic) + " harmonics) -- frame " +
                       juce::String(liveFrame) + " live",
                   bounds.removeFromBottom(16.0f), juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui
