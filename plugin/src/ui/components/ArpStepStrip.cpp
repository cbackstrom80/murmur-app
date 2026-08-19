#include "ArpStepStrip.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "ArpUiFormatters.h"
#include "pw8/sequencer/ArpeggiatorTypes.hpp"

namespace pw8::plugin::ui
{
    ArpStepStrip::ArpStepStrip(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(30);
    }

    ArpStepStrip::~ArpStepStrip() { stopTimer(); }

    void ArpStepStrip::setPageStart(int pageStartStep) noexcept
    {
        pageStart_ = juce::jmax(0, pageStartStep);
        repaint();
    }

    int ArpStepStrip::visibleStepCount(int totalSteps) const noexcept
    {
        return juce::jmin(layout::kArpStepPageSize, juce::jmax(0, totalSteps - pageStart_));
    }

    int ArpStepStrip::laneWidthForLayout(int totalSteps) const noexcept
    {
        if (totalSteps <= 16)
            return layout::kArpStepColumnWidth;
        if (totalSteps <= 32)
            return 34;
        return layout::kArpStepColumnWidthCompact;
    }

    int ArpStepStrip::totalSteps() const noexcept
    {
        return static_cast<int>(processor_.getArpNumSteps());
    }

    int ArpStepStrip::addLaneLocalIndex(int total) const noexcept
    {
        if (total >= static_cast<int>(sequencer::kMaxArpSteps))
            return -1;

        const int lastGlobal = total - 1;
        if (lastGlobal < pageStart_ || lastGlobal >= pageStart_ + layout::kArpStepPageSize)
            return -1;

        return lastGlobal - pageStart_ + 1;
    }

    void ArpStepStrip::syncLayoutMetrics(int viewportWidth) noexcept
    {
        const int total = juce::jmax(1, totalSteps());
        const int visible = visibleStepCount(total);
        const int addLane = addLaneLocalIndex(total);
        const int laneSlots = visible + (addLane >= 0 ? 1 : 0);
        laneWidth_ = laneWidthForLayout(total);

        const int totalGap = juce::jmax(0, laneSlots - 1) * layout::kArpStepColumnGap;
        const int preferredWidth = laneSlots * laneWidth_ + totalGap;

        if (viewportWidth > 0 && preferredWidth > viewportWidth)
            contentWidth_ = preferredWidth;
        else if (viewportWidth > preferredWidth)
            contentWidth_ = viewportWidth;
        else
            contentWidth_ = juce::jmax(preferredWidth, viewportWidth);

        const int height = layout::kArpStepToolRowHeight + layout::kArpSeqHeaderHeight + 12
                           + layout::kArpStepSequencerRowHeight;
        setSize(contentWidth_, height);
    }

    int ArpStepStrip::getPreferredContentWidth() const noexcept
    {
        const int total = juce::jmax(1, totalSteps());
        const int visible = visibleStepCount(total);
        const int addLane = addLaneLocalIndex(total);
        const int laneSlots = visible + (addLane >= 0 ? 1 : 0);
        const int laneW = laneWidthForLayout(total);
        const int totalGap = juce::jmax(0, laneSlots - 1) * layout::kArpStepColumnGap;
        return laneSlots * laneW + totalGap;
    }

    int ArpStepStrip::laneWidthForIndex(int index, int count) const noexcept
    {
        juce::ignoreUnused(index, count);
        return laneWidth_ > 0 ? laneWidth_ : layout::kArpStepColumnWidth;
    }

    int ArpStepStrip::laneOffsetForIndex(int index, int count) const noexcept
    {
        int x = 0;
        for (int i = 0; i < index; ++i)
            x += laneWidthForIndex(i, count) + layout::kArpStepColumnGap;
        return x;
    }

    int ArpStepStrip::globalStepIndexForLocal(int localIndex) const noexcept
    {
        return pageStart_ + localIndex;
    }

    void ArpStepStrip::timerCallback()
    {
        const auto numSteps = processor_.getArpNumSteps();
        playheadStep_ = processor_.getArpPlayheadStep();
        noteSequenceIndex_ = processor_.getArpNoteSequenceIndex();
        pulsePhase_ += 0.08f;
        if (pulsePhase_ > juce::MathConstants<float>::twoPi)
            pulsePhase_ -= juce::MathConstants<float>::twoPi;

        if (numSteps != lastNumSteps_)
        {
            lastNumSteps_ = numSteps;
            const int maxPage = juce::jmax(0, static_cast<int>(numSteps) - layout::kArpStepPageSize);
            pageStart_ = juce::jlimit(0, maxPage, pageStart_);
            if (auto* parent = getParentComponent())
                parent->resized();
        }

        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        if (arp.enabled && numSteps > 0)
        {
            const int playhead = static_cast<int>(playheadStep_ % numSteps);
            const int pageEnd = pageStart_ + layout::kArpStepPageSize;
            if (playhead < pageStart_ || playhead >= pageEnd)
            {
                pageStart_ = (playhead / layout::kArpStepPageSize) * layout::kArpStepPageSize;
                if (auto* parent = getParentComponent())
                    parent->resized();
            }

            if (auto* viewport = dynamic_cast<juce::Viewport*>(getParentComponent()))
            {
                const int count = visibleStepCount(static_cast<int>(numSteps));
                const int localIndex = playhead - pageStart_;
                if (juce::isPositiveAndBelow(localIndex, count))
                {
                    const int playheadX =
                        laneOffsetForIndex(localIndex, count) + laneWidthForIndex(localIndex, count) / 2;
                    const int viewX = viewport->getViewPositionX();
                    const int viewW = viewport->getViewWidth();
                    if (playheadX < viewX + 24)
                        viewport->setViewPosition(juce::jmax(0, playheadX - 48), 0);
                    else if (playheadX > viewX + viewW - 24)
                        viewport->setViewPosition(juce::jmax(0, playheadX - viewW + 48), 0);
                }
            }
        }

        repaint();
    }

    void ArpStepStrip::resized()
    {
        syncLayoutMetrics(getWidth());
    }

    juce::Rectangle<int> ArpStepStrip::laneBounds(int localIndex, int count) const
    {
        auto area = getLocalBounds();
        area.removeFromTop(layout::kArpStepToolRowHeight);
        area.removeFromTop(layout::kArpSeqHeaderHeight + 12);
        const int x = area.getX() + laneOffsetForIndex(localIndex, count);
        const int laneW = laneWidthForIndex(localIndex, count);
        return {x, area.getY(), laneW, layout::kArpStepSequencerRowHeight};
    }

    juce::Rectangle<int> ArpStepStrip::velocityLaneBounds(juce::Rectangle<int> lane) const
    {
        const int laneW = juce::jmin(layout::kArpStepLaneWidth, lane.getWidth() - 4);
        const int x = lane.getX() + (lane.getWidth() - laneW) / 2;
        return {x, lane.getY() + 14, laneW, layout::kArpStepLaneHeight};
    }

    juce::Rectangle<int> ArpStepStrip::addLaneBounds(int localIndex, int count) const
    {
        juce::ignoreUnused(count);
        auto area = getLocalBounds();
        area.removeFromTop(layout::kArpStepToolRowHeight);
        area.removeFromTop(layout::kArpSeqHeaderHeight + 12);
        const int x = area.getX() + laneOffsetForIndex(localIndex, localIndex);
        const int laneW = laneWidthForIndex(localIndex, localIndex);
        return {x, area.getY(), laneW, layout::kArpStepSequencerRowHeight};
    }

    void ArpStepStrip::paintAddStepLane(juce::Graphics& g, juce::Rectangle<int> lane) const
    {
        auto body = velocityLaneBounds(lane).toFloat();
        g.setColour(palette::kPanelRaised.withAlpha(0.55f));
        g.fillRoundedRectangle(body, 4.0f);
        g.setColour(palette::kAccent.withAlpha(0.55f));
        g.drawRoundedRectangle(body.reduced(0.5f), 4.0f, 1.5f);

        g.setColour(palette::kAccent);
        g.setFont(fonts::label(16.0f));
        g.drawText("+", body, juce::Justification::centred);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("ADD", lane.removeFromBottom(14), juce::Justification::centred);
    }

