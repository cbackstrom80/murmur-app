#include "PatchBrowserBar.h"

#include <cmath>

#include "../PlayModeLayout.h"
#include "../theme/FigmaKnobTokens.h"
#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "InterstellarHudDraw.h"
#include "ModRoutingUi.h"
#include "state/PluginState.h"

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

        juce::String desktopPresetBankLine(const content::PresetIndex& index,
                                           const juce::String& currentPath,
                                           const content::PresetMetadataFilter& filter,
                                           const content::FavoritesStore* favoritesStore)
        {
            juce::String bank = "INIT";
            if (currentPath.isNotEmpty())
                bank = juce::File(currentPath).getParentDirectory().getFileName().toUpperCase();

            const juce::StringArray* favoritesOnly =
                filter.favoritesOnly && favoritesStore != nullptr ? &favoritesStore->paths() : nullptr;
            const auto entries = index.filtered(filter, favoritesOnly);
            const int total = entries.size();
            int presetNum = 0;
            for (int i = 0; i < total; ++i)
            {
                if (entries.getReference(i).absolutePath == currentPath)
                {
                    presetNum = i + 1;
                    break;
                }
            }

            if (total <= 0)
                return "BANK: " + bank;

            if (presetNum <= 0)
                presetNum = 1;

            return "BANK: " + bank + " · PRESET " + juce::String(presetNum) + " / " + juce::String(total);
        }

        juce::String formatMasterGainDb(float linearGain) noexcept
        {
            const float clamped = juce::jmax(0.0001f, linearGain);
            const float db = 20.0f * std::log10(clamped);
            return juce::String(db, 1) + " dB";
        }
    } // namespace

    PatchBrowserBar::PatchBrowserBar(MurmurProcessor& processor)
        : processor_(processor),
          spectrumScope_(processor)
    {
        patchNameLabel_.setJustificationType(juce::Justification::centred);
        patchNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        patchNameLabel_.setFont(fonts::title(14.0f));
        patchNameLabel_.setInterceptsMouseClicks(true, false);
        patchNameLabel_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
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
        masterVolumeKnob_->applyFigmaContext(figma::KnobContext::PlayHeaderMaster);
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
        startTimerHz(8);
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
            patchNameLabel_.setText(name.isEmpty() ? "INIT" : name.toUpperCase(), juce::dontSendNotification);

        const auto hint = performanceHintForPatch(processor_.getCurrentPatch(), &processor_.apvts);
        if (patchHintLabel_.getText() != hint)
            patchHintLabel_.setText(hint, juce::dontSendNotification);
        patchHintLabel_.setVisible(!desktopPlayModeChrome_ && !obsidianChromeMode_ && !hint.isEmpty());

        juce::String category;
        if (const auto presetPath = processor_.getCurrentPresetPath(); presetPath.isNotEmpty())
            category = juce::File(presetPath).getParentDirectory().getFileName();
        if (category != lastPresetCategory_)
        {
            lastPresetCategory_ = category;
            repaint();
        }

        if (desktopPlayModeChrome_)
        {
            const auto bankLine =
                desktopPresetBankLine(presetIndex_, processor_.getCurrentPresetPath(), browseFilter_, favoritesStore_);
            if (bankLine != desktopPresetBankText_)
            {
                desktopPresetBankText_ = bankLine;
                patchHintLabel_.setText(bankLine, juce::dontSendNotification);
                repaint();
            }

            const float bpm = processor_.getHostBpm();
            const float masterGain = processor_.apvts.getRawParameterValue(kMasterGainId) != nullptr
                                         ? processor_.apvts.getRawParameterValue(kMasterGainId)->load()
                                         : 1.0f;
            if (std::abs(bpm - desktopHostBpm_) > 0.01f || std::abs(masterGain - desktopMasterGain_) > 0.001f)
            {
                desktopHostBpm_ = bpm;
                desktopMasterGain_ = masterGain;
                repaint();
            }
        }
    }

    void PatchBrowserBar::setDesktopPlayModeChrome(bool desktopChrome)
    {
        desktopPlayModeChrome_ = desktopChrome;
        if (desktopChrome)
            obsidianChromeMode_ = false;

        spectrumScope_.setVisible(!desktopPlayModeChrome_ && !obsidianChromeMode_);
        scopeModeToggle_.setVisible(!desktopPlayModeChrome_ && !obsidianChromeMode_);
        loadButton_.setVisible(!desktopPlayModeChrome_ && !obsidianChromeMode_);
        browseButton_.setVisible(!desktopPlayModeChrome_ && !obsidianChromeMode_);
        patchHintLabel_.setVisible(!desktopPlayModeChrome_ && !obsidianChromeMode_ && !patchHintLabel_.getText().isEmpty());
        if (masterVolumeKnob_ != nullptr)
            masterVolumeKnob_->setVisible(!desktopPlayModeChrome_);

        if (desktopPlayModeChrome_)
        {
            patchNameLabel_.setFont(fonts::title(18.0f));
            patchNameLabel_.setJustificationType(juce::Justification::centred);
            patchHintLabel_.setFont(fonts::label(8.0f));
            patchHintLabel_.setJustificationType(juce::Justification::centred);
            patchHintLabel_.setVisible(true);
            patchHintLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
            patchHintLabel_.setInterceptsMouseClicks(true, false);
            patchHintLabel_.setMouseCursor(juce::MouseCursor::PointingHandCursor);
            for (auto* btn : {&prevButton_, &nextButton_})
            {
                btn->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
                btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            }
        }
        else
        {
            patchNameLabel_.setFont(fonts::title(14.0f));
            patchHintLabel_.setInterceptsMouseClicks(false, false);
            for (auto* btn : {&prevButton_, &nextButton_})
            {
                btn->setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
                btn->setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
            }
        }

        timerCallback();
        resized();
        repaint();
    }

    void PatchBrowserBar::setObsidianChromeMode(bool obsidianChrome)
    {
        if (desktopPlayModeChrome_)
            return;

        obsidianChromeMode_ = obsidianChrome;
        spectrumScope_.setVisible(!obsidianChromeMode_);
        scopeModeToggle_.setVisible(!obsidianChromeMode_);
        loadButton_.setVisible(!obsidianChromeMode_);
        browseButton_.setVisible(!obsidianChromeMode_);
        patchHintLabel_.setVisible(!obsidianChromeMode_ && !patchHintLabel_.getText().isEmpty());
        if (masterVolumeKnob_ != nullptr)
        {
            masterVolumeKnob_->setVisible(true);
            masterVolumeKnob_->applyFigmaContext(
                obsidianChromeMode_ ? figma::KnobContext::ChromeMaster : figma::KnobContext::PlayHeaderMaster);
        }
        resized();
    }

    void PatchBrowserBar::paintDesktopPlayModeChrome(juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour(juce::Colour(0xff11131a));
        g.fillRoundedRectangle(bounds, static_cast<float>(layout::kDesktopPlayModeTopBarCornerRadius));

        g.setColour(palette::kBorder.withAlpha(0.95f));
        g.drawRoundedRectangle(bounds.reduced(0.75f), static_cast<float>(layout::kDesktopPlayModeTopBarCornerRadius),
                               1.5f);

        const float padX = static_cast<float>(layout::kDesktopPlayModeTopBarPaddingX);
        auto row = bounds.reduced(padX, 0.0f);
        const float rowMidY = row.getCentreY();

        // Brand group (36:6)
        auto brandGroup = row.removeFromLeft(200.0f);
        const float logoSize = static_cast<float>(layout::kDesktopPlayModeBrandLogoSize);
        auto logo = juce::Rectangle<float>(brandGroup.getX(), rowMidY - logoSize * 0.5f, logoSize, logoSize);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(logo, 6.0f);
        branding::paintMarkGlow(g, branding::getMarkIcon(), logo.reduced(6.0f));

        auto brandText = brandGroup.withTrimmedLeft(logoSize + static_cast<float>(layout::kDesktopPlayModeBrandGroupGap));
        g.setFont(fonts::title(14.0f));
        g.setColour(palette::kTextPrimary);
        g.drawText("MURMUR · PLAY", brandText.getX(), rowMidY - 16.0f, brandText.getWidth(), 18.0f,
                   juce::Justification::centredLeft, true);

        g.setColour(palette::kAccent);
        g.fillEllipse(brandText.getX(), rowMidY + 4.0f, 6.0f, 6.0f);
        g.setFont(fonts::label(9.0f));
        g.drawText("LIVE ENGINE ONLINE", brandText.getX() + 12.0f, rowMidY + 1.0f, 140.0f, 12.0f,
                   juce::Justification::centredLeft, true);

        // Transport + clock (36:25) — right cluster
        auto transport = row.removeFromRight(static_cast<float>(layout::kDesktopPlayModeTempoBoxWidth
                                                                + layout::kDesktopPlayModeTransportGap
                                                                + layout::kDesktopPlayModeMasterOutWidth));
        const float tempoW = static_cast<float>(layout::kDesktopPlayModeTempoBoxWidth);
        const float tempoH = static_cast<float>(layout::kDesktopPlayModeTempoBoxHeight);
        auto tempoBox = juce::Rectangle<float>(transport.getX(), rowMidY - tempoH * 0.5f, tempoW, tempoH);
        g.setColour(juce::Colour(0xff161922));
        g.fillRoundedRectangle(tempoBox, 6.0f);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(tempoBox.reduced(0.5f), 6.0f, 1.0f);

        g.setFont(fonts::value(16.0f));
        g.setColour(palette::kAccent);
        g.drawText(juce::String(desktopHostBpm_, 2), tempoBox.getX(), tempoBox.getY() + 4.0f, tempoBox.getWidth(),
                   20.0f, juce::Justification::centred, true);
        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kTextDim);
        g.drawText("BPM (TAP)", tempoBox.getX(), tempoBox.getBottom() - 14.0f, tempoBox.getWidth(), 10.0f,
                   juce::Justification::centred, true);

        const float masterW = static_cast<float>(layout::kDesktopPlayModeMasterOutWidth);
        const float masterH = static_cast<float>(layout::kDesktopPlayModeMasterOutHeight);
        auto masterBox = juce::Rectangle<float>(transport.getRight() - masterW, rowMidY - masterH * 0.5f, masterW,
                                                masterH);
        g.setFont(fonts::label(8.0f));
        g.setColour(palette::kTextDim);
        g.drawText("MASTER OUT", masterBox.getX(), masterBox.getY(), 60.0f, 10.0f, juce::Justification::centredLeft,
                   true);
        g.setFont(fonts::value(8.0f));
        g.setColour(palette::kAccent);
        g.drawText(formatMasterGainDb(desktopMasterGain_), masterBox.getRight() - 48.0f, masterBox.getY(), 48.0f, 10.0f,
                   juce::Justification::centredRight, true);

        const float trackH = static_cast<float>(layout::kDesktopPlayModeMasterLedTrackHeight);
        auto track = juce::Rectangle<float>(masterBox.getX(), masterBox.getBottom() - trackH - 2.0f, masterW, trackH);
        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(track, 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.85f));
        g.drawRoundedRectangle(track.reduced(0.5f), 4.0f, 1.0f);

        const float fillNorm = juce::jlimit(0.0f, 1.0f, desktopMasterGain_ / 4.0f);
        auto fill = track.reduced(2.0f);
        fill.setWidth(fill.getWidth() * fillNorm);
        g.setColour(palette::kAccent);
        g.fillRoundedRectangle(fill, 2.0f);

        // Preset browser pill chrome (36:15) — center
        auto pill = patchNameLabel_.getBounds().toFloat().expanded(8.0f, 0.0f);
        if (patchHintLabel_.isVisible())
            pill = pill.getUnion(patchHintLabel_.getBounds().toFloat().expanded(8.0f, 2.0f));
        g.setColour(juce::Colour(0xff0a0b0e));
        g.fillRoundedRectangle(pill, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.95f));
        g.drawRoundedRectangle(pill.reduced(0.75f), 8.0f, 1.5f);
    }

    void PatchBrowserBar::mouseDown(const juce::MouseEvent& event)
    {
        if (!desktopPlayModeChrome_)
            return;

        if (event.getNumberOfClicks() >= 2 && event.x < layout::kDesktopPlayModeTopBarPaddingX + 200)
        {
            if (onExitDesktopChromeRequested)
                onExitDesktopChromeRequested();
            return;
        }

        const auto presetBounds = patchNameLabel_.getBounds().getUnion(patchHintLabel_.getBounds());
        if (presetBounds.contains(event.getPosition()) && onBrowseClicked)
            onBrowseClicked();
    }

    void PatchBrowserBar::paint(juce::Graphics& g)
    {
        const auto fullBounds = getLocalBounds().toFloat();

        if (desktopPlayModeChrome_)
        {
            paintDesktopPlayModeChrome(g, fullBounds);
            return;
        }

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

        const float brandWidth = obsidianChromeMode_
                                     ? static_cast<float>(layout::kObsidianBrandWidth)
                                     : static_cast<float>(branding::wordmarkWidth());
        auto brandArea = fullBounds.withTrimmedRight(fullBounds.getWidth() - brandWidth).reduced(8.0f, 6.0f);

        if (obsidianChromeMode_)
        {
            auto logo = brandArea.removeFromLeft(24.0f);
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(logo, 4.0f);
            branding::paintMarkGlow(g, branding::getMarkIcon(), logo.reduced(4.0f));
            brandArea.removeFromLeft(12.0f);

            auto titleArea = brandArea.removeFromTop(16.0f);
            g.setFont(fonts::label(13.0f));
            g.setColour(palette::kTextPrimary);
            g.drawText("MURMUR 8-ENGINE", titleArea, juce::Justification::centredLeft, true);

            g.setFont(fonts::label(8.0f));
            g.setColour(palette::kAccent);
            g.drawText("DISCRETE HYBRID SYNTHESIZER", brandArea.removeFromTop(10.0f),
                       juce::Justification::centredLeft, true);
        }
        else
        {
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
        }

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

            if (interstellar::isInterstellarCategory(lastPresetCategory_))
                interstellar::paintHudBadge(g, pill.expanded(1.0f), true);
        }
    }

    void PatchBrowserBar::resized()
    {
        if (desktopPlayModeChrome_)
        {
            auto bounds = getLocalBounds().reduced(layout::kDesktopPlayModeTopBarPaddingX, 0);
            const int barMidY = bounds.getCentreY();

            const int transportWidth = layout::kDesktopPlayModeTempoBoxWidth + layout::kDesktopPlayModeTransportGap
                                       + layout::kDesktopPlayModeMasterOutWidth;
            bounds.removeFromRight(transportWidth);
            bounds.removeFromLeft(200);

            auto presetArea = bounds.withSizeKeepingCentre(layout::kDesktopPlayModePresetBrowserWidth,
                                                           layout::kDesktopPlayModePresetBrowserHeight);
            presetArea = presetArea.getIntersection(bounds);
            prevButton_.setBounds(presetArea.removeFromLeft(layout::kDesktopPlayModePresetNavButtonWidth));
            nextButton_.setBounds(presetArea.removeFromRight(layout::kDesktopPlayModePresetNavButtonWidth));
            patchNameLabel_.setBounds(presetArea.removeFromTop(24).withY(barMidY - 18));
            patchHintLabel_.setBounds(presetArea.withY(barMidY + 4).withHeight(12));
            return;
        }

        if (obsidianChromeMode_)
        {
            auto bounds = getLocalBounds().reduced(layout::kObsidianChromePaddingX, 0);

            auto masterCol = bounds.removeFromRight(118);
            masterVolumeKnob_->setBounds(masterCol.removeFromRight(52).withSizeKeepingCentre(52, 46));

            auto presetArea = bounds.withSizeKeepingCentre(layout::kObsidianPresetBrowserWidth, 32);
            presetArea = presetArea.getIntersection(bounds);
            prevButton_.setBounds(presetArea.removeFromLeft(20).reduced(0, 8));
            nextButton_.setBounds(presetArea.removeFromRight(20).reduced(0, 8));
            patchNameLabel_.setBounds(presetArea.reduced(4, 6));
            patchNameLabel_.setFont(fonts::label(10.0f));
            patchNameLabel_.setJustificationType(juce::Justification::centred);
            return;
        }

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
        // Accept both the legacy .pw8 extension and the current .murmur extension --
        // see docs/REBRAND_MURMUR.md.
        fileChooser_ = std::make_unique<juce::FileChooser>("Load a patch...", juce::File(), "*.pw8;*.murmur");
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
