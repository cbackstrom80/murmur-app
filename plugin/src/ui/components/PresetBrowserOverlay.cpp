#include "PresetBrowserOverlay.h"

#include "../theme/ObsidianDraw.h"
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

        favoritesToggle_.setClickingTogglesState(true);
        favoritesToggle_.setColour(juce::ToggleButton::textColourId, palette::kTextSecondary);
        favoritesToggle_.setColour(juce::ToggleButton::tickColourId, palette::kAccent);
        favoritesToggle_.onClick = [this] {
            juce::MessageManager::callAsync([this] { rebuildFacetsAndList(); });
        };
        panel_.addAndMakeVisible(favoritesToggle_);

        for (auto* row : {&typeFacet_, &moodFacet_, &contextFacet_, &tagFacet_})
        {
            row->onChange = [this] { rebuildFacetsAndList(); };
            panel_.addAndMakeVisible(*row);
        }

        listBox_.setColour(juce::ListBox::backgroundColourId, palette::kBackgroundBottom);
        listBox_.setOutlineThickness(0);
        panel_.addAndMakeVisible(listBox_);

        closeButton_.onClick = [this] { dismiss(); };
        closeButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        closeButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        panel_.addAndMakeVisible(closeButton_);
    }

    content::PresetMetadataFilter PresetBrowserOverlay::browseFilter() const
    {
        content::PresetMetadataFilter filter;
        filter.query = searchField_.getText();
        filter.category = typeFacet_.getSelectedValue();
        filter.mood = moodFacet_.getSelectedValue();
        filter.genre = contextFacet_.getSelectedValue();
        filter.tag = tagFacet_.getSelectedValue();
        filter.favoritesOnly = favoritesToggle_.getToggleState();
        return filter;
    }

    void PresetBrowserOverlay::showOverlay()
    {
        presetIndex_.rescan();
        searchField_.setText({}, juce::dontSendNotification);
        favoritesToggle_.setToggleState(false, juce::dontSendNotification);
        typeFacet_.setSelectedValue({});
        moodFacet_.setSelectedValue({});
        contextFacet_.setSelectedValue({});
        tagFacet_.setSelectedValue({});
        rebuildFacetsAndList();
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

    void PresetBrowserOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop.withAlpha(0.72f));

        auto panelBounds = panel_.getBounds().toFloat();
        draw::fillRecessedRoundedRect(g, panelBounds, 8.0f);
        draw::strokeGlowPath(g, draw::roundedRectPath(panelBounds, 8.0f), 0.55f, 1.2f, false);
    }

    void PresetBrowserOverlay::resized()
    {
        panel_.setBounds(getLocalBounds().reduced(40, 56));
        auto bounds = panel_.getLocalBounds().reduced(14);

        titleLabel_.setBounds(bounds.removeFromTop(24));
        bounds.removeFromTop(6);
        closeButton_.setBounds(bounds.removeFromTop(28).removeFromRight(72));
        bounds.removeFromTop(6);

        auto searchRow = bounds.removeFromTop(28);
        favoritesToggle_.setBounds(searchRow.removeFromRight(64));
        searchRow.removeFromRight(8);
        searchField_.setBounds(searchRow);
        bounds.removeFromTop(6);

        constexpr int kFacetRowHeight = 24;
        typeFacet_.setBounds(bounds.removeFromTop(kFacetRowHeight));
        bounds.removeFromTop(2);
        moodFacet_.setBounds(bounds.removeFromTop(kFacetRowHeight));
        bounds.removeFromTop(2);
        contextFacet_.setBounds(bounds.removeFromTop(kFacetRowHeight));
        bounds.removeFromTop(2);
        tagFacet_.setBounds(bounds.removeFromTop(kFacetRowHeight));
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
        rebuildFacetsAndList();
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

    void PresetBrowserOverlay::rebuildFacetsAndList()
    {
        const auto filter = browseFilter();
        const juce::StringArray* favoritesOnly = filter.favoritesOnly ? &favoritesStore_.paths() : nullptr;

        auto refreshFacet = [&](MetadataFacetRow& row, content::PresetFacet facet) {
            const auto values = presetIndex_.uniqueFacetValues(facet, filter, favoritesOnly);
            auto selected = row.getSelectedValue();
            if (selected.isNotEmpty() && !values.contains(selected, true))
                selected = {};
            row.setValues(values);
            row.setSelectedValue(selected);
        };

        refreshFacet(typeFacet_, content::PresetFacet::Category);
        refreshFacet(moodFacet_, content::PresetFacet::Mood);
        refreshFacet(contextFacet_, content::PresetFacet::Genre);
        refreshFacet(tagFacet_, content::PresetFacet::Tag);

        visibleEntries_ = presetIndex_.filtered(browseFilter(), favoritesOnly);
        listBox_.updateContent();
        if (visibleEntries_.isEmpty())
            listBox_.deselectAllRows();
        else
            listBox_.selectRow(0, false, true);
        listBox_.repaint();

        if (onFiltersChanged)
            onFiltersChanged();
    }

    juce::String PresetBrowserOverlay::formatEntrySubline(const content::PresetEntry& entry) const
    {
        juce::StringArray parts;
        if (entry.category.isNotEmpty())
            parts.add(entry.category);
        if (entry.moods.size() > 0)
            parts.add(entry.moods.joinIntoString(", "));
        if (entry.genres.size() > 0)
            parts.add(entry.genres.joinIntoString(", "));
        return parts.joinIntoString(" · ");
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
        const auto rowBounds = juce::Rectangle<float>(4.0f, 1.0f, static_cast<float>(width) - 8.0f,
                                                      static_cast<float>(height) - 2.0f);
        if (selected)
        {
            draw::fillRecessedRoundedRect(g, rowBounds, 4.0f);
            g.setColour(palette::kAccent.withAlpha(0.12f));
            g.fillRoundedRectangle(rowBounds, 4.0f);
            draw::strokeGlowPath(g, draw::roundedRectPath(rowBounds, 4.0f), 0.9f, 1.2f, true);
        }

        auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(6, 2);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(13.0f));
        g.drawText(entry.name, bounds.removeFromTop(16), juce::Justification::centredLeft, true);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(10.0f));
        g.drawText(formatEntrySubline(entry), bounds, juce::Justification::centredLeft, true);

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
        rebuildFacetsAndList();
    }

    void PresetBrowserOverlay::loadRow(int row)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;
        processor_.loadPatchFromFile(visibleEntries_.getReference(row).absolutePath);
        dismiss();
    }

} // namespace pw8::plugin::ui