    int ArpStepStrip::stepIndexAt(juce::Point<int> pos) const
    {
        const int total = juce::jmax(1, totalSteps());
        const int count = visibleStepCount(total);
        const int addLane = addLaneLocalIndex(total);

        for (int i = 0; i < count; ++i)
        {
            if (laneBounds(i, count).contains(pos))
                return globalStepIndexForLocal(i);
        }

        if (addLane >= 0 && addLaneBounds(addLane, count).contains(pos))
            return -2;

        return -1;
    }

    void ArpStepStrip::mouseDown(const juce::MouseEvent& event)
    {
        const int globalIndex = stepIndexAt(event.getPosition());
        if (globalIndex == -2)
        {
            processor_.setArpNumSteps(totalSteps() + 1);
            if (auto* parent = getParentComponent())
                parent->resized();
            repaint();
            return;
        }

        if (globalIndex < 0)
            return;

        const int total = juce::jmax(1, totalSteps());
        const int count = visibleStepCount(total);
        const int localIndex = globalIndex - pageStart_;
        if (!juce::isPositiveAndBelow(localIndex, count))
            return;

        auto lane = laneBounds(localIndex, count);
        auto meta = lane.removeFromBottom(layout::kArpStepMetaHeight);
        auto accentBadge = meta.removeFromTop(10).withSizeKeepingCentre(18, 10);
        meta.removeFromTop(4);
        auto ratchetBadge = meta.removeFromTop(10).withSizeKeepingCentre(22, 10);
        auto laneBody = velocityLaneBounds(lane);

        if (event.mods.isShiftDown())
        {
            processor_.toggleArpStepTie(static_cast<std::size_t>(globalIndex));
            repaint();
            return;
        }

        if (event.mods.isAltDown())
        {
            processor_.cycleArpStepProbability(static_cast<std::size_t>(globalIndex));
            repaint();
            return;
        }

        if (event.mods.isCommandDown() || event.mods.isCtrlDown())
        {
            processor_.cycleArpStepOctaveOffset(static_cast<std::size_t>(globalIndex));
            repaint();
            return;
        }

        if (accentBadge.contains(event.getPosition()))
        {
            processor_.toggleArpStepAccent(static_cast<std::size_t>(globalIndex));
            repaint();
            return;
        }
        if (ratchetBadge.contains(event.getPosition()))
        {
            processor_.cycleArpStepRatchet(static_cast<std::size_t>(globalIndex));
            repaint();
            return;
        }
        if (laneBody.contains(event.getPosition()))
        {
            dragStepIndex_ = globalIndex;
            const float norm = 1.0f - juce::jlimit(0.0f, 1.0f,
                                                    (static_cast<float>(event.getPosition().y) - laneBody.getY())
                                                        / static_cast<float>(laneBody.getHeight()));
            processor_.setArpStepVelocity(static_cast<std::size_t>(globalIndex), norm);
            repaint();
            return;
        }

        processor_.toggleArpStepEnabled(static_cast<std::size_t>(globalIndex));
        repaint();
    }

    void ArpStepStrip::mouseDrag(const juce::MouseEvent& event)
    {
        if (dragStepIndex_ < 0)
            return;

        const int total = juce::jmax(1, totalSteps());
        const int count = visibleStepCount(total);
        const int localIndex = dragStepIndex_ - pageStart_;
        if (!juce::isPositiveAndBelow(localIndex, count))
            return;

        auto lane = laneBounds(localIndex, count);
        lane.removeFromBottom(layout::kArpStepMetaHeight);
        const auto laneBody = velocityLaneBounds(lane);
        const float norm = 1.0f - juce::jlimit(0.0f, 1.0f,
                                                (static_cast<float>(event.getPosition().y) - laneBody.getY())
                                                    / static_cast<float>(laneBody.getHeight()));
        processor_.setArpStepVelocity(static_cast<std::size_t>(dragStepIndex_), norm);
        repaint();
    }

    void ArpStepStrip::paintStepLane(juce::Graphics& g, int localIndex, int globalIndex, juce::Rectangle<int> lane,
                                     bool isPlayhead) const
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const auto& step = arp.steps[static_cast<std::size_t>(globalIndex)];

