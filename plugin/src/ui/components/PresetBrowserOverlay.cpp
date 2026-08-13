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

        countLabel_.setFont(fonts::label(fonts::kCaptionSize));
        countLabel_.setColour(juce::Label::textColourId, palette::kTextSecondary);
        countLabel_.setJustificationType(juce::Justification::centredRight);
        panel_.addAndMakeVisible(countLabel_);

        clearFiltersButton_.onClick = [this] { clearAllFilters(); };
        clearFiltersButton_.setColour(juce::TextButton::buttonColourId, palette::kPanelRaised);
        clearFiltersButton_.setColour(juce::TextButton::textColourOffId, palette::kTextSecondary);
        clearFiltersButton_.setVisible(false);
        panel_.addAndMakeVisible(clearFiltersButton_);

        searchField_.setTextToShowWhenEmpty("Search presets...", palette::kTextSecondary);
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

        emptyStateLabel_.setText("No presets match — widen filters", juce::dontSendNotification);
        emptyStateLabel_.setFont(fonts::label(fonts::kBodyLabelSize));
        emptyStateLabel_.setColour(juce::Label::textColourId, palette::kTextDim);
        emptyStateLabel_.setJustificationType(juce::Justification::centred);
        emptyStateLabel_.setVisible(false);
        panel_.addAndMakeVisible(emptyStateLabel_);

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

    bool PresetBrowserOverlay::isFunnelNarrowed() const
    {
        return browseFilter().isNarrowed();
    }

    void PresetBrowserOverlay::clearAllFilters()
    {
        searchField_.setText({}, juce::dontSendNotification);
        favoritesToggle_.setToggleState(false, juce::dontSendNotification);
        typeFacet_.setSelectedValue({});
        moodFacet_.setSelectedValue({});
        contextFacet_.setSelectedValue({});
        tagFacet_.setSelectedValue({});
        rebuildFacetsAndList();
    }

    void PresetBrowserOverlay::showOverlay()
    {
        presetIndex_.rescan();
        clearAllFilters();
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

    void PresetBrowserOverlay::layoutFacetRows(juce::Rectangle<int>& bounds)
    {
        constexpr int kFacetRowHeight = 24;
        constexpr int kFacetRowGap = 2;

        auto placeRow = [&](MetadataFacetRow& row) {
            if (!row.isVisible())
                return;
            row.setBounds(bounds.removeFromTop(kFacetRowHeight));
            bounds.removeFromTop(kFacetRowGap);
        };

        placeRow(typeFacet_);
        placeRow(moodFacet_);
        placeRow(contextFacet_);
        placeRow(tagFacet_);
    }

    void PresetBrowserOverlay::resized()
    {
        panel_.setBounds(getLocalBounds().reduced(40, 56));
        auto bounds = panel_.getLocalBounds().reduced(14);

        auto titleRow = bounds.removeFromTop(24);
        titleLabel_.setBounds(titleRow.removeFromLeft(96));
        countLabel_.setBounds(titleRow.removeFromRight(120));
        bounds.removeFromTop(6);

        auto closeRow = bounds.removeFromTop(28);
        clearFiltersButton_.setBounds(closeRow.removeFromLeft(72));
        closeRow.removeFromLeft(8);
        closeButton_.setBounds(closeRow.removeFromRight(72));
        bounds.removeFromTop(6);

        auto searchRow = bounds.removeFromTop(28);
        favoritesToggle_.setBounds(searchRow.removeFromRight(64));
        searchRow.removeFromRight(8);
        searchField_.setBounds(searchRow);
        bounds.removeFromTop(6);

        layoutFacetRows(bounds);
        bounds.removeFromTop(6);

        listBox_.setBounds(bounds);
        emptyStateLabel_.setBounds(bounds);
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

    void PresetBrowserOverlay::updateFunnelChrome()
    {
        const int count = visibleEntries_.size();
        countLabel_.setText(juce::String(count) + (count == 1 ? " preset" : " presets"), juce::dontSendNotification);

        const bool narrowed = isFunnelNarrowed();
        clearFiltersButton_.setVisible(narrowed);

        const bool empty = visibleEntries_.isEmpty();
        emptyStateLabel_.setVisible(empty);
        listBox_.setVisible(!empty);
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
            row.setVisible(row.hasFacetValues());
        };

        refreshFacet(typeFacet_, content::PresetFacet::Category);
        refreshFacet(moodFacet_, content::PresetFacet::Mood);
        refreshFacet(contextFacet_, content::PresetFacet::Genre);
        refreshFacet(tagFacet_, content::PresetFacet::Tag);

        visibleEntries_ = presetIndex_.filtered(browseFilter(), favoritesOnly);
        refreshAmbiguousDisplayNames();
        updateFunnelChrome();
        resized();

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
        if (entry.category.isNotEmpty() && ambiguousDisplayNames_.contains(entry.name.trim(), true))
            parts.add(entry.category);
        if (entry.moods.size() > 0)
            parts.add(entry.moods.joinIntoString(", "));
        if (entry.genres.size() > 0)
            parts.add(entry.genres.joinIntoString(", "));
        return parts.joinIntoString(" · ");
    }

    juce::String PresetBrowserOverlay::formatEntryTitle(const content::PresetEntry& entry) const
    {
        if (ambiguousDisplayNames_.contains(entry.name.trim(), true) && entry.category.isNotEmpty())
            return entry.category.toUpperCase() + " · " + entry.name;
        return entry.name;
    }

    void PresetBrowserOverlay::refreshAmbiguousDisplayNames()
    {
        ambiguousDisplayNames_.clear();
        for (int i = 0; i < visibleEntries_.size(); ++i)
        {
            const auto name = visibleEntries_.getReference(i).name.trim();
            if (name.isEmpty())
                continue;

            for (int j = i + 1; j < visibleEntries_.size(); ++j)
            {
                if (visibleEntries_.getReference(j).name.trim().equalsIgnoreCase(name))
                {
                    ambiguousDisplayNames_.addIfNotAlreadyThere(name);
                    break;
                }
            }
        }
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
        g.drawText(formatEntryTitle(entry), bounds.removeFromTop(16), juce::Justification::centredLeft, true);

        g.setColour(palette::kTextSecondary);
        g.setFont(fonts::label(fonts::kBodyLabelSize));
        g.drawText(formatEntrySubline(entry), bounds, juce::Justification::centredLeft, true);

        const bool starred = favoritesStore_.isFavorite(entry.absolutePath);
        g.setColour(starred ? palette::kAccent : palette::kTextSecondary);
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
