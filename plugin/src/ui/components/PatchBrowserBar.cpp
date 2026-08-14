#include "PatchBrowserBar.h"

#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        juce::Colour glowMix(float alpha)
        {
            return branding::glowColour().withAlpha(alpha);
        }

        void paintVerticalSeparator(juce::Graphics& g, float x, float top, float bottom)
        {
            juce::ColourGradient line(glowMix(0.55f), x, top, glowMix(0.0f), x, bottom, false);
            g.setGradientFill(line);
            g.fillRect(x, top, 1.0f, bottom - top);
        }

        juce::String performanceHintFromDescription(const std::string& description)
        {
            if (description.empty())
                return {};

            juce::String text = juce::String::fromUTF8(description.data(),
                                                       static_cast<int>(description.size()));
            text = text.trim();

            int end = -1;
            for (int i = 0; i < text.length(); ++i)
            {
                const juce::juce_wchar c = text[i];
                if (c == '.' || c == '!' || c == '?')
                {
                    end = i;
                    break;
                }
            }

            juce::String hint = end >= 0 ? text.substring(0, end + 1).trim() : text;
            if (hint.length() > 80)
                hint = hint.substring(0, 77).trimEnd() + "...";
            return hint;
        }
    } // namespace

    PatchBrowserBar::PatchBrowserBar(PatchworkEightProcessor& processor)
        : processor_(processor),
          spectrumScope_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centred);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(fonts::title(14.0f));
        addAndMakeVisible(patchNameLabel_);

        patchHintLabel_.setJustificationType(juce::Justification::centred);
        patchHintLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        patchHintLabel_.setFont(fonts::label(fonts::kCaptionSize));
        patchHintLabel_.setInterceptsMouseClicks(false, false);
        addChildComponent(patchHintLabel_);

        addAndMakeVisible(spectrumScope_);
        spectrumScope_.setViewMode(processor_.getScopeViewMode());

        scopeModeToggle_.setMode(processor_.getScopeViewMode());
        scopeModeToggle_.onModeChanged = [this](ScopeViewMode mode) {
            processor_.setScopeViewMode(mode);
            spectrumScope_.setViewMode(mode);
        };
        addAndMakeVisible(scopeModeToggle_);

        masterVolumeKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "VOL", nullptr, palette::kAccentWarm);
        masterVolumeKnob_->setHeaderCompactMode(true);
        addAndMakeVisible(*masterVolumeKnob_);

        prevButton_.onClick = [this] { stepPreset(-1); };
        nextButton_.onClick = [this] { stepPreset(1); };
        browseButton_.onClick = [this] {
            if (onBrowseClicked)
                onBrowseClicked();
        };
        for (auto* btn : {&prevButton_, &nextButton_, &browseButton_})
        {
            btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
            btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            addAndMakeVisible(*btn);
        }

        loadButton_.onClick = [this] { loadPatchFromFile(); };
        loadButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        loadButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        loadButton_.setColour(juce::ComboBox::outlineColourId, palette::kBorderBright);
        addAndMakeVisible(loadButton_);

        refreshPresetIndex();
        startTimerHz(2);
        timerCallback();
    }

    PatchBrowserBar::~PatchBrowserBar()
    {
        stopTimer();
    }

    void PatchBrowserBar::setBrowseFilters(const content::PresetMetadataFilter& filter)
    {
        browseFilter_ = filter;
    }

    void PatchBrowserBar::refreshPresetIndex()
    {
        presetIndex_.rescan();
        prevButton_.setEnabled(!presetIndex_.allEntries().isEmpty());
        nextButton_.setEnabled(!presetIndex_.allEntries().isEmpty());
    }

    void PatchBrowserBar::timerCallback()
    {
        const auto& meta = processor_.getCurrentPatch().metadata;
        const auto name = juce::String(meta.name);
        if (patchNameLabel_.getText() != name)
            patchNameLabel_.setText(name.isEmpty() ? "INIT" : name, juce::dontSendNotification);

        const auto hint = performanceHintFromDescription(meta.description);
        if (patchHintLabel_.getText() != hint)
            patchHintLabel_.setText(hint, juce::dontSendNotification);
        patchHintLabel_.setVisible(!hint.isEmpty());
    }

    void PatchBrowserBar::paint(juce::Graphics& g)
    {
        const auto fullBounds = getLocalBounds().toFloat();

        juce::ColourGradient barFill(palette::kPanelRaised.brighter(0.06f), fullBounds.getX(), fullBounds.getY(),
                                     palette::kPanel.darker(0.12f), fullBounds.getRight(), fullBounds.getBottom(), false);
        barFill.addColour(0.35, palette::kPanelRaised);
        g.setGradientFill(barFill);
        g.fillRoundedRectangle(fullBounds.reduced(0.0f, 1.0f), 8.0f);

        g.setColour(palette::kTopHighlight.withAlpha(0.08f));
        g.drawHorizontalLine(static_cast<int>(fullBounds.getY() + 1.0f), fullBounds.getX() + 8.0f,
                             fullBounds.getRight() - 8.0f);

        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(fullBounds.reduced(0.5f, 1.5f), 8.0f, 1.0f);

        const float brandWidth = static_cast<float>(branding::wordmarkWidth());
        auto brandArea = fullBounds.withTrimmedRight(fullBounds.getWidth() - brandWidth).reduced(8.0f, 6.0f);

        auto badge = brandArea.removeFromLeft(52.0f);
        g.setColour(palette::kBackgroundBottom.withAlpha(0.92f));
        g.fillRoundedRectangle(badge, 10.0f);

        g.setColour(glowMix(0.22f));
        g.drawRoundedRectangle(badge.expanded(0.5f), 10.5f, 1.25f);

        g.setColour(glowMix(0.08f));
        g.fillRoundedRectangle(badge.expanded(2.0f), 12.0f);

        branding::paintMarkGlow(g, branding::getMarkIcon(), badge.reduced(3.0f));

        brandArea.removeFromLeft(10.0f);

        auto titleArea = brandArea.removeFromTop(24.0f);
        g.setFont(fonts::title(19.0f));
        g.setColour(palette::kTextPrimary);
        g.drawText("MURMUR", titleArea, juce::Justification::centredLeft, true);

        g.setFont(fonts::label(fonts::kCaptionSize));
        g.setColour(palette::kTextSecondary);
        g.drawText("SOUND IN MOTION", brandArea.removeFromTop(14.0f), juce::Justification::centredLeft, true);

        paintVerticalSeparator(g, brandWidth - 0.5f, fullBounds.getY() + 10.0f, fullBounds.getBottom() - 10.0f);

        if (patchNameLabel_.isVisible() || patchHintLabel_.isVisible())
        {
            auto pill = patchNameLabel_.getBounds().toFloat().expanded(5.0f, 2.5f);
            if (patchHintLabel_.isVisible())
                pill = pill.getUnion(patchHintLabel_.getBounds().toFloat().expanded(5.0f, 2.0f));
            g.setColour(palette::kBackgroundBottom.withAlpha(0.78f));
            g.fillRoundedRectangle(pill, 4.0f);
            g.setColour(palette::kBorder.withAlpha(0.55f));
            g.drawRoundedRectangle(pill, 4.0f, 1.0f);
        }
    }

    void PatchBrowserBar::resized()
    {
        auto bounds = getLocalBounds().withTrimmedLeft(branding::wordmarkWidth());
        loadButton_.setBounds(bounds.removeFromRight(72).reduced(0, 8));
        bounds.removeFromRight(4);
        browseButton_.setBounds(bounds.removeFromRight(72).reduced(0, 8));
        bounds.removeFromRight(4);
        nextButton_.setBounds(bounds.removeFromRight(28).reduced(0, 10));
        prevButton_.setBounds(bounds.removeFromRight(28).reduced(0, 10));
        bounds.removeFromRight(6);
        masterVolumeKnob_->setBounds(bounds.removeFromRight(54).reduced(0, 2));
        bounds.removeFromRight(6);
        const int presetColumnWidth = juce::jmax(140, bounds.getWidth() / 4);
        auto presetColumn = bounds.removeFromRight(presetColumnWidth);
        patchNameLabel_.setBounds(presetColumn.removeFromTop(18));
        if (patchHintLabel_.isVisible())
            patchHintLabel_.setBounds(presetColumn.removeFromTop(14));
        bounds.removeFromRight(6);
        spectrumScope_.setBounds(bounds.reduced(0, 6));
        scopeModeToggle_.setBounds(spectrumScope_.getBounds().removeFromTop(18).removeFromRight(56).reduced(4, 1));
    }

    void PatchBrowserBar::stepPreset(int direction)
    {
        const auto current = processor_.getCurrentPresetPath();
        const juce::StringArray* favoritesOnly =
            browseFilter_.favoritesOnly && favoritesStore_ != nullptr ? &favoritesStore_->paths() : nullptr;
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_.nextAfter(current, browseFilter_, favoritesOnly);
        else
            entry = presetIndex_.prevBefore(current, browseFilter_, favoritesOnly);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
        timerCallback();
    }

    void PatchBrowserBar::loadPatchFromFile()
    {
        fileChooser_ = std::make_unique<juce::FileChooser>("Load a patch...", juce::File(), "*.pw8");
        const auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser_->launchAsync(flags, [this](const juce::FileChooser& chooser) {
            const auto file = chooser.getResult();
            if (!file.existsAsFile())
                return;

            processor_.loadPatchFromFile(file.getFullPathName());
            timerCallback();
        });
    }

} // namespace pw8::plugin::ui
