#include "DesignEnvelopeSegmentsPanel.h"

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/envelope/SegmentEnvelope.hpp"
#include "pw8/modulation/MorphEasing.hpp"

namespace pw8::plugin::ui
{
    DesignEnvelopeSegmentsPanel::DesignEnvelopeSegmentsPanel(PatchworkEightProcessor& processor)
        : processor_(processor)
    {
        titleLabel_.setText("ENVELOPE SEGMENTS", juce::dontSendNotification);
        titleLabel_.setFont(fonts::label(14.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        addAndMakeVisible(titleLabel_);

        subtitleLabel_.setText("Stages-style segment chains" + juce::String(fonts::kSep) + "ENV 1-8",
                               juce::dontSendNotification);
        subtitleLabel_.setFont(fonts::label(10.0f));
        subtitleLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(subtitleLabel_);

        backButton_.onClick = [this] {
            if (onClosed)
                onClosed();
        };
        addAndMakeVisible(backButton_);

        for (std::size_t i = 0; i < envPills_.size(); ++i)
        {
            envPills_[i].setButtonText("ENV " + juce::String(static_cast<int>(i + 1)));
            envPills_[i].onClick = [this, i] { setSelectedEnv(i); };
            addAndMakeVisible(envPills_[i]);
        }

        for (std::size_t i = 0; i < segmentDots_.size(); ++i)
        {
            segmentDots_[i].setButtonText(juce::String(static_cast<int>(i + 1)));
            segmentDots_[i].onClick = [this, i] { setSelectedSegment(i); };
            addAndMakeVisible(segmentDots_[i]);
        }

        typeChip_.onClick = [this] {
            if (selectedSegment_ < editChain_.segmentCount)
                cycleSegmentType(selectedSegment_);
        };
        shapeChip_.onClick = [this] {
            if (selectedSegment_ < editChain_.segmentCount)
                cycleSegmentShape(selectedSegment_);
        };
        addAndMakeVisible(typeChip_);
        addAndMakeVisible(shapeChip_);

        durationSlider_.setRange(1.0, 5000.0, 1.0);
        durationSlider_.setTextValueSuffix(" ms");
        durationSlider_.onValueChange = [this] {
            if (selectedSegment_ < editChain_.segmentCount)
            {
                editChain_.segments[selectedSegment_].durationMs = static_cast<float>(durationSlider_.getValue());
                commitChain();
            }
        };
        levelSlider_.setRange(0.0, 1.0, 0.01);
        levelSlider_.onValueChange = [this] {
            if (selectedSegment_ < editChain_.segmentCount)
            {
                editChain_.segments[selectedSegment_].level = static_cast<float>(levelSlider_.getValue());
                commitChain();
            }
        };
        loopStartSlider_.setRange(0.0, 5.0, 1.0);
        loopStartSlider_.onValueChange = [this] {
            editChain_.loopStart = static_cast<std::size_t>(loopStartSlider_.getValue());
            commitChain();
        };
        loopEndSlider_.setRange(0.0, 5.0, 1.0);
        loopEndSlider_.onValueChange = [this] {
            editChain_.loopEnd = static_cast<std::size_t>(loopEndSlider_.getValue());
            commitChain();
        };

        for (auto* l : {&durationLabel_, &levelLabel_, &loopStartLabel_, &loopEndLabel_})
        {
            l->setFont(fonts::label(9.0f));
            l->setColour(juce::Label::textColourId, palette::kTextDim);
            addAndMakeVisible(*l);
        }
        durationLabel_.setText("DURATION", juce::dontSendNotification);
        levelLabel_.setText("LEVEL", juce::dontSendNotification);
        loopStartLabel_.setText("LOOP START", juce::dontSendNotification);
        loopEndLabel_.setText("LOOP END", juce::dontSendNotification);

        for (auto* s : {&durationSlider_, &levelSlider_, &loopStartSlider_, &loopEndSlider_})
            addAndMakeVisible(*s);

        addSegmentButton_.onClick = [this] { addSegment(); };
        removeSegmentButton_.onClick = [this] { removeSegment(); };
        addAndMakeVisible(addSegmentButton_);
        addAndMakeVisible(removeSegmentButton_);

        openPlayMotionButton_.onClick = [this] {
            if (onOpenPlayMotion)
                onOpenPlayMotion();
        };
        addAndMakeVisible(openPlayMotionButton_);

        footerHint_.setText("Empty chain = legacy ADSR" + juce::String(fonts::kSep) + "Shapes reuse MorphEasing LUTs",
                            juce::dontSendNotification);
        footerHint_.setFont(fonts::micro(9.0f));
        footerHint_.setColour(juce::Label::textColourId, palette::kTextDim);
        addAndMakeVisible(footerHint_);

        addAndMakeVisible(chainPanel_);
        refreshFromPatch();
    }

    DesignEnvelopeSegmentsPanel::~DesignEnvelopeSegmentsPanel() = default;

    void DesignEnvelopeSegmentsPanel::showOverlay() { setVisible(true); }

    void DesignEnvelopeSegmentsPanel::dismiss() { setVisible(false); }

    void DesignEnvelopeSegmentsPanel::setEmbeddedInDesignMode(bool embedded)
    {
        embeddedInDesignMode_ = embedded;
        backButton_.setVisible(!embedded);
    }

    void DesignEnvelopeSegmentsPanel::refreshFromPatch()
    {
        editChain_ = processor_.getSegmentEnvelopeChain(selectedEnv_);
        if (selectedSegment_ >= editChain_.segmentCount && editChain_.segmentCount > 0)
            selectedSegment_ = editChain_.segmentCount - 1;
        refreshEnvPills();
        refreshSegmentEditor();
    }

    void DesignEnvelopeSegmentsPanel::styleEnvPill(juce::TextButton& btn, bool active)
    {
        btn.setColour(juce::TextButton::buttonColourId, active ? palette::kFigmaTeal.withAlpha(0.22f) : juce::Colour(0xff12141a));
        btn.setColour(juce::TextButton::textColourOffId, active ? palette::kFigmaTeal : palette::kTextDim);
    }

    void DesignEnvelopeSegmentsPanel::styleSegmentChip(juce::TextButton& btn, bool selected)
    {
        btn.setColour(juce::TextButton::buttonColourId, selected ? palette::kAccent.withAlpha(0.25f) : palette::kPanelRaised);
        btn.setColour(juce::TextButton::textColourOffId, selected ? palette::kAccent : palette::kTextDim);
    }

    void DesignEnvelopeSegmentsPanel::refreshEnvPills()
    {
        for (std::size_t i = 0; i < envPills_.size(); ++i)
            styleEnvPill(envPills_[i], i == selectedEnv_);
    }

    void DesignEnvelopeSegmentsPanel::refreshSegmentEditor()
    {
        for (std::size_t i = 0; i < segmentDots_.size(); ++i)
        {
            const bool active = i < editChain_.segmentCount;
            segmentDots_[i].setVisible(active);
            styleSegmentChip(segmentDots_[i], i == selectedSegment_);
        }

        if (editChain_.segmentCount == 0)
        {
            typeChip_.setEnabled(false);
            shapeChip_.setEnabled(false);
            durationSlider_.setEnabled(false);
            levelSlider_.setEnabled(false);
            loopStartSlider_.setEnabled(false);
            loopEndSlider_.setEnabled(false);
            return;
        }

        loopStartSlider_.setEnabled(true);
        loopEndSlider_.setEnabled(true);

        typeChip_.setEnabled(true);
        shapeChip_.setEnabled(true);
        durationSlider_.setEnabled(true);
        levelSlider_.setEnabled(true);

        const auto& seg = editChain_.segments[selectedSegment_];
        typeChip_.setButtonText(pw8::envelope::segmentTypeLabel(seg.type));
        const auto easing = pw8::modulation::parseMorphEasing(seg.shape.empty() ? "linear" : seg.shape);
        shapeChip_.setButtonText(pw8::modulation::morphEasingLabel(easing));
        durationSlider_.setValue(seg.durationMs, juce::dontSendNotification);
        levelSlider_.setValue(seg.level, juce::dontSendNotification);
        loopStartSlider_.setValue(static_cast<double>(editChain_.loopStart), juce::dontSendNotification);
        loopEndSlider_.setValue(static_cast<double>(editChain_.loopEnd), juce::dontSendNotification);
    }

    void DesignEnvelopeSegmentsPanel::setSelectedEnv(const std::size_t envIndex)
    {
        selectedEnv_ = juce::jmin(envIndex, envPills_.size() - 1);
        selectedSegment_ = 0;
        refreshFromPatch();
    }

    void DesignEnvelopeSegmentsPanel::setSelectedSegment(const std::size_t segmentIndex)
    {
        if (segmentIndex >= editChain_.segmentCount)
            return;
        selectedSegment_ = segmentIndex;
        refreshSegmentEditor();
    }

    void DesignEnvelopeSegmentsPanel::cycleSegmentType(const std::size_t segmentIndex)
    {
        auto& seg = editChain_.segments[segmentIndex];
        seg.type = pw8::envelope::cycleSegmentType(seg.type);
        commitChain();
    }

    void DesignEnvelopeSegmentsPanel::cycleSegmentShape(const std::size_t segmentIndex)
    {
        auto& seg = editChain_.segments[segmentIndex];
        const auto easing = pw8::modulation::parseMorphEasing(seg.shape.empty() ? "linear" : seg.shape);
        seg.shape = pw8::modulation::morphEasingToString(pw8::modulation::cycleMorphEasing(easing));
        commitChain();
    }

    void DesignEnvelopeSegmentsPanel::addSegment()
    {
        if (editChain_.segmentCount >= pw8::envelope::kMaxSegmentEnvelopeSegments)
            return;
        auto& seg = editChain_.segments[editChain_.segmentCount];
        seg = {pw8::envelope::SegmentType::Ramp, 100.0f, 1.0f, "linear"};
        ++editChain_.segmentCount;
        selectedSegment_ = editChain_.segmentCount - 1;
        commitChain();
    }

    void DesignEnvelopeSegmentsPanel::removeSegment()
    {
        if (editChain_.segmentCount == 0)
            return;
        for (std::size_t i = selectedSegment_; i + 1 < editChain_.segmentCount; ++i)
            editChain_.segments[i] = editChain_.segments[i + 1];
        --editChain_.segmentCount;
        if (selectedSegment_ >= editChain_.segmentCount && editChain_.segmentCount > 0)
            selectedSegment_ = editChain_.segmentCount - 1;
        commitChain();
    }

    void DesignEnvelopeSegmentsPanel::commitChain()
    {
        processor_.setSegmentEnvelopeChain(selectedEnv_, editChain_);
        refreshSegmentEditor();
    }

    bool DesignEnvelopeSegmentsPanel::keyPressed(const juce::KeyPress& key)
    {
        if (key == juce::KeyPress::escapeKey)
        {
            if (onClosed)
                onClosed();
            return true;
        }
        return false;
    }

    void DesignEnvelopeSegmentsPanel::paintSegmentPreview(juce::Graphics& g, juce::Rectangle<int> bounds) const
    {
        if (bounds.isEmpty())
            return;

        draw::fillRecessedRoundedRect(g, bounds.toFloat(), 4.0f);

        if (editChain_.segmentCount == 0)
        {
            g.setColour(palette::kTextSecondary);
            g.setFont(fonts::micro(9.5f));
            g.drawText("No segments yet", bounds.removeFromTop(bounds.getHeight() / 2), juce::Justification::centred);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::micro(8.5f));
            g.drawText("+ SEG adds stages" + juce::String(fonts::kSep) + "empty = legacy ADSR",
                       bounds, juce::Justification::centred);
            return;
        }

        auto plot = bounds.reduced(10, 8).toFloat();
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(static_cast<int>(plot.getBottom()), plot.getX(), plot.getRight());

        float totalMs = 0.0f;
        for (std::size_t i = 0; i < editChain_.segmentCount; ++i)
            totalMs += juce::jmax(1.0f, editChain_.segments[i].durationMs);
        if (totalMs <= 0.0f)
            totalMs = 1.0f;

        juce::Path path;
        float x = plot.getX();
        float y = plot.getBottom();
        path.startNewSubPath(x, y);

        for (std::size_t i = 0; i < editChain_.segmentCount; ++i)
        {
            const auto& seg = editChain_.segments[i];
            const float segW = plot.getWidth() * (juce::jmax(1.0f, seg.durationMs) / totalMs);
            const float targetY = plot.getBottom() - seg.level * plot.getHeight();
            const float nextX = x + segW;

            if (seg.type == pw8::envelope::SegmentType::Step)
                path.lineTo(x, targetY);
            else
                path.lineTo(nextX, targetY);

            if (seg.type == pw8::envelope::SegmentType::Hold)
                path.lineTo(nextX, targetY);
            else if (i + 1 < editChain_.segmentCount)
                path.lineTo(nextX, y);

            x = nextX;
            y = targetY;
        }

        g.setColour(palette::kFigmaTeal.withAlpha(0.85f));
        g.strokePath(path, juce::PathStrokeType(2.0f));
        g.setColour(palette::kAccentWarm.withAlpha(0.9f));
        g.fillEllipse(x - 3.0f, y - 3.0f, 6.0f, 6.0f);
    }

    void DesignEnvelopeSegmentsPanel::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundBottom);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawHorizontalLine(0, 0.0f, static_cast<float>(getWidth()));
    }

    void DesignEnvelopeSegmentsPanel::paintOverChildren(juce::Graphics& g)
    {
        paintSegmentPreview(g, chainPreviewBounds_);

        auto dotsArea = chainPanel_.getContentBounds().removeFromTop(28).reduced(8, 4);
        dotsArea = getLocalArea(&chainPanel_, dotsArea);
        if (editChain_.segmentCount < 2)
            return;

        g.setColour(palette::kFigmaTeal.withAlpha(0.45f));
        const int dotW = 28;
        const int x0 = dotsArea.getX() + dotW / 2;
        const int x1 = dotsArea.getX() + static_cast<int>((editChain_.segmentCount - 1) * dotW) + dotW / 2;
        g.drawLine(static_cast<float>(x0), static_cast<float>(dotsArea.getCentreY()),
                   static_cast<float>(x1), static_cast<float>(dotsArea.getCentreY()), 1.5f);

        if (editChain_.loopEnd >= editChain_.loopStart && editChain_.loopEnd < editChain_.segmentCount)
        {
            const int lx0 = dotsArea.getX() + static_cast<int>(editChain_.loopStart * dotW) + dotW / 2;
            const int lx1 = dotsArea.getX() + static_cast<int>(editChain_.loopEnd * dotW) + dotW / 2;
            g.setColour(palette::kAccentWarm.withAlpha(0.7f));
            g.drawLine(static_cast<float>(lx0), static_cast<float>(dotsArea.getBottom() - 2),
                       static_cast<float>(lx1), static_cast<float>(dotsArea.getBottom() - 2), 2.0f);
        }
    }

    void DesignEnvelopeSegmentsPanel::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kDesignEnvelopeSegmentsPageOuterMargin);
        auto header = bounds.removeFromTop(layout::kDesignEnvelopeSegmentsHeaderHeight);
        backButton_.setBounds(header.removeFromLeft(100).reduced(0, 8));
        header.removeFromLeft(8);
        titleLabel_.setBounds(header.removeFromTop(18));
        subtitleLabel_.setBounds(header.removeFromTop(14));

        bounds.removeFromTop(layout::kDesignDynamicsLabPageSectionGap);
        auto envRow = bounds.removeFromTop(24);
        for (auto& pill : envPills_)
        {
            pill.setBounds(envRow.removeFromLeft(56).reduced(0, 2));
            envRow.removeFromLeft(4);
        }

        auto footer = bounds.removeFromBottom(layout::kDesignEnvelopeSegmentsFooterHeight);
        openPlayMotionButton_.setBounds(footer.removeFromLeft(180).reduced(0, 4));
        footer.removeFromLeft(8);
        footerHint_.setBounds(footer);

        bounds.removeFromBottom(8);
        auto actions = bounds.removeFromBottom(28);
        addSegmentButton_.setBounds(actions.removeFromLeft(64).reduced(0, 2));
        actions.removeFromLeft(6);
        removeSegmentButton_.setBounds(actions.removeFromLeft(64).reduced(0, 2));

        bounds.removeFromBottom(8);
        const int chainHeight = juce::jmax(layout::kDesignEnvelopeSegmentsChainMinHeight, bounds.getHeight());
        chainPanel_.setBounds(bounds.removeFromTop(chainHeight));

        auto chainContent = getLocalArea(&chainPanel_, chainPanel_.getContentBounds()).reduced(10);

        auto dotRow = chainContent.removeFromTop(28);
        for (std::size_t i = 0; i < segmentDots_.size(); ++i)
            segmentDots_[i].setBounds(dotRow.removeFromLeft(28).reduced(2));

        chainPreviewBounds_ = chainContent.removeFromTop(juce::jmax(72, chainContent.getHeight() / 2)).reduced(0, 4);
        chainContent.removeFromTop(8);

        auto chipRow = chainContent.removeFromTop(24);
        typeChip_.setBounds(chipRow.removeFromLeft(48).reduced(0, 2));
        chipRow.removeFromLeft(6);
        shapeChip_.setBounds(chipRow.removeFromLeft(48).reduced(0, 2));

        chainContent.removeFromTop(8);
        durationLabel_.setBounds(chainContent.removeFromTop(12));
        durationSlider_.setBounds(chainContent.removeFromTop(22));
        chainContent.removeFromTop(6);
        levelLabel_.setBounds(chainContent.removeFromTop(12));
        levelSlider_.setBounds(chainContent.removeFromTop(22));

        chainContent.removeFromTop(8);
        auto loopRow = chainContent.removeFromTop(48);
        auto loopL = loopRow.removeFromLeft(loopRow.getWidth() / 2);
        loopStartLabel_.setBounds(loopL.removeFromTop(12));
        loopStartSlider_.setBounds(loopL);
        loopEndLabel_.setBounds(loopRow.removeFromTop(12));
        loopEndSlider_.setBounds(loopRow);
    }

} // namespace pw8::plugin::ui
