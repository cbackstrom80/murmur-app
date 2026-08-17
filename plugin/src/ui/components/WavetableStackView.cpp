#include "WavetableStackView.h"

#include <algorithm>
#include <cmath>

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "wireframe/WavetableMeshPaint.h"
#include "wireframe/WireframeProjection.h"
#include "pw8/algorithm/AlgorithmTypes.hpp"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kArrowWidth = 34;
        constexpr int kButtonHeight = 28;
        constexpr int kButtonMargin = 4;
        constexpr int kCaptionHeight = 18;
    } // namespace

    WavetableStackView::WavetableStackView(PatchworkEightProcessor& processor) : processor_(processor)
    {
        wavetableIndex_.rescan();
        loadButton_.setVisible(false); // Custom import is secondary; factory ← → nav is primary.
        loadButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        loadButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        loadButton_.onClick = [this] { loadWavetableFromFile(); };
        addChildComponent(loadButton_);

        tableNameLabel_.setJustificationType(juce::Justification::centred);
        tableNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        tableNameLabel_.setFont(fonts::label(11.0f));
        addAndMakeVisible(tableNameLabel_);

        for (auto* arrow : {&prevButton_, &nextButton_})
        {
            arrow->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            arrow->setColour(juce::TextButton::buttonOnColourId, palette::kAccentDim);
            arrow->setColour(juce::TextButton::textColourOffId, palette::kAccent);
            addAndMakeVisible(*arrow);
        }
        prevButton_.setTooltip("Previous wavetable in the factory library");
        nextButton_.setTooltip("Next wavetable in the factory library");
        prevButton_.onClick = [this] { goToSibling(-1); };
        nextButton_.onClick = [this] { goToSibling(1); };

        refreshSiblings();
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
        const auto engineParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        int engineOrdinal = 0;
        if (auto* raw = processor_.apvts.getRawParameterValue(engineParamId))
            engineOrdinal = static_cast<int>(raw->load() + 0.5f);
        const bool needsTable = engineOrdinal == static_cast<int>(algorithm::EngineType::Wavetable)
                                || engineOrdinal == static_cast<int>(algorithm::EngineType::Granular);

        const auto& op = processor_.getCurrentPatch().layerA.operators[static_cast<std::size_t>(selectedNode_)];
        if (!needsTable || !op.wavetableId.empty())
            return;

        if (wavetableIndex_.allEntries().isEmpty())
            wavetableIndex_.rescan();
        if (wavetableIndex_.allEntries().isEmpty())
            return;

        processor_.setOperatorWavetableFile(static_cast<std::size_t>(selectedNode_),
                                            wavetableIndex_.allEntries().getFirst().absolutePath);
        refreshSiblings();
    }

    void WavetableStackView::setShowLoadButton(bool show)
    {
        loadButton_.setVisible(show);
        resized();
    }

    void WavetableStackView::resized()
    {
        auto bounds = getLocalBounds().reduced(kButtonMargin);

        if (loadButton_.isVisible())
        {
            auto loadRow = bounds.removeFromTop(kButtonHeight + 2);
            loadButton_.setBounds(loadRow.removeFromRight(88).reduced(1));
        }

        auto captionRow = bounds.removeFromBottom(kCaptionHeight);
        tableNameLabel_.setBounds(captionRow);

        auto meshArea = bounds;
        const int arrowColumn = kArrowWidth + 4;
        prevButton_.setBounds(meshArea.removeFromLeft(arrowColumn).withSizeKeepingCentre(kArrowWidth, kButtonHeight));
        nextButton_.setBounds(meshArea.removeFromRight(arrowColumn).withSizeKeepingCentre(kArrowWidth, kButtonHeight));
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
        const bool canCycle = total > 0;
        prevButton_.setEnabled(canCycle);
        nextButton_.setEnabled(canCycle);

        if (total > 0)
        {
            const int idx = wavetableIndex_.indexOf(lastKnownWavetableId_);
            const auto& entry = idx >= 0 ? wavetableIndex_.allEntries()[idx] : wavetableIndex_.allEntries().getFirst();
            juce::String caption = entry.name;
            if (idx >= 0)
                caption += "  ·  " + juce::String(idx + 1) + " / " + juce::String(total);
            tableNameLabel_.setText(caption, juce::dontSendNotification);
        }
        else
        {
            tableNameLabel_.setText("No factory wavetables found", juce::dontSendNotification);
        }
    }

    void WavetableStackView::goToSibling(int delta)
    {
        if (wavetableIndex_.allEntries().isEmpty())
            wavetableIndex_.rescan();
        if (wavetableIndex_.allEntries().isEmpty())
            return;

        processor_.syncCurrentPatchFromApvts();
        const auto current = currentWavetablePath();
        const auto next = delta > 0 ? wavetableIndex_.nextAfter(current) : wavetableIndex_.prevBefore(current);
        if (!next.has_value())
            return;

        if (processor_.setOperatorWavetableFile(static_cast<std::size_t>(selectedNode_), next->absolutePath))
        {
            refreshSiblings();
            repaint();
        }
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
        bounds.removeFromBottom(static_cast<float>(kCaptionHeight + kButtonMargin)); // Caption + table name row.
        bounds.removeFromLeft(static_cast<float>(kArrowWidth + kButtonMargin + 4)); // Prev arrow column.
        bounds.removeFromRight(static_cast<float>(kArrowWidth + kButtonMargin + 4)); // Next arrow column.

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

        oscillator::WtWarpParams warpParams;
        const auto engineParamId = operatorParamId(static_cast<std::size_t>(selectedNode_), "Engine");
        int engineOrdinal = static_cast<int>(algorithm::EngineType::Wavetable);
        if (auto* rawEngine = processor_.apvts.getRawParameterValue(engineParamId))
            engineOrdinal = static_cast<int>(rawEngine->load() + 0.5f);
        if (engineOrdinal == static_cast<int>(algorithm::EngineType::Wavetable))
        {
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(selectedNode_), "WtBend")))
                warpParams.bend = raw->load();
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(selectedNode_), "WtAsymmetry")))
                warpParams.asymmetry = raw->load();
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(selectedNode_), "WtSyncRatio")))
                warpParams.syncRatio = raw->load();
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(selectedNode_), "WtSyncAmount")))
                warpParams.syncAmount = raw->load();
        }

        auto captionArea = bounds.removeFromBottom(14.0f);

        wireframe::paintWavetableTableMesh(g, bounds, table, warpParams, livePos, wireframe::labHeroMeshOptions());

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::value(10.0f));
        const int libIndex = wavetableIndex_.indexOf(currentWavetablePath());
        juce::String libPos;
        if (libIndex >= 0)
            libPos = " · lib " + juce::String(libIndex + 1) + "/" + juce::String(wavetableIndex_.allEntries().size());
        g.drawText(juce::String(numFrames) + " frames, mip 0 (" + juce::String(mip.maxHarmonic) + " harmonics) · frame "
                       + juce::String(liveFrame) + " live" + libPos,
                   captionArea, juce::Justification::centredLeft);

        if (granularOverlay_)
        {
            wireframe::GranularOverlayParams grainParams;
            grainParams.wavetablePos = livePos;
            if (auto* raw = processor_.apvts.getRawParameterValue(
                    operatorParamId(static_cast<std::size_t>(selectedNode_), "GrainSizeMs")))
                grainParams.grainSizeMs = raw->load();
            wireframe::paintGranularGrainOverlay(g, bounds, grainParams);
        }
    }

} // namespace pw8::plugin::ui
