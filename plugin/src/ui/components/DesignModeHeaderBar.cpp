#include "DesignModeHeaderBar.h"

#include <optional>

#include "../theme/FigmaKnobTokens.h"
#include "../theme/BrandingAssets.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        static constexpr const char* kNavTabLabels[] = {"PLAY", "DESIGN", "MOD", "FX RACK", "BROWSE"};
        static constexpr int kNavTabWidths[] = {51, 62, 49, 69, 69};
    } // namespace

    DesignModeHeaderBar::DesignModeHeaderBar(PatchworkEightProcessor& processor)
        : processor_(processor)
    {
        presetNameLabel_.setFont(fonts::label(9.0f));
        presetNameLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        presetNameLabel_.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(presetNameLabel_);

        presetBankLabel_.setFont(fonts::label(7.0f));
        presetBankLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        presetBankLabel_.setJustificationType(juce::Justification::centredRight);
        addAndMakeVisible(presetBankLabel_);

        masterVolumeKnob_ = std::make_unique<GlowKnob>(processor_.apvts, kMasterGainId, "MASTER");
        masterVolumeKnob_->setHeaderCompactMode(true);
        masterVolumeKnob_->applyFigmaContext(figma::KnobContext::ChromeMaster);
        addAndMakeVisible(*masterVolumeKnob_);

        refreshPresetIndex();
        startTimerHz(4);
    }

    DesignModeHeaderBar::~DesignModeHeaderBar() { stopTimer(); }

    void DesignModeHeaderBar::refreshPresetIndex() { presetIndex_.rescan(); }

    void DesignModeHeaderBar::setEditorMode(layout::EditorMode mode)
    {
        editorMode_ = mode;
        repaint();
    }

    void DesignModeHeaderBar::timerCallback()
    {
        const auto& patch = processor_.getCurrentPatch();
        presetNameLabel_.setText(patch.metadata.name.empty() ? "INIT PATCH"
                                                             : juce::String(patch.metadata.name),
                                 juce::dontSendNotification);

        juce::String bank = "FACTORY";
        if (const auto path = processor_.getCurrentPresetPath(); path.isNotEmpty())
            bank = juce::File(path).getParentDirectory().getFileName().toUpperCase();
        presetBankLabel_.setText(bank, juce::dontSendNotification);
    }

    void DesignModeHeaderBar::stepPreset(int direction)
    {
        juce::StringArray favorites;
        if (favoritesStore_ != nullptr)
            favorites = favoritesStore_->paths();

        const auto current = processor_.getCurrentPresetPath();
        const juce::StringArray* favoritesOnly = favorites.isEmpty() ? nullptr : &favorites;
        std::optional<content::PresetEntry> entry;
        if (direction > 0)
            entry = presetIndex_.nextAfter(current, {}, favoritesOnly);
        else
            entry = presetIndex_.prevBefore(current, {}, favoritesOnly);

        if (!entry.has_value())
            return;

        processor_.loadPatchFromFile(entry->absolutePath);
    }

    int DesignModeHeaderBar::navTabIndexAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < static_cast<int>(navTabBounds_.size()); ++i)
        {
            if (navTabBounds_[static_cast<std::size_t>(i)].contains(pos))
                return i;
        }
        return -1;
    }

    void DesignModeHeaderBar::mouseDown(const juce::MouseEvent& event)
    {
        const int tab = navTabIndexAt(event.getPosition());
        if (tab < 0)
            return;

        if (tab == 0 && onEditorModeChanged)
            onEditorModeChanged(layout::EditorMode::Play);
        else if (tab == 1 && onEditorModeChanged)
            onEditorModeChanged(layout::EditorMode::Design);
        else if (tab == 4 && onBrowseClicked)
            onBrowseClicked();
    }

    void DesignModeHeaderBar::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(0.5f);
        draw::fillRecessedRoundedRect(g, bounds, 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);

        const int padX = layout::kDesignModeV2HeaderPaddingX;
        auto brand = bounds.removeFromLeft(173.0f).reduced(static_cast<float>(padX), 0.0f);
        brand = brand.withHeight(29.0f).withCentre({brand.getCentreX(), bounds.getCentreY()});

        const auto logo = brand.removeFromLeft(24.0f);
        branding::paintMarkGlow(g, branding::getMarkIcon(), logo.reduced(4.0f));

        g.setFont(fonts::title(11.0f));
        g.setColour(palette::kTextPrimary);
        g.drawText("MURMUR 8-ENGINE", brand.getX() + 36.0f, brand.getY(), brand.getWidth() - 36.0f, 17.0f,
                   juce::Justification::centredLeft, true);
        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kTextDim);
        g.drawText("DISCRETE HYBRID SYNTHESIZER", brand.getX() + 36.0f, brand.getY() + 19.0f, brand.getWidth() - 36.0f,
                   10.0f, juce::Justification::centredLeft, true);

        g.setFont(fonts::label(9.0f));
        int tabX = getWidth() / 2 - 158;
        const int tabY = (getHeight() - layout::kDesignModeV2NavTabHeight) / 2;
        for (int i = 0; i < 5; ++i)
        {
            const auto tab = juce::Rectangle<int>(tabX, tabY, kNavTabWidths[i], layout::kDesignModeV2NavTabHeight);
            navTabBounds_[static_cast<std::size_t>(i)] = tab;

            const bool active = (i == 0 && editorMode_ == layout::EditorMode::Play)
                                || (i == 1 && editorMode_ == layout::EditorMode::Design);
            g.setColour(active ? palette::kMurmurViolet.withAlpha(0.35f) : palette::kPanelRaised);
            g.fillRoundedRectangle(tab.toFloat(), 4.0f);
            g.setColour(active ? palette::kMurmurViolet.withAlpha(0.85f) : palette::kBorder.withAlpha(0.45f));
            g.drawRoundedRectangle(tab.toFloat().reduced(0.5f), 4.0f, 1.0f);
            g.setColour(active ? palette::kTextPrimary : palette::kTextSecondary);
            g.drawText(kNavTabLabels[i], tab, juce::Justification::centred, true);
            tabX += kNavTabWidths[i] + layout::kDesignModeV2NavTabGap;
        }
    }

    void DesignModeHeaderBar::resized()
    {
        auto right = getLocalBounds().reduced(layout::kDesignModeV2HeaderPaddingX, 7);
        right.removeFromLeft(getWidth() / 2 + 80);

        auto master = right.removeFromRight(96);
        masterVolumeKnob_->setBounds(master.removeFromRight(44).withHeight(40));

        auto preset = right.removeFromRight(layout::kDesignModeV2PresetSelectorWidth).withHeight(
            layout::kDesignModeV2PresetSelectorHeight);
        preset = preset.withY((getHeight() - preset.getHeight()) / 2);
        presetBankLabel_.setBounds(preset.removeFromRight(53).reduced(0, 10));
        presetNameLabel_.setBounds(preset.reduced(12, 10));
    }

} // namespace pw8::plugin::ui
