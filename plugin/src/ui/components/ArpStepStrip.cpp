#include "ArpStepStrip.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    ArpStepStrip::ArpStepStrip(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(15);
    }

    ArpStepStrip::~ArpStepStrip() { stopTimer(); }

    void ArpStepStrip::syncLayoutMetrics(int viewportWidth) noexcept
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));
        const int totalGap = juce::jmax(0, count - 1) * layout::kArpStepColumnGap;
        const int preferredWidth = count * layout::kArpStepColumnWidth + totalGap;

        laneWidth_ = layout::kArpStepColumnWidth;
        if (viewportWidth > 0 && preferredWidth > viewportWidth)
        {
            contentWidth_ = preferredWidth;
        }
        else if (viewportWidth > preferredWidth)
        {
            contentWidth_ = viewportWidth;
        }
        else
        {
            contentWidth_ = juce::jmax(preferredWidth, viewportWidth);
        }

        const int height = layout::kArpSeqHeaderHeight + 12 + layout::kArpStepSequencerRowHeight;
        setSize(contentWidth_, height);
    }

    int ArpStepStrip::getPreferredContentWidth() const noexcept
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));
        const int totalGap = juce::jmax(0, count - 1) * layout::kArpStepColumnGap;
        return count * layout::kArpStepColumnWidth + totalGap;
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

    void ArpStepStrip::timerCallback()
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const auto numSteps = arp.numSteps;
        playheadStep_ = processor_.getArpPlayheadStep();
        noteSequenceIndex_ = processor_.getArpNoteSequenceIndex();
        if (numSteps != lastNumSteps_)
        {
            lastNumSteps_ = numSteps;
            if (auto* parent = getParentComponent())
                parent->resized();
        }

        if (arp.enabled && numSteps > 0)
        {
            if (auto* viewport = dynamic_cast<juce::Viewport*>(getParentComponent()))
            {
                const int count = juce::jmax(1, static_cast<int>(numSteps));
                const int playheadIndex = static_cast<int>(playheadStep_ % static_cast<std::size_t>(count));
                const int playheadX = laneOffsetForIndex(playheadIndex, count) + laneWidthForIndex(playheadIndex, count) / 2;
                const int viewX = viewport->getViewPositionX();
                const int viewW = viewport->getViewWidth();
                if (playheadX < viewX + 24)
                    viewport->setViewPosition(juce::jmax(0, playheadX - 48), 0);
                else if (playheadX > viewX + viewW - 24)
                    viewport->setViewPosition(juce::jmax(0, playheadX - viewW + 48), 0);
            }
        }

        repaint();
    }

    void ArpStepStrip::resized()
    {
        syncLayoutMetrics(getWidth());
    }

    juce::Rectangle<int> ArpStepStrip::laneBounds(int index, int count) const
    {
        auto area = getLocalBounds();
        area.removeFromTop(layout::kArpSeqHeaderHeight + 12);
        const int x = area.getX() + laneOffsetForIndex(index, count);
        const int laneW = laneWidthForIndex(index, count);
        return {x, area.getY(), laneW, layout::kArpStepSequencerRowHeight};
    }

    juce::Rectangle<int> ArpStepStrip::velocityLaneBounds(juce::Rectangle<int> lane) const
    {
        const int laneW = juce::jmin(layout::kArpStepLaneWidth, lane.getWidth() - 4);
        const int x = lane.getX() + (lane.getWidth() - laneW) / 2;
        return {x, lane.getY() + 14, laneW, layout::kArpStepLaneHeight};
    }

    void ArpStepStrip::mouseDown(const juce::MouseEvent& event)
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));

        for (int i = 0; i < count; ++i)
        {
            auto lane = laneBounds(i, count);
            auto meta = lane.removeFromBottom(layout::kArpStepMetaHeight);
            auto accentBadge = meta.removeFromTop(10).withSizeKeepingCentre(18, 10);
            meta.removeFromTop(4);
            auto ratchetBadge = meta.removeFromTop(10).withSizeKeepingCentre(22, 10);
            auto laneBody = velocityLaneBounds(lane);

            if (accentBadge.contains(event.getPosition()))
            {
                processor_.toggleArpStepAccent(static_cast<std::size_t>(i));
                repaint();
                return;
            }
            if (ratchetBadge.contains(event.getPosition()))
            {
                processor_.cycleArpStepRatchet(static_cast<std::size_t>(i));
                repaint();
                return;
            }
            if (laneBody.contains(event.getPosition()) || lane.contains(event.getPosition()))
            {
                processor_.toggleArpStepEnabled(static_cast<std::size_t>(i));
                repaint();
                return;
            }
        }
    }

    void ArpStepStrip::paint(juce::Graphics& g)
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));
        const auto numSteps = static_cast<std::size_t>(count);

        auto header = getLocalBounds().removeFromTop(layout::kArpSeqHeaderHeight);
        g.setColour(palette::kAccent);
        g.fillEllipse(static_cast<float>(header.getX() + 2), static_cast<float>(header.getCentreY() - 3), 6.0f, 6.0f);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::label(11.0f));
        g.drawText(juce::String(count) + "-STEP VELOCITY & PATTERN EDITOR", header.withTrimmedLeft(14),
                   juce::Justification::centredLeft);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        juce::String syncText = "SYNC: ";
        syncText += arp.rateMode == sequencer::ArpRateMode::TempoSync ? "INTERNAL 120 BPM" : "FREE";
        if (arp.enabled)
            syncText += "  ·  STEP " + juce::String(static_cast<int>((playheadStep_ % numSteps) + 1));
        g.drawText(syncText, header, juce::Justification::centredRight);

        getLocalBounds().removeFromTop(layout::kArpSeqHeaderHeight + 12);

        for (int i = 0; i < count; ++i)
        {
            const auto& step = arp.steps[static_cast<std::size_t>(i)];
            auto lane = laneBounds(i, count);
            const bool isPlayhead = arp.enabled && (static_cast<std::size_t>(i) == (playheadStep_ % numSteps));

            if (isPlayhead)
            {
                g.setColour(palette::kAccent.withAlpha(0.12f));
                g.fillRect(lane.expanded(1, 0));
                g.setColour(palette::kAccent.withAlpha(0.55f));
                g.drawVerticalLine(lane.getCentreX(), static_cast<float>(lane.getY()),
                                   static_cast<float>(lane.getBottom()));
            }

            auto ledArea = lane.removeFromTop(6);
            g.setColour(step.enabled ? palette::kAccent : palette::kTextDim.withAlpha(0.35f));
            if (isPlayhead)
                g.fillEllipse(ledArea.getCentreX() - 4.0f, static_cast<float>(ledArea.getY()), 8.0f, 8.0f);
            else
                g.fillEllipse(ledArea.getCentreX() - 3.0f, static_cast<float>(ledArea.getY()), 6.0f, 6.0f);

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
            }

            auto accentBadge = meta.removeFromTop(10).withSizeKeepingCentre(18, 10).toFloat();
            meta.removeFromTop(4);
            auto ratchetBadge = meta.removeFromTop(10).withSizeKeepingCentre(22, 10).toFloat();
            meta.removeFromTop(4);
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
            g.drawText(juce::String(i + 1).paddedLeft('0', 2), stepNum, juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui
