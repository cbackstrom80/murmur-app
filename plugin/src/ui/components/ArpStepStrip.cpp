#include "ArpStepStrip.h"

#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kMinCellWidth = 18;
        constexpr int kCellHeight = 36;
        constexpr int kCellGap = 4;
        constexpr int kPolyRowHeight = 14;
    } // namespace

    ArpStepStrip::ArpStepStrip(PatchworkEightProcessor& processor) : processor_(processor)
    {
        startTimerHz(15);
    }

    ArpStepStrip::~ArpStepStrip()
    {
        stopTimer();
    }

    int ArpStepStrip::cellWidthForCount(int count) const noexcept
    {
        const int totalGap = juce::jmax(0, count - 1) * kCellGap;
        return juce::jmax(kMinCellWidth, (getWidth() - totalGap) / count);
    }

    void ArpStepStrip::timerCallback()
    {
        const auto numSteps = processor_.getCurrentPatch().arpeggiator.numSteps;
        playheadStep_ = processor_.getArpPlayheadStep();
        noteSequenceIndex_ = processor_.getArpNoteSequenceIndex();
        if (numSteps != lastNumSteps_)
        {
            lastNumSteps_ = numSteps;
            resized();
        }
        repaint();
    }

    void ArpStepStrip::resized()
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));
        const int width = count * kMinCellWidth + juce::jmax(0, count - 1) * kCellGap;
        setSize(juce::jmax(getWidth(), width), kCellHeight + kPolyRowHeight + 18);
    }

    void ArpStepStrip::mouseDown(const juce::MouseEvent& event)
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));
        const int cellWidth = cellWidthForCount(count);
        auto bounds = getLocalBounds();
        bounds.removeFromTop(18);
        bounds.removeFromTop(kPolyRowHeight + 4);

        int x = bounds.getX();
        for (int i = 0; i < count; ++i)
        {
            juce::Rectangle<int> cell(x, bounds.getY(), cellWidth, bounds.getHeight());
            if (cell.contains(event.getPosition()))
            {
                processor_.toggleArpStepEnabled(static_cast<std::size_t>(i));
                repaint();
                return;
            }
            x += cellWidth + kCellGap;
        }
    }

    void ArpStepStrip::paint(juce::Graphics& g)
    {
        const auto& arp = processor_.getCurrentPatch().arpeggiator;
        const int count = juce::jmax(1, static_cast<int>(arp.numSteps));
        const int cellWidth = cellWidthForCount(count);

        auto bounds = getLocalBounds();
        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(10.0f));
        g.drawText("STEP PATTERN — click to toggle rest", bounds.removeFromTop(14), juce::Justification::centredLeft);

        auto polyRow = bounds.removeFromTop(kPolyRowHeight);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::value(9.0f));
        const auto numSteps = static_cast<std::size_t>(count);
        const auto noteIdx = noteSequenceIndex_ % juce::jmax<std::size_t>(1, numSteps);
        g.drawText("Polymetric: step " + juce::String(static_cast<int>(playheadStep_ % numSteps) + 1) + " / note " +
                       juce::String(static_cast<int>(noteIdx) + 1),
                   polyRow, juce::Justification::centredLeft);
        bounds.removeFromTop(4);

        for (int i = 0; i < count; ++i)
        {
            const auto& step = arp.steps[static_cast<std::size_t>(i)];
            auto cell = bounds.removeFromLeft(cellWidth).reduced(0, 1);
            if (i + 1 < count)
                bounds.removeFromLeft(kCellGap);

            const bool isPlayhead =
                arp.enabled && (static_cast<std::size_t>(i) == (playheadStep_ % static_cast<std::size_t>(count)));
            const auto cellF = cell.toFloat();
            draw::fillRecessedRoundedRect(g, cellF, 4.0f);

            if (isPlayhead)
            {
                g.setColour(palette::kAccent.withAlpha(0.35f));
                g.drawRoundedRectangle(cellF.expanded(1.0f), 5.0f, 2.0f);
            }

            if (!step.enabled)
            {
                g.setColour(palette::kTextDim.withAlpha(0.55f));
                g.setFont(fonts::label(11.0f));
                g.drawText("REST", cell, juce::Justification::centred);
                g.setColour(palette::kBorder);
                g.drawRoundedRectangle(cellF.reduced(0.5f), 4.0f, 1.0f);
                continue;
            }

            juce::Colour fill = palette::kAccent.withAlpha(step.probability < 0.999f ? 0.35f : 0.55f);
            if (step.accent)
                fill = palette::kAccentWarm.withAlpha(0.75f);
            g.setColour(fill);
            g.fillRoundedRectangle(cellF.reduced(1.5f), 3.0f);

            if (step.tie)
            {
                g.setColour(palette::kAccentWarm);
                g.drawHorizontalLine(cell.getCentreY(), static_cast<float>(cell.getX() + 2),
                                     static_cast<float>(cell.getRight() - 2));
            }

            g.setColour(palette::kTextPrimary);
            g.setFont(fonts::label(9.0f));
            juce::String glyph = juce::String(i + 1);
            if (step.ratchetCount > 1)
                glyph += " x" + juce::String(step.ratchetCount);
            if (step.accent)
                glyph += " !";
            if (step.probability < 0.999f)
                glyph += " ?";
            g.drawText(glyph, cell, juce::Justification::centred);
        }
    }

} // namespace pw8::plugin::ui
