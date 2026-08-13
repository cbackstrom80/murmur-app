#include "DesignModeEditor.h"

#include "PlayModeLayout.h"
#include "theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    DesignModeEditor::DesignModeEditor(PatchworkEightProcessor& processor) : processor_(processor)
    {
        setLookAndFeel(&lookAndFeel_);

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
        {
            auto& btn = tabButtons_[i];
            btn.setClickingTogglesState(true);
            btn.setRadioGroupId(9100);
            btn.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn.setColour(juce::TextButton::buttonOnColourId, palette::kAccentDim.withAlpha(0.55f));
            btn.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            btn.setColour(juce::TextButton::textColourOnId, palette::kAccent);
            btn.onClick = [this, page = static_cast<Page>(i)] { showPage(page); };
            addAndMakeVisible(btn);
        }

        addChildComponent(graphPage_);
        addChildComponent(matrixPage_);
        addChildComponent(fxPage_);
        addChildComponent(wavetablePage_);

        graphPage_.addAndMakeVisible(graphPanel_);
        matrixPage_.addAndMakeVisible(matrixPanel_);
        fxPage_.addAndMakeVisible(fxPanel_);
        wavetablePage_.addAndMakeVisible(wavetablePanel_);

        graphEditor_ = std::make_unique<AlgorithmGraphEditor>(processor_);
        graphPanel_.addAndMakeVisible(*graphEditor_);
        graphEditor_->onGraphApplied = [this] {
            if (onGraphApplied)
                onGraphApplied();
        };

        matrixEditor_ = std::make_unique<ModMatrixDesignPanel>(processor_);
        matrixPanel_.addAndMakeVisible(*matrixEditor_);

        openBuilderButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        openBuilderButton_.setColour(juce::TextButton::textColourOffId, palette::kAccent);
        openBuilderButton_.onClick = [] {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::InfoIcon, "Wavetable Builder",
                "External wavetable builder handoff — embedded editor lands in a later sprint.\n\n"
                "Use tools/wavetable_builder for now.");
        };

        for (auto* label : {&fxPlaceholder_, &wavetablePlaceholder_})
        {
            label->setJustificationType(juce::Justification::centred);
            label->setColour(juce::Label::textColourId, palette::kTextDim);
        }

        fxPanel_.addAndMakeVisible(fxPlaceholder_);
        wavetablePanel_.addAndMakeVisible(wavetablePlaceholder_);
        wavetablePanel_.addAndMakeVisible(openBuilderButton_);

        showPage(Page::Graph);
    }

    void DesignModeEditor::refreshFromPatch()
    {
        if (graphEditor_ != nullptr)
            graphEditor_->refreshFromPatch();
        if (matrixEditor_ != nullptr)
            matrixEditor_->refreshFromPatch();
    }

    void DesignModeEditor::showPage(Page page)
    {
        currentPage_ = page;
        graphPage_.setVisible(page == Page::Graph);
        matrixPage_.setVisible(page == Page::Matrix);
        fxPage_.setVisible(page == Page::Fx);
        wavetablePage_.setVisible(page == Page::Wavetable);

        for (std::size_t i = 0; i < tabButtons_.size(); ++i)
            tabButtons_[i].setToggleState(static_cast<Page>(i) == page, juce::dontSendNotification);

        resized();
    }

    void DesignModeEditor::paint(juce::Graphics& g)
    {
        const auto h = static_cast<float>(getHeight());
        juce::ColourGradient bg(palette::kBackgroundTop, 0.0f, 0.0f, palette::kBackgroundBottom, 0.0f, h, false);
        g.setGradientFill(bg);
        g.fillAll();
    }

    void DesignModeEditor::resized()
    {
        auto bounds = getLocalBounds().reduced(layout::kOuterMargin);

        auto tabRow = bounds.removeFromTop(layout::kTabRowHeight);
        const int tabWidth = tabRow.getWidth() / static_cast<int>(tabButtons_.size());
        for (auto& btn : tabButtons_)
            btn.setBounds(tabRow.removeFromLeft(tabWidth).reduced(2, 2));
        bounds.removeFromTop(layout::kBlockGap);

        graphPage_.setBounds(bounds);
        matrixPage_.setBounds(bounds);
        fxPage_.setBounds(bounds);
        wavetablePage_.setBounds(bounds);

        graphPanel_.setBounds(graphPage_.getLocalBounds());
        matrixPanel_.setBounds(matrixPage_.getLocalBounds());
        fxPanel_.setBounds(fxPage_.getLocalBounds());
        wavetablePanel_.setBounds(wavetablePage_.getLocalBounds());

        if (graphEditor_ != nullptr)
            graphEditor_->setBounds(graphPanel_.getContentBounds());

        if (matrixEditor_ != nullptr)
            matrixEditor_->setBounds(matrixPanel_.getContentBounds());

        fxPlaceholder_.setBounds(fxPanel_.getContentBounds());
        auto wtBounds = wavetablePanel_.getContentBounds();
        auto builderRow = wtBounds.removeFromBottom(36);
        builderRow = builderRow.withSizeKeepingCentre(160, 28);
        openBuilderButton_.setBounds(builderRow);
        wavetablePlaceholder_.setBounds(wtBounds);
    }

} // namespace pw8::plugin::ui