        if (isPlayhead)
        {
            const float pulse = 0.5f + 0.5f * std::sin(pulsePhase_);
            g.setColour(palette::kAccent.withAlpha(0.10f + pulse * 0.10f));
            g.fillRect(lane.expanded(2, 0));
            g.setColour(palette::kAccent.withAlpha(0.45f + pulse * 0.35f));
            g.drawVerticalLine(lane.getCentreX(), static_cast<float>(lane.getY()),
                               static_cast<float>(lane.getBottom()));
            draw::fillGlowDot(g, {static_cast<float>(lane.getCentreX()), static_cast<float>(lane.getY() + 3)},
                             4.0f + pulse * 2.0f, palette::kAccent, 0.55f + pulse * 0.35f, 4);
        }

        auto ledArea = lane.removeFromTop(6);
        if (!step.enabled)
        {
            g.setColour(palette::kTextDim.withAlpha(0.35f));
            g.fillEllipse(ledArea.getCentreX() - 3.0f, static_cast<float>(ledArea.getY()), 6.0f, 6.0f);
        }
        else
        {
            g.setColour(isPlayhead ? palette::kAccentWarm : palette::kAccent);
            if (isPlayhead)
                g.fillEllipse(ledArea.getCentreX() - 4.0f, static_cast<float>(ledArea.getY()), 8.0f, 8.0f);
            else
                g.fillEllipse(ledArea.getCentreX() - 3.0f, static_cast<float>(ledArea.getY()), 6.0f, 6.0f);
        }

        lane.removeFromTop(8);
        auto meta = lane.removeFromBottom(layout::kArpStepMetaHeight);
        auto laneBody = velocityLaneBounds(lane).toFloat();

