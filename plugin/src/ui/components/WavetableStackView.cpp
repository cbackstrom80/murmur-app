#include "WavetableStackView.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "pw8/dsp/Math.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        // Mesh density -- rows drawn regardless of the table's real frame
        // count; extras are honest interpolation, see the header doc comment.
        // Lower than the Python prototype's 26: that was tuned against a
        // ~680x500 canvas, but this component's real bounds land closer to
        // 554x176 (a much shorter aspect -- OperatorEditorPanel's Wavetable
        // slot is wide, not tall). At the prototype's row count in this
        // little vertical room, 26 rows land under 3px apart and visually
        // mush together instead of reading as distinct mesh lines; fewer,
        // more widely-separated rows read far better at this actual size.
        constexpr int kMeshRows = 15;
        // Points per row -- enough to read the waveform's real shape without
        // painting a path per sample (a 2048-sample frame doesn't need 2048
        // path points to look identical at this pixel scale).
        constexpr int kPointsPerRow = 48;
        // Connect every Nth sample index between a row and its farther
        // neighbour -- this is what makes it read as one continuous mesh
        // surface rather than a stack of independent parallel ribbons.
        constexpr int kCrossLineStride = 6;

        // Cheap pseudo-3D projection (validated against a Python prototype
        // before porting): each row shifts right/up and shrinks as depth
        // increases -- no real camera/projection matrix, just enough to read
        // as perspective at this scale, same spirit as the shear the previous
        // "deck of cards" rendering used. kRowStepYFrac raised well above the
        // prototype's 0.34 for the same reason as kMeshRows above -- this
        // component's real vertical budget is much smaller, so depth-rise
        // needs a bigger share of it to stay legible.
        constexpr float kRowStepXFrac = 0.16f;
        constexpr float kRowStepYFrac = 0.40f;
        constexpr float kFarShrink = 0.55f; // farthest row's scale relative to nearest.

        constexpr int kButtonWidth = 64;
        constexpr int kArrowWidth = 22;
        constexpr int kButtonHeight = 18;
        constexpr int kButtonMargin = 4;
    } // namespace

    WavetableStackView::WavetableStackView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        wavetableIndex_.rescan();
        loadButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        loadButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        loadButton_.onClick = [this] { loadWavetableFromFile(); };
        addAndMakeVisible(loadButton_);

        for (auto* arrow : {&prevButton_, &nextButton_})
        {
            arrow->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            arrow->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            arrow->setEnabled(false); // Nothing to browse until refreshSiblings() finds a directory.
            addAndMakeVisible(*arrow);
        }
        prevButton_.onClick = [this] { goToSibling(-1); };
        nextButton_.onClick = [this] { goToSibling(1); };

        startTimerHz(4); // Table content only changes on patch load / node reselection; slow poll is plenty.
    }

    WavetableStackView::~WavetableStackView()
    {
        stopTimer();
    }

    void WavetableStackView::showNode(int nodeIndex)
    {
        selectedNode_ = juce::jlimit(0, static_cast<int>(pw8::core::kNodesPerLayer) - 1, nodeIndex);
        ensureDefaultWavetableLoaded();
        refreshSiblings();
        repaint();
    }

    juce::String WavetableStackView::currentWavetablePath() const
    {
        return juce::String(processor_.getCurrentPatch()
                                .layerA.operators[static_cast<std::size_t>(selectedNode_)]
                                .wavetableId);
    }

    void WavetableStackView::ensureDefaultWavetableLoaded()
    {
        const auto& op = processor_.getCurrentPatch().layerA.operators[static_cast<std::size_t>(selectedNode_)];
        const bool needsTable = op.engine == algorithm::EngineType::Wavetable || op.engine == algorithm::EngineType::Granular;
        if (!needsTable || !op.wavetableId.empty())
            return;

        if (wavetableIndex_.allEntries().isEmpty())
            wavetableIndex_.rescan();
        if (wavetableIndex_.allEntries().isEmpty())
            return;

        processor_.setOperatorWavetableFile(static_cast<std::size_t>(selectedNode_),
                                            wavetableIndex_.allEntries().getFirst().absolutePath);
    }

    void WavetableStackView::resized()
    {
        auto bounds = getLocalBounds().reduced(kButtonMargin);
        auto row = bounds.removeFromTop(kButtonHeight);
        nextButton_.setBounds(row.removeFromRight(kArrowWidth));
        row.removeFromRight(kButtonMargin);
        loadButton_.setBounds(row.removeFromRight(kButtonWidth));
        row.removeFromRight(kButtonMargin);
        prevButton_.setBounds(row.removeFromRight(kArrowWidth));
    }

    void WavetableStackView::timerCallback()
    {
        const auto& wavetableId = processor_.getCurrentPatch().layerA.operators[static_cast<std::size_t>(selectedNode_)].wavetableId;
        if (juce::String(wavetableId) != lastKnownWavetableId_)
            refreshSiblings(); // Picks up a change made via the "Load..." dialog, the arrows themselves, or a host-driven patch reload.
        repaint(); // Cheap: paint() below reads already-loaded table data, no allocation here.
    }

    void WavetableStackView::refreshSiblings()
    {
        if (wavetableIndex_.allEntries().isEmpty())
            wavetableIndex_.rescan();

        lastKnownWavetableId_ = currentWavetablePath();

        const int total = wavetableIndex_.allEntries().size();
        prevButton_.setEnabled(total > 1);
        nextButton_.setEnabled(total > 1);
    }

    void WavetableStackView::goToSibling(int delta)
    {
        if (wavetableIndex_.allEntries().isEmpty())
            return;

        const auto current = currentWavetablePath();
        const auto next = delta > 0 ? wavetableIndex_.nextAfter(current) : wavetableIndex_.prevBefore(current);
        if (!next.has_value())
            return;

        processor_.setOperatorWavetableFile(static_cast<std::size_t>(selectedNode_), next->absolutePath);
        refreshSiblings();
        repaint();
    }

    void WavetableStackView::loadWavetableFromFile()
    {
        fileChooser_ = std::make_unique<juce::FileChooser>(
            "Load a wavetable JSON (pw8-wavetable-builder output)...", juce::File(), "*.json");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

        // SafePointer, not a raw `this` capture: if the editor closes while this
        // native dialog is still open (modeless, can sit open indefinitely), the
        // completion callback would otherwise touch a destroyed component --
        // exactly the lifetime hazard already present (uncorrected) in
        // PatchBrowserBar's own file-chooser loader, not repeated here. Also
        // captures which node the request was FOR, not just `this` -- if the
        // player reselects a different node while the dialog is still open, the
        // file they picked should still land on the operator they meant it for.
        const juce::Component::SafePointer<WavetableStackView> safeThis(this);
        const int nodeAtRequestTime = selectedNode_;
        fileChooser_->launchAsync(flags, [safeThis, nodeAtRequestTime](const juce::FileChooser& chooser) {
            auto* self = safeThis.getComponent();
            if (self == nullptr)
                return; // Component (and its editor) closed while the dialog was open.

            const auto file = chooser.getResult();
            if (!file.existsAsFile())
                return; // Cancelled -- not an error.

            self->processor_.setOperatorWavetableFile(static_cast<std::size_t>(nodeAtRequestTime),
                                                        file.getFullPathName());
            self->wavetableIndex_.rescan();
            self->refreshSiblings();
            self->repaint();
        });
    }

    void WavetableStackView::paint(juce::Graphics& g)
    {
        const auto* table = processor_.getActiveWavetableTable(static_cast<std::size_t>(selectedNode_));
        auto bounds = getLocalBounds().toFloat();
        bounds.removeFromTop(static_cast<float>(kButtonHeight + kButtonMargin * 2)); // Room for the load/prev/next row.

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

        // Which frame is "live" right now, if this operator's WavetablePos param is
        // automated/APVTS-driven -- the mesh row nearest this position gets the full-
        // accent highlight, wherever it happens to fall in the depth stack (not always
        // the frontmost row, since row order is fixed table-frame order, front to back).
        const auto posParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "WavetablePos");
        float livePos = 0.0f;
        if (auto* raw = processor_.apvts.getRawParameterValue(posParamId))
            livePos = juce::jlimit(0.0f, 1.0f, raw->load());
        const float liveFramePos = livePos * static_cast<float>(numFrames - 1);
        const int liveFrame = juce::jlimit(0, numFrames - 1, static_cast<int>(liveFramePos + 0.5f));

        auto captionArea = bounds.removeFromBottom(16.0f);

        // Interpolated sample at depth t in [0,1] (0 = front/nearest = table frame 0,
        // 1 = back/farthest = the table's last frame) and point index p -- honest
        // interpolation between the two real frames t falls between, see the header
        // doc comment for why this isn't fabricated data.
        auto sampleAt = [&](float depthT, int p) -> float
        {
            const float framePos = depthT * static_cast<float>(numFrames - 1);
            const int f0 = static_cast<int>(framePos);
            const int f1 = juce::jmin(f0 + 1, numFrames - 1);
            const float frameFrac = framePos - static_cast<float>(f0);

            const float tp = static_cast<float>(p) / static_cast<float>(kPointsPerRow - 1);
            const float srcPos = tp * static_cast<float>(samplesPerFrame - 1);
            const int s0 = static_cast<int>(srcPos);
            const int s1 = juce::jmin(s0 + 1, samplesPerFrame - 1);
            const float sampleFrac = srcPos - static_cast<float>(s0);

            const std::size_t off0 = static_cast<std::size_t>(f0) * static_cast<std::size_t>(samplesPerFrame);
            const std::size_t off1 = static_cast<std::size_t>(f1) * static_cast<std::size_t>(samplesPerFrame);
            const float v0 = pw8::dsp::lerp(mip.samples[off0 + static_cast<std::size_t>(s0)],
                                             mip.samples[off0 + static_cast<std::size_t>(s1)], sampleFrac);
            const float v1 = pw8::dsp::lerp(mip.samples[off1 + static_cast<std::size_t>(s0)],
                                             mip.samples[off1 + static_cast<std::size_t>(s1)], sampleFrac);
            return pw8::dsp::lerp(v0, v1, frameFrac);
        };

        // originY/rowHeight/stepY are chosen so every row's full amplitude
        // swing, at every depth, stays inside `bounds` -- derived, not
        // guessed: the nearest row (t=0, scale=1) needs
        // originYFrac +/- rowHeightFrac/2 within [0,1], and the farthest row
        // (t=1, scale=kFarShrink) needs (originYFrac - stepYFrac) +/-
        // rowHeightFrac*kFarShrink/2 within [0,1]. 0.55/0.45/0.40 satisfies
        // both with margin to spare (checked by hand before picking these).
        const float originX = bounds.getX() + bounds.getWidth() * 0.42f;
        const float originY = bounds.getY() + bounds.getHeight() * 0.55f;
        const float rowWidth = bounds.getWidth() * 0.60f;
        const float rowHeight = bounds.getHeight() * 0.45f;
        const float stepX = bounds.getWidth() * kRowStepXFrac;
        const float stepY = bounds.getHeight() * kRowStepYFrac;

        struct RowPoints
        {
            std::array<juce::Point<float>, static_cast<std::size_t>(kPointsPerRow)> pts;
        };
        std::array<RowPoints, static_cast<std::size_t>(kMeshRows)> rows;

        int liveRowIndex = 0;
        float liveRowDist = std::numeric_limits<float>::max();

        for (int r = 0; r < kMeshRows; ++r)
        {
            const float depthT = static_cast<float>(r) / static_cast<float>(kMeshRows - 1);
            const float scale = 1.0f - (1.0f - kFarShrink) * depthT;

            const float rowFramePos = depthT * static_cast<float>(numFrames - 1);
            if (const float dist = std::abs(rowFramePos - liveFramePos); dist < liveRowDist)
            {
                liveRowDist = dist;
                liveRowIndex = r;
            }

            for (int p = 0; p < kPointsPerRow; ++p)
            {
                const float sampleValue = sampleAt(depthT, p);
                const float tp = static_cast<float>(p) / static_cast<float>(kPointsPerRow - 1);
                const float x = originX - rowWidth * 0.5f * scale + tp * rowWidth * scale + stepX * depthT;
                const float y = originY - stepY * depthT - sampleValue * rowHeight * 0.5f * scale;
                rows[static_cast<std::size_t>(r)].pts[static_cast<std::size_t>(p)] = {x, y};
            }
        }

        // Painter's algorithm, farthest row first: each nearer row's silhouette
        // (its own line, filled down to a baseline below it, in the panel's own
        // background colour) erases whatever farther content was already drawn
        // in that footprint -- the classic hidden-line wireframe-landscape trick,
        // validated in a Python prototype against this exact projection before
        // porting here.
        for (int r = kMeshRows - 1; r >= 0; --r)
        {
            const auto& row = rows[static_cast<std::size_t>(r)];
            const float depthT = static_cast<float>(r) / static_cast<float>(kMeshRows - 1);

            juce::Path linePath;
            float maxY = row.pts.front().y;
            for (int p = 0; p < kPointsPerRow; ++p)
            {
                const auto& pt = row.pts[static_cast<std::size_t>(p)];
                if (p == 0)
                    linePath.startNewSubPath(pt);
                else
                    linePath.lineTo(pt);
                maxY = juce::jmax(maxY, pt.y);
            }

            juce::Path silhouette(linePath);
            const float baselineY = maxY + 24.0f;
            silhouette.lineTo(row.pts.back().x, baselineY);
            silhouette.lineTo(row.pts.front().x, baselineY);
            silhouette.closeSubPath();
            g.setColour(palette::kPanel);
            g.fillPath(silhouette);

            const bool isLive = r == liveRowIndex;
            const float alpha = isLive ? 1.0f : juce::jmap(depthT, 0.0f, 1.0f, 0.75f, 0.15f);
            const float strokeWidth = isLive ? 2.0f : juce::jmap(depthT, 0.0f, 1.0f, 1.4f, 0.6f);

            // Cheap glow: a wide, dim pass under the crisp line -- the same
            // "reads as lit, not just coloured" trick ObsidianLookAndFeel's
            // knob value-arc already uses, no image blur/convolution needed.
            g.setColour(palette::kAccent.withAlpha(alpha * 0.25f));
            g.strokePath(linePath, juce::PathStrokeType(strokeWidth * 2.2f, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));
            g.setColour((isLive ? palette::kAccent : palette::kBorderBright).withAlpha(alpha));
            g.strokePath(linePath, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved,
                                                          juce::PathStrokeType::rounded));

            // Cross-lines to the FARTHER neighbour (r+1, already drawn this
            // pass) -- connecting to the nearer one instead would draw them
            // before that row's own occluding silhouette exists yet, so a
            // nearer row could never occlude a crossline the way it should.
            if (r < kMeshRows - 1)
            {
                const auto& fartherRow = rows[static_cast<std::size_t>(r + 1)];
                juce::Path crossPath;
                for (int p = 0; p < kPointsPerRow; p += kCrossLineStride)
                {
                    crossPath.startNewSubPath(row.pts[static_cast<std::size_t>(p)]);
                    crossPath.lineTo(fartherRow.pts[static_cast<std::size_t>(p)]);
                }
                g.setColour(palette::kBorderBright.withAlpha(alpha * 0.5f));
                g.strokePath(crossPath, juce::PathStrokeType(0.8f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
            }
        }

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::value(10.0f));
        const int libIndex = wavetableIndex_.indexOf(currentWavetablePath());
        const int libTotal = wavetableIndex_.allEntries().size();
        juce::String libPos;
        if (libIndex >= 0 && libTotal > 0)
            libPos = " -- " + juce::String(libIndex + 1) + " / " + juce::String(libTotal);
        g.drawText(juce::String(numFrames) + " frames, mip 0 (" + juce::String(mip.maxHarmonic) + " harmonics) -- frame " +
                       juce::String(liveFrame) + " live" + libPos,
                   captionArea, juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui
