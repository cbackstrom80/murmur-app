#include "PresetBrowserOverlay.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    PresetBrowserOverlay::PresetBrowserOverlay(PatchworkEightProcessor& processor, content::PresetIndex& presetIndex,
                                               content::FavoritesStore& favoritesStore)
        : processor_(processor), presetIndex_(presetIndex), favoritesStore_(favoritesStore)
    {
        setVisible(false);
        addAndMakeVisible(panel_);
        panel_.addAndMakeVisible(titleLabel_);
        titleLabel_.setFont(fonts::title(16.0f));
        titleLabel_.setColour(juce::Label::textColourId, palette::kTextPrimary);
        titleLabel_.setJustificationType(juce::Justification::centredLeft);

        searchField_.setTextToShowWhenEmpty("Search presets...", palette::kTextDim);
        searchField_.setColour(juce::TextEditor::backgroundColourId, palette::kPanelRaised);
        searchField_.setColour(juce::TextEditor::textColourId, palette::kTextPrimary);
        searchField_.setColour(juce::TextEditor::outlineColourId, palette::kBorderBright);
        searchField_.addListener(this);
        panel_.addAndMakeVisible(searchField_);

        categoryBox_.addItem("All categories", 1);
        categoryBox_.setSelectedId(1, juce::dontSendNotification);
        categoryBox_.onChange = [this] { rebuildList(); };
        categoryBox_.setColour(juce::ComboBox::backgroundColourId, palette::kPanelRaised);
        categoryBox_.setColour(juce::ComboBox::textColourId, palette::kTextPrimary);
        panel_.addAndMakeVisible(categoryBox_);

        listBox_.setColour(juce::ListBox::backgroundColourId, palette::kBackgroundBottom);
        listBox_.setOutlineThickness(0);
        panel_.addAndMakeVisible(listBox_);

        closeButton_.onClick = [this] { dismiss(); };
        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        panel_.addAndMakeVisible(closeButton_);
    }

    void PresetBrowserOverlay::showOverlay()
    {
        presetIndex_.rescan();
        categoryBox_.clear(juce::dontSendNotification);
        categoryBox_.addItem("All categories", 1);
        categoryBox_.addItem("Favorites", 2);
        int id = 3;
        for (const auto& cat : presetIndex_.uniqueCategories())
            categoryBox_.addItem(cat, id++);
        categoryBox_.setSelectedId(1, juce::dontSendNotification);
        searchField_.setText({}, juce::dontSendNotification);
        rebuildList();
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        panel_.setVisible(true);
        toFront(true);
        searchField_.grabKeyboardFocus();
    }

    void PresetBrowserOverlay::dismiss()
    {
        setVisible(false);
        setInterceptsMouseClicks(false, true);
        if (onClosed)
            onClosed();
    }

    juce::String PresetBrowserOverlay::browseCategory() const
    {
        if (categoryBox_.getSelectedId() <= 1 || browseFavoritesOnly())
            return {};
        return categoryBox_.getText();
    }

    void PresetBrowserOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop.withAlpha(0.72f));

        auto panelBounds = panel_.getBounds().toFloat();
        g.setColour(palette::kPanelRaised);
        g.fillRoundedRectangle(panelBounds, 8.0f);
        g.setColour(palette::kBorderBright);
        g.drawRoundedRectangle(panelBounds, 8.0f, 1.0f);
    }

    void PresetBrowserOverlay::resized()
    {
        panel_.setBounds(getLocalBounds().reduced(48, 80));
        auto bounds = panel_.getLocalBounds().reduced(14);

        titleLabel_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(8);
        closeButton_.setBounds(bounds.removeFromTop(28).removeFromRight(72));
        bounds.removeFromTop(8);

        auto filterRow = bounds.removeFromTop(28);
        categoryBox_.setBounds(filterRow.removeFromRight(180));
        filterRow.removeFromRight(8);
        searchField_.setBounds(filterRow);
        bounds.removeFromTop(8);

        listBox_.setBounds(bounds);
    }

    void PresetBrowserOverlay::mouseDown(const juce::MouseEvent& event)
    {
        if (!panel_.getBounds().contains(event.getPosition()))
            dismiss();
    }

    void PresetBrowserOverlay::textEditorTextChanged(juce::TextEditor&)
    {
        rebuildList();
    }

    void PresetBrowserOverlay::textEditorReturnKeyPressed(juce::TextEditor&)
    {
        if (!visibleEntries_.isEmpty())
            loadRow(0);
    }

    void PresetBrowserOverlay::textEditorEscapeKeyPressed(juce::TextEditor&)
    {
        dismiss();
    }

    void PresetBrowserOverlay::rebuildList()
    {
        const juce::StringArray* favoritesOnly = browseFavoritesOnly() ? &favoritesStore_.paths() : nullptr;
        visibleEntries_ = presetIndex_.filtered(searchField_.getText(), browseCategory(), favoritesOnly);
        listBox_.updateContent();
        listBox_.repaint();
    }

    int PresetBrowserOverlay::getNumRows()
    {
        return visibleEntries_.size();
    }

    void PresetBrowserOverlay::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;

        const auto& entry = visibleEntries_.getReference(row);
        if (selected)
            g.fillAll(palette::kAccent.withAlpha(0.22f));

        auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(6, 2);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(13.0f));
        g.drawText(entry.name, bounds.removeFromTop(16), juce::Justification::centredLeft, true);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(10.0f));
        const auto meta = entry.category + (entry.moods.isEmpty() ? juce::String() : " · " + entry.moods.joinIntoString(", "));
        g.drawText(meta, bounds, juce::Justification::centredLeft, true);

        const bool starred = favoritesStore_.isFavorite(entry.absolutePath);
        g.setColour(starred ? palette::kAccent : palette::kTextDim);
        g.setFont(fonts::title(14.0f));
        g.drawText(starred ? "*" : "o", width - 24, 0, 20, height, juce::Justification::centred, false);
    }

    void PresetBrowserOverlay::listBoxItemClicked(int row, const juce::MouseEvent& event)
    {
        if (event.x >= listBox_.getWidth() - 28)
        {
            toggleFavoriteRow(row);
            return;
        }
        loadRow(row);
    }

    void PresetBrowserOverlay::toggleFavoriteRow(int row)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;
        favoritesStore_.toggleFavorite(visibleEntries_.getReference(row).absolutePath);
        rebuildList();
    }

    void PresetBrowserOverlay::loadRow(int row)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;
        processor_.loadPatchFromFile(visibleEntries_.getReference(row).absolutePath);
        dismiss();
    }

} // namespace pw8::plugin::ui