        draw::fillRecessedRoundedRect(g, laneBody, 4.0f);
        g.setColour(step.enabled ? palette::kBorderBright.withAlpha(0.65f) : palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(laneBody.reduced(0.5f), 4.0f, 1.0f);

        if (step.enabled)
        {
            const float vel = juce::jlimit(0.0f, 1.0f, step.velocityScale * (step.accent ? 1.0f : 0.85f));
            const float barH = laneBody.getHeight() * vel;
            juce::Rectangle<float> bar = laneBody.reduced(2.0f);
            bar = bar.removeFromBottom(barH);
            g.setColour(step.accent ? palette::kAccentWarm : palette::kAccent);
            g.fillRoundedRectangle(bar, 2.0f);

            if (step.tie)
            {
                g.setColour(palette::kAccent.withAlpha(0.85f));
                g.drawHorizontalLine(static_cast<int>(laneBody.getBottom() - 4.0f), laneBody.getX() + 3.0f,
                                       laneBody.getRight() - 3.0f);
            }

            if (step.probability < 0.99f)
            {
                g.setFont(fonts::label(6.0f));
                g.setColour(palette::kTextDim);
                g.drawText(juce::String(static_cast<int>(step.probability * 100.0f)) + "%",
                           laneBody.removeFromTop(10.0f), juce::Justification::centredRight, false);
            }
        }

        if (step.octaveOffset != 0)
        {
            g.setFont(fonts::label(6.0f));
            g.setColour(palette::kAccentWarm);
            g.drawText(step.octaveOffset > 0 ? "+" + juce::String(step.octaveOffset)
                                             : juce::String(step.octaveOffset),
                       laneBody.removeFromTop(8.0f), juce::Justification::centredLeft, false);
        }

        auto accentBadge = meta.removeFromTop(10).withSizeKeepingCentre(18, 10).toFloat();
        meta.removeFromTop(4);
        auto ratchetBadge = meta.removeFromTop(10).withSizeKeepingCentre(22, 10).toFloat();
        auto stepNum = meta;

        g.setColour(step.accent ? palette::kAccentWarm : palette::kPanel);
        g.fillRoundedRectangle(accentBadge, 2.0f);
        g.setColour(step.accent ? palette::kBackgroundTop : palette::kTextDim);
        g.setFont(juce::Font(juce::FontOptions(6.0f)));
        g.drawText("ACC", accentBadge, juce::Justification::centred);

        const bool ratchetActive = step.ratchetCount > 1;
        g.setColour(ratchetActive ? palette::kMurmurViolet : palette::kPanelRaised);
        g.fillRoundedRectangle(ratchetBadge, 3.0f);
        g.setColour(ratchetActive ? palette::kBackgroundTop : palette::kTextDim);
        g.setFont(juce::Font(juce::FontOptions(7.0f)));
        g.drawText("x" + juce::String(step.ratchetCount), ratchetBadge, juce::Justification::centred);

        g.setColour(step.enabled ? palette::kTextPrimary : palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText(juce::String(globalIndex + 1).paddedLeft('0', 2), stepNum, juce::Justification::centred);
    }

    void ArpStepStrip::paint(juce::Graphics& g)
    {
        const int total = juce::jmax(1, totalSteps());
        const int count = visibleStepCount(total);
        const int addLane = addLaneLocalIndex(total);
        const auto numSteps = static_cast<std::size_t>(total);
        const auto& arp = processor_.getCurrentPatch().arpeggiator;

        auto toolRow = getLocalBounds().removeFromTop(layout::kArpStepToolRowHeight);
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextDim);
        auto helpTop = toolRow.removeFromTop(toolRow.getHeight() / 2 + 1);
        auto helpBottom = toolRow;
        g.drawText("Click step: on/off   Drag: velocity", helpTop.reduced(2, 0),
                   juce::Justification::centredLeft, false);
        g.drawText("Shift: tie   Alt: prob   Cmd: octave", helpBottom.reduced(2, 0),
                   juce::Justification::centredLeft, false);

        if (total > layout::kArpStepPageSize)
        {
            const int page = pageStart_ / layout::kArpStepPageSize;
            const int pageCount = (total + layout::kArpStepPageSize - 1) / layout::kArpStepPageSize;
            g.drawText("PAGE " + juce::String(page + 1) + "/" + juce::String(pageCount),
                       toolRow.withY(helpTop.getY()), juce::Justification::centredRight, false);
        }

        auto header = getLocalBounds().removeFromTop(layout::kArpSeqHeaderHeight);
        const bool running = arp.enabled && processor_.getHostIsPlaying();
        g.setColour(running ? palette::kAccent : palette::kTextDim.withAlpha(0.55f));
        g.fillEllipse(static_cast<float>(header.getX() + 2), static_cast<float>(header.getCentreY() - 3), 6.0f, 6.0f);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(12.0f));
        g.drawText(juce::String(total) + "-STEP PATTERN", header.withTrimmedLeft(14),
                   juce::Justification::centredLeft);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        juce::String syncText;
        if (arp.rateMode == sequencer::ArpRateMode::TempoSync)
            syncText = juce::String(processor_.getHostBpm(), 1) + " BPM" + juce::String(fonts::kSep)
                       + arpSyncDivisionLabel(arp.syncDivisionIndex);
        else
            syncText = juce::String("FREE") + juce::String(fonts::kSep) + juce::String(arp.rateHz, 1) + " Hz";

        if (arp.enabled)
        {
            syncText += juce::String(fonts::kSep) + "STEP " + juce::String(static_cast<int>((playheadStep_ % numSteps) + 1));
            if (noteSequenceIndex_ > 0)
                syncText += juce::String(fonts::kSep) + "NOTE " + juce::String(static_cast<int>(noteSequenceIndex_ + 1));
        }
        g.drawText(syncText, header, juce::Justification::centredRight);

        getLocalBounds().removeFromTop(layout::kArpSeqHeaderHeight + 12);

        for (int i = 0; i < count; ++i)
        {
            const int globalIndex = globalStepIndexForLocal(i);
            auto lane = laneBounds(i, count);
            const bool isPlayhead = arp.enabled && (static_cast<std::size_t>(globalIndex) == (playheadStep_ % numSteps));
            paintStepLane(g, i, globalIndex, lane, isPlayhead);
        }

        if (addLane >= 0)
            paintAddStepLane(g, addLaneBounds(addLane, count));
    }

} // namespace pw8::plugin::ui
