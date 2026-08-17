#include "PresetBrowserOverlay.h"

#include <cmath>

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        using layout::kPresetExplorerFacetRowGap;
        using layout::kPresetExplorerFacetRowHeight;
        using layout::kPresetExplorerFacetStripHeight;
        using layout::kPresetExplorerBankPillHeight;
        using layout::kPresetExplorerBottomBarHeight;
        using layout::kPresetExplorerCategoryRowHeight;
        using layout::kPresetExplorerCenterColumnWidth;
        using layout::kPresetExplorerDetailWaveformHeight;
        using layout::kPresetExplorerLeftColumnWidth;
        using layout::kPresetExplorerLoadButtonHeight;
        using layout::kPresetExplorerModalHeight;
        using layout::kPresetExplorerModalWidth;
        using layout::kPresetExplorerPresetRowHeight;
        using layout::kPresetExplorerRightColumnWidth;
        using layout::kPresetExplorerSearchBarHeight;
        using layout::kPresetExplorerSearchBarWidth;
        using layout::kPresetExplorerSecondaryButtonHeight;
        using layout::kPresetExplorerTableHeaderHeight;
        using layout::kPresetExplorerTopBarHeight;

        void styleModalButton(juce::TextButton& btn, bool primary)
        {
            btn.setColour(juce::TextButton::buttonColourId, primary ? palette::kAccent : palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, primary ? palette::kBackgroundTop : palette::kTextSecondary);
        }

        [[nodiscard]] juce::Rectangle<int> centeredModalBounds(juce::Rectangle<int> host)
        {
            const int width = juce::jmin(kPresetExplorerModalWidth, host.getWidth() - 24);
            const int height = juce::jmin(kPresetExplorerModalHeight, host.getHeight() - 24);
            return host.withSizeKeepingCentre(width, height);
        }
        [[nodiscard]] bool isContextCrossoverMood(const juce::String& mood) noexcept
        {
            static const char* kContextTokens[] = {"cinematic", "score", "trailer", "game",      "worship",
                                                   "sleep",     "demo",  "ambient", "sound-design", nullptr};
            const auto lower = mood.trim().toLowerCase();
            for (std::size_t i = 0; kContextTokens[i] != nullptr; ++i)
            {
                if (lower == kContextTokens[i])
                    return true;
            }
            return false;
        }

        void collectFacetValue(content::PresetFacet facet, const content::PresetEntry& entry,
                               juce::StringArray& out)
        {
            auto addUnique = [&](const juce::String& value) {
                const auto trimmed = value.trim();
                if (trimmed.isNotEmpty() && !out.contains(trimmed, true))
                    out.add(trimmed);
            };

            switch (facet)
            {
                case content::PresetFacet::Category:
                    addUnique(entry.category);
                    break;
                case content::PresetFacet::Mood:
                    for (const auto& mood : entry.moods)
                        addUnique(mood);
                    break;
                case content::PresetFacet::Genre:
                    for (const auto& genre : entry.genres)
                        addUnique(genre);
                    for (const auto& mood : entry.moods)
                    {
                        if (isContextCrossoverMood(mood))
                            addUnique(mood);
                    }
                    // Tags like interstellar / hoover-bass often carry the demo vocabulary.
                    for (const auto& tag : entry.tags)
                    {
                        const auto lower = tag.trim().toLowerCase();
                        if (lower.containsChar('-') || lower == "interstellar" || lower.contains("hoover"))
                            addUnique(tag);
                    }
                    break;
                case content::PresetFacet::Tag:
                    for (const auto& tag : entry.tags)
                        addUnique(tag);
                    break;
            }
        }
    } // namespace

    PresetBrowserOverlay::PresetBrowserOverlay(PatchworkEightProcessor& processor, content::PresetIndex& presetIndex,
                                               content::FavoritesStore& favoritesStore)
        : processor_(processor), presetIndex_(presetIndex), favoritesStore_(favoritesStore)
    {
        setVisible(false);

        searchField_.setTextToShowWhenEmpty("Search presets by name, tag, mood...", palette::kTextDim);
        searchField_.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
        searchField_.setColour(juce::TextEditor::textColourId, palette::kTextPrimary);
        searchField_.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        searchField_.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        searchField_.setFont(fonts::label(9.0f));
        searchField_.setIndents(28, 2);
        searchField_.addListener(this);
        addAndMakeVisible(searchField_);

        styleModalButton(loadButton_, true);
        loadButton_.onClick = [this] { loadSelected(); };
        addAndMakeVisible(loadButton_);

        styleModalButton(favoriteButton_, false);
        favoriteButton_.onClick = [this] { toggleFavoriteSelected(); };
        addAndMakeVisible(favoriteButton_);

        styleModalButton(compareButton_, false);
        compareButton_.onClick = [] { /* A/B compare — future */ };
        addAndMakeVisible(compareButton_);

        auto wireFacetRow = [this](std::unique_ptr<MetadataFacetRow>& row, const juce::String& label) {
            row = std::make_unique<MetadataFacetRow>(label);
            row->onChange = [this] { rebuildLists(); };
            addAndMakeVisible(*row);
        };
        wireFacetRow(moodFacetRow_, "MOOD");
        wireFacetRow(contextFacetRow_, "CONTEXT");
        wireFacetRow(tagFacetRow_, "TAG");

        startTimerHz(4);
    }

    content::PresetMetadataFilter PresetBrowserOverlay::browseFilterWithoutFacet(content::PresetFacet facet) const
    {
        auto filter = browseFilter();
        switch (facet)
        {
            case content::PresetFacet::Category: filter.category = {}; break;
            case content::PresetFacet::Mood: filter.mood = {}; break;
            case content::PresetFacet::Genre: filter.genre = {}; break;
            case content::PresetFacet::Tag: filter.tag = {}; break;
        }
        return filter;
    }

    juce::StringArray PresetBrowserOverlay::uniqueFacetValuesForBank(content::PresetFacet facet) const
    {
        // Facet chips show vocabulary for the whole factory bank — not only favorites.
        auto filter = browseFilterWithoutFacet(facet);
        filter.favoritesOnly = false;

        juce::StringArray values;
        for (const auto& entry : presetIndex_.filtered(filter, nullptr))
        {
            if (!entryMatchesBank(entry))
                continue;
            collectFacetValue(facet, entry, values);
        }
        values.sort(true);
        return values;
    }

    content::PresetMetadataFilter PresetBrowserOverlay::browseFilter() const
    {
        content::PresetMetadataFilter filter;
        filter.query = searchField_.getText();
        if (selectedCategoryKey_ == kFavoritesCategoryKey)
            filter.favoritesOnly = true;
        else if (selectedCategoryKey_.isNotEmpty() && selectedCategoryKey_ != kAllCategoryKey)
            filter.category = selectedCategoryKey_;
        if (moodFacetRow_ != nullptr)
            filter.mood = moodFacetRow_->getSelectedValue();
        if (contextFacetRow_ != nullptr)
            filter.genre = contextFacetRow_->getSelectedValue();
        if (tagFacetRow_ != nullptr)
            filter.tag = tagFacetRow_->getSelectedValue();
        return filter;
    }

    void PresetBrowserOverlay::showOverlay()
    {
        presetIndex_.rescan();
        searchField_.setText({}, juce::dontSendNotification);
        selectedBank_ = PresetBank::Factory;
        selectedCategoryKey_ = kAllCategoryKey;
        selectedRow_ = -1;
        presetListScrollY_ = 0;
        if (moodFacetRow_ != nullptr)
            moodFacetRow_->setSelectedValue({});
        if (contextFacetRow_ != nullptr)
            contextFacetRow_->setSelectedValue({});
        if (tagFacetRow_ != nullptr)
            tagFacetRow_->setSelectedValue({});
        rebuildLists();
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        toFront(true);
        searchField_.grabKeyboardFocus();
        resized();
        repaint();
    }

    void PresetBrowserOverlay::dismiss()
    {
        setVisible(false);
        setInterceptsMouseClicks(false, true);
        if (onClosed)
            onClosed();
    }

    bool PresetBrowserOverlay::entryMatchesBank(const content::PresetEntry& entry) const
    {
        const auto path = entry.absolutePath.toLowerCase();
        switch (selectedBank_)
        {
            case PresetBank::Factory:
                return path.contains("/factory/") || path.contains("/showcase/");
            case PresetBank::User:
                return path.contains("/user/");
            case PresetBank::Community:
                return path.contains("/community/") || path.contains("/comm/");
        }
        return true;
    }

    void PresetBrowserOverlay::rebuildFacetRows()
    {
        const auto mood = moodFacetRow_ != nullptr ? moodFacetRow_->getSelectedValue() : juce::String();
        const auto context = contextFacetRow_ != nullptr ? contextFacetRow_->getSelectedValue() : juce::String();
        const auto tag = tagFacetRow_ != nullptr ? tagFacetRow_->getSelectedValue() : juce::String();

        if (moodFacetRow_ != nullptr)
        {
            moodFacetRow_->setValues(uniqueFacetValuesForBank(content::PresetFacet::Mood));
            moodFacetRow_->setSelectedValue(mood);
            moodFacetRow_->setVisible(moodFacetRow_->hasFacetValues());
        }
        if (contextFacetRow_ != nullptr)
        {
            contextFacetRow_->setValues(uniqueFacetValuesForBank(content::PresetFacet::Genre));
            contextFacetRow_->setSelectedValue(context);
            contextFacetRow_->setVisible(contextFacetRow_->hasFacetValues());
        }
        if (tagFacetRow_ != nullptr)
        {
            tagFacetRow_->setValues(uniqueFacetValuesForBank(content::PresetFacet::Tag));
            tagFacetRow_->setSelectedValue(tag);
            tagFacetRow_->setVisible(tagFacetRow_->hasFacetValues());
        }
    }

    void PresetBrowserOverlay::selectMetadataFacet(content::PresetFacet facet, const juce::String& value)
    {
        switch (facet)
        {
            case content::PresetFacet::Mood:
                if (moodFacetRow_ != nullptr)
                    moodFacetRow_->setSelectedValue(value);
                break;
            case content::PresetFacet::Genre:
                if (contextFacetRow_ != nullptr)
                    contextFacetRow_->setSelectedValue(value);
                break;
            case content::PresetFacet::Tag:
                if (tagFacetRow_ != nullptr)
                    tagFacetRow_->setSelectedValue(value);
                break;
            default:
                break;
        }
        selectedRow_ = -1;
        rebuildLists();
    }

    void PresetBrowserOverlay::rebuildLists()
    {
        rebuildFacetRows();

        categoryKeys_.clear();
        categoryLabels_.clear();
        categoryCounts_.clear();

        juce::Array<content::PresetEntry> bankEntries;
        for (const auto& entry : presetIndex_.allEntries())
        {
            if (entryMatchesBank(entry))
                bankEntries.add(entry);
        }

        int favoritesCount = 0;
        for (const auto& entry : bankEntries)
        {
            if (favoritesStore_.isFavorite(entry.absolutePath))
                ++favoritesCount;
        }

        categoryKeys_.add(kAllCategoryKey);
        categoryLabels_.add("◆ ALL PRESETS");
        categoryCounts_.add(bankEntries.size());

        categoryKeys_.add(kFavoritesCategoryKey);
        categoryLabels_.add("★ FAVORITES");
        categoryCounts_.add(favoritesCount);

        juce::StringArray seenCategories;
        juce::StringArray sortedCategories;
        for (const auto& entry : bankEntries)
        {
            const auto cat = entry.category.trim();
            if (cat.isEmpty())
                continue;
            if (!seenCategories.contains(cat, true))
            {
                seenCategories.add(cat);
                sortedCategories.add(cat);
            }
        }
        sortedCategories.sort(true);

        for (const auto& cat : sortedCategories)
        {
            int count = 0;
            for (const auto& entry : bankEntries)
            {
                if (entry.category.trim().equalsIgnoreCase(cat))
                    ++count;
            }
            categoryKeys_.add(cat);
            categoryLabels_.add("◆ " + cat.toUpperCase());
            categoryCounts_.add(count);
        }

        if (selectedCategoryKey_.isEmpty())
            selectedCategoryKey_ = kAllCategoryKey;

        const auto filter = browseFilter();
        const juce::StringArray* fav = filter.favoritesOnly ? &favoritesStore_.paths() : nullptr;
        visibleEntries_.clear();
        for (const auto& entry : presetIndex_.filtered(filter, fav))
        {
            if (entryMatchesBank(entry))
                visibleEntries_.add(entry);
        }

        presetListScrollY_ = 0;
        if (selectedRow_ >= visibleEntries_.size())
            selectedRow_ = visibleEntries_.isEmpty() ? -1 : 0;
        if (selectedRow_ < 0 && !visibleEntries_.isEmpty())
            selectedRow_ = 0;

        resized();
        repaint();

        if (onFiltersChanged)
            onFiltersChanged();
    }

    void PresetBrowserOverlay::selectCategory(const juce::String& categoryKey)
    {
        selectedCategoryKey_ = categoryKey;
        selectedRow_ = -1;
        rebuildLists();
    }

    void PresetBrowserOverlay::selectRow(int row)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;
        selectedRow_ = row;
        repaint();
    }

    void PresetBrowserOverlay::selectBank(PresetBank bank)
    {
        if (selectedBank_ == bank)
            return;
        selectedBank_ = bank;
        selectedCategoryKey_ = kAllCategoryKey;
        selectedRow_ = -1;
        rebuildLists();
    }

    void PresetBrowserOverlay::loadSelected()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
            return;
        processor_.loadPatchFromFile(visibleEntries_.getReference(selectedRow_).absolutePath);
        dismiss();
    }

    void PresetBrowserOverlay::toggleFavoriteSelected()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
            return;
        favoritesStore_.toggleFavorite(visibleEntries_.getReference(selectedRow_).absolutePath);
        rebuildLists();
    }

    void PresetBrowserOverlay::stepSelection(int direction)
    {
        if (visibleEntries_.isEmpty())
            return;

        if (selectedRow_ < 0)
            selectedRow_ = 0;
        else
            selectedRow_ = juce::jlimit(0, visibleEntries_.size() - 1, selectedRow_ + direction);

        const int rowStride = kPresetExplorerPresetRowHeight + 2;
        const int rowTop = selectedRow_ * rowStride;
        const int rowBottom = rowTop + kPresetExplorerPresetRowHeight;
        if (rowTop < presetListScrollY_)
            presetListScrollY_ = rowTop;
        else if (rowBottom > presetListScrollY_ + presetTableBounds_.getHeight())
            presetListScrollY_ = rowBottom - presetTableBounds_.getHeight();

        repaint();
    }

    juce::String PresetBrowserOverlay::formatAuthorForEntry(const content::PresetEntry& entry) const
    {
        if (entry.author.isNotEmpty())
            return entry.author.toUpperCase();
        if (!entry.tags.isEmpty())
            return entry.tags[0].toUpperCase();
        return "MURMUR AUDIO";
    }

    juce::String PresetBrowserOverlay::formatDateForEntry(const content::PresetEntry& entry) const
    {
        juce::File file(entry.absolutePath);
        if (file.existsAsFile())
        {
            const auto time = file.getLastModificationTime();
            return juce::String(time.getDayOfMonth()).paddedLeft('0', 2) + "/"
                   + juce::String(time.getMonth() + 1).paddedLeft('0', 2) + "/"
                   + juce::String(time.getYear()).substring(2);
        }
        return "—";
    }

    int PresetBrowserOverlay::ratingForEntry(const content::PresetEntry& entry) const
    {
        if (favoritesStore_.isFavorite(entry.absolutePath))
            return 5;
        return 1 + (std::abs(entry.name.hashCode()) % 5);
    }

    juce::Array<int> PresetBrowserOverlay::activeEnginesForEntry(const content::PresetEntry& entry) const
    {
        juce::Array<int> active;
        if (entry.enginesSummary.containsIgnoreCase("ALL"))
        {
            for (int i = 1; i <= 8; ++i)
                active.add(i);
            return active;
        }

        juce::StringArray parts;
        parts.addTokens(entry.enginesSummary, ",", "");
        for (const auto& part : parts)
        {
            const int engine = part.trim().getIntValue();
            if (engine >= 1 && engine <= 8)
                active.addIfNotAlreadyThere(engine);
        }

        if (active.isEmpty())
            active.add(1);
        return active;
    }

    void PresetBrowserOverlay::paintStarRating(juce::Graphics& g, juce::Rectangle<int> area, int filledStars,
                                               bool large) const
    {
        const int starSize = large ? 10 : 8;
        const int gap = large ? 2 : 2;
        auto row = area.withSizeKeepingCentre((starSize + gap) * 5 - gap, starSize);

        for (int i = 0; i < 5; ++i)
        {
            auto star = row.removeFromLeft(starSize);
            row.removeFromLeft(gap);
            const bool filled = i < filledStars;
            g.setColour(filled ? palette::kAccent : palette::kTextDim.withAlpha(0.35f));
            juce::Path starPath;
            const float cx = star.getCentreX();
            const float cy = star.getCentreY();
            const float r = static_cast<float>(starSize) * 0.45f;
            for (int p = 0; p < 5; ++p)
            {
                const float angle = static_cast<float>(p) * juce::MathConstants<float>::twoPi / 5.0f
                                    - juce::MathConstants<float>::halfPi;
                const float px = cx + std::cos(angle) * r;
                const float py = cy + std::sin(angle) * r;
                if (p == 0)
                    starPath.startNewSubPath(px, py);
                else
                    starPath.lineTo(px, py);
            }
            starPath.closeSubPath();
            g.fillPath(starPath);
        }
    }

    void PresetBrowserOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(juce::Colours::black.withAlpha(0.72f));
        paintModalPanel(g);
    }

    void PresetBrowserOverlay::paintModalPanel(juce::Graphics& g) const
    {
        g.setColour(palette::kPanelRaised.darker(0.35f));
        g.fillRoundedRectangle(modalBounds_.toFloat(), 8.0f);
        g.setColour(palette::kBorder.withAlpha(0.55f));
        g.drawRoundedRectangle(modalBounds_.toFloat().reduced(0.5f), 8.0f, 1.0f);
    }

    void PresetBrowserOverlay::paintOverChildren(juce::Graphics& g)
    {
        tagPillHits_.clear();
        paintTopBar(g);
        paintLeftColumn(g);
        paintCenterColumn(g);
        paintRightColumn(g);
        paintBottomBar(g);
    }

    void PresetBrowserOverlay::paintTopBar(juce::Graphics& g) const
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(topBarBounds_.getBottom(), static_cast<float>(modalBounds_.getX()),
                             static_cast<float>(modalBounds_.getRight()));

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(11.0f));
        g.drawText("PRESET EXPLORER", topBarBounds_.reduced(16, 0), juce::Justification::centredLeft, true);

        const int searchX = modalBounds_.getCentreX() - kPresetExplorerSearchBarWidth / 2;
        const auto searchBounds =
            juce::Rectangle<int>(searchX, topBarBounds_.getY() + 12, kPresetExplorerSearchBarWidth,
                                 kPresetExplorerSearchBarHeight);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(searchBounds.toFloat(), 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(searchBounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("⌕", searchBounds.withWidth(24), juce::Justification::centred, false);

        g.setColour(palette::kTextDim);
        g.drawRoundedRectangle(closeButtonBounds_.toFloat(), 4.0f, 1.0f);
        g.drawText("✕", closeButtonBounds_, juce::Justification::centred, false);
    }

    void PresetBrowserOverlay::paintLeftColumn(juce::Graphics& g) const
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawVerticalLine(leftColumnBounds_.getRight(), static_cast<float>(workspaceBounds_.getY()),
                           static_cast<float>(workspaceBounds_.getBottom()));

        static constexpr const char* kBankLabels[] = {"FACTORY", "USER", "COMM"};
        auto pillRow = bankPillsBounds_;
        const int pillWidth = (pillRow.getWidth() - 8) / 3;
        for (int i = 0; i < 3; ++i)
        {
            auto pill = pillRow.removeFromLeft(pillWidth);
            pillRow.removeFromLeft(4);
            const bool active = static_cast<int>(selectedBank_) == i;
            g.setColour(active ? palette::kAccent.withAlpha(0.18f) : palette::kBackgroundBottom);
            g.fillRoundedRectangle(pill.toFloat(), 4.0f);
            if (active)
            {
                g.setColour(palette::kAccent.withAlpha(0.55f));
                g.drawRoundedRectangle(pill.toFloat().reduced(0.5f), 4.0f, 1.0f);
            }
            g.setColour(active ? palette::kAccent : palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText(kBankLabels[i], pill, juce::Justification::centred, false);
        }

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("CATEGORIES", categoryListBounds_.getX(), categoryListBounds_.getY() - 14,
                   categoryListBounds_.getWidth(), 10, juce::Justification::centredLeft);

        auto rowArea = categoryListBounds_;
        for (int i = 0; i < categoryKeys_.size(); ++i)
        {
            auto row = rowArea.removeFromTop(kPresetExplorerCategoryRowHeight);
            rowArea.removeFromTop(1);
            const bool selected = categoryKeys_[i].equalsIgnoreCase(selectedCategoryKey_);
            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.14f));
                g.fillRoundedRectangle(row.toFloat(), 4.0f);
            }

            g.setColour(selected ? palette::kAccent : palette::kTextSecondary);
            g.setFont(fonts::label(9.0f));
            g.drawText(categoryLabels_[i], row.reduced(6, 0), juce::Justification::centredLeft, true);

            auto badge = row.removeFromRight(24).withSizeKeepingCentre(21, 11);
            g.setColour(palette::kBackgroundBottom);
            g.fillRoundedRectangle(badge.toFloat(), 3.0f);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText(juce::String(categoryCounts_[i]), badge, juce::Justification::centred, false);
        }
    }

    void PresetBrowserOverlay::paintCenterColumn(juce::Graphics& g) const
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawVerticalLine(centerColumnBounds_.getRight(), static_cast<float>(workspaceBounds_.getY()),
                           static_cast<float>(workspaceBounds_.getBottom()));

        auto header = presetTableBounds_.withHeight(kPresetExplorerTableHeaderHeight);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("FAV", header.getX() + 10, header.getY(), 24, 10, juce::Justification::centredLeft);
        g.drawText("PRESET NAME", header.getX() + 40, header.getY(), 184, 10, juce::Justification::centredLeft);
        g.drawText("AUTHOR", header.getX() + 236, header.getY(), 110, 10, juce::Justification::centredLeft);
        g.drawText("RATING", header.getX() + 358, header.getY(), 56, 10, juce::Justification::centredLeft);
        g.drawText("DATE", header.getX() + 426, header.getY(), 60, 10, juce::Justification::centredLeft);

        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(presetTableBounds_.getY(), static_cast<float>(presetTableBounds_.getX()),
                             static_cast<float>(presetTableBounds_.getRight()));

        auto table = presetTableBounds_;
        const int rowStride = kPresetExplorerPresetRowHeight + 2;
        const int totalHeight = visibleEntries_.size() * rowStride;
        const int maxScroll = juce::jmax(0, totalHeight - table.getHeight());
        const int listScrollY = juce::jlimit(0, maxScroll, presetListScrollY_);

        g.saveState();
        g.reduceClipRegion(table);
        table = table.withY(table.getY() - listScrollY);

        for (int row = 0; row < visibleEntries_.size(); ++row)
        {
            auto rowBounds = table.removeFromTop(kPresetExplorerPresetRowHeight);
            table.removeFromTop(2);
            const bool selected = row == selectedRow_;
            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.12f));
                g.fillRoundedRectangle(rowBounds.toFloat(), 4.0f);
                draw::strokeGlowPath(g, draw::roundedRectPath(rowBounds.toFloat(), 4.0f), 0.8f, 1.0f, true);
            }

            const auto& entry = visibleEntries_.getReference(row);
            const bool starred = favoritesStore_.isFavorite(entry.absolutePath);
            g.setColour(starred ? palette::kAccent : palette::kTextDim.withAlpha(0.45f));
            g.setFont(fonts::title(10.0f));
            g.drawText(starred ? "★" : "☆", rowBounds.getX() + 10, rowBounds.getY(), 18, rowBounds.getHeight(),
                       juce::Justification::centred, false);

            g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
            g.setFont(fonts::label(9.0f));
            g.drawText(entry.name.toUpperCase(), rowBounds.getX() + 40, rowBounds.getY(), 184, rowBounds.getHeight(),
                       juce::Justification::centredLeft, true);
            g.drawText(formatAuthorForEntry(entry), rowBounds.getX() + 236, rowBounds.getY(), 110,
                       rowBounds.getHeight(), juce::Justification::centredLeft, true);
            paintStarRating(g, rowBounds.withX(rowBounds.getX() + 348).withWidth(56), ratingForEntry(entry), false);
            g.setColour(palette::kTextDim);
            g.drawText(formatDateForEntry(entry), rowBounds.getX() + 426, rowBounds.getY(), 60, rowBounds.getHeight(),
                       juce::Justification::centredLeft, true);
        }
        g.restoreState();
    }

    void PresetBrowserOverlay::paintRightColumn(juce::Graphics& g) const
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(10.0f));
            g.drawText("Select a preset to preview", rightColumnBounds_.reduced(12), juce::Justification::centred, true);
            return;
        }

        const auto& entry = visibleEntries_.getReference(selectedRow_);
        auto area = rightColumnBounds_.reduced(12);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("PRESET PROFILE", area.removeFromTop(10), juce::Justification::centredLeft, true);
        area.removeFromTop(2);

        g.setColour(palette::kAccent);
        g.setFont(fonts::title(13.0f));
        g.drawText(entry.name.toUpperCase(), area.removeFromTop(18), juce::Justification::centredLeft, true);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("BY " + formatAuthorForEntry(entry), area.removeFromTop(12), juce::Justification::centredLeft, true);
        area.removeFromTop(12);

        auto waveBox = area.removeFromTop(kPresetExplorerDetailWaveformHeight);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(waveBox.toFloat(), 6.0f);
        g.setColour(palette::kTextDim);
        g.drawText("WAVEPROFILE", waveBox.reduced(6).removeFromTop(10), juce::Justification::centredLeft, true);
        auto canvas = waveBox.reduced(6, 18);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(canvas.getCentreY(), static_cast<float>(canvas.getX()),
                             static_cast<float>(canvas.getRight()));
        juce::Path waveA, waveB;
        waveA.startNewSubPath(static_cast<float>(canvas.getX()), static_cast<float>(canvas.getCentreY()));
        waveB.startNewSubPath(static_cast<float>(canvas.getX()), static_cast<float>(canvas.getCentreY()));
        for (int x = 0; x < canvas.getWidth(); ++x)
        {
            const float t = static_cast<float>(x) / static_cast<float>(juce::jmax(1, canvas.getWidth()));
            waveA.lineTo(static_cast<float>(canvas.getX() + x),
                         static_cast<float>(canvas.getCentreY()) + std::sin(t * 9.0f) * 16.0f);
            waveB.lineTo(static_cast<float>(canvas.getX() + x),
                         static_cast<float>(canvas.getCentreY()) + std::sin(t * 13.0f + 0.6f) * 10.0f);
        }
        g.setColour(palette::kAccent.withAlpha(0.85f));
        g.strokePath(waveA, juce::PathStrokeType(1.3f));
        g.setColour(palette::kAccentWarm.withAlpha(0.75f));
        g.strokePath(waveB, juce::PathStrokeType(1.1f));

        area.removeFromTop(12);
        g.setColour(palette::kTextDim);
        g.drawText("ENGINE ALLOCATION", area.removeFromTop(10), juce::Justification::centredLeft, true);
        area.removeFromTop(4);
        const auto activeEngines = activeEnginesForEntry(entry);
        auto ledRow = area.removeFromTop(21);
        const int ledStride = ledRow.getWidth() / 8;
        for (int engine = 1; engine <= 8; ++engine)
        {
            auto slot = ledRow.removeFromLeft(ledStride);
            const bool active = activeEngines.contains(engine);
            g.setColour(active ? palette::kAccent : palette::kTextDim.withAlpha(0.25f));
            g.fillEllipse(slot.getX(), static_cast<float>(slot.getY()), 10.0f, 10.0f);
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText(juce::String(engine), slot.withTrimmedTop(12), juce::Justification::centred, false);
        }

        area.removeFromTop(12);
        g.setColour(palette::kTextDim);
        g.drawText("METADATA", area.removeFromTop(10), juce::Justification::centredLeft, true);
        area.removeFromTop(4);

        struct MetaPill
        {
            juce::String value;
            content::PresetFacet facet;
        };
        juce::Array<MetaPill> pills;
        for (const auto& mood : entry.moods)
            pills.add({mood, content::PresetFacet::Mood});
        for (const auto& genre : entry.genres)
            pills.add({genre, content::PresetFacet::Genre});
        for (const auto& tag : entry.tags)
            pills.add({tag, content::PresetFacet::Tag});

        const auto filter = browseFilter();
        int tagX = area.getX();
        int tagY = area.getY();
        if (pills.isEmpty())
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(8.0f));
            g.drawText("No metadata tags", area.removeFromTop(14), juce::Justification::centredLeft, true);
        }
        else
        {
            for (const auto& pillSpec : pills)
            {
                const auto label = pillSpec.value.toUpperCase();
                const int tagW = juce::jmax(28, label.length() * 6 + 10);
                if (tagX + tagW > area.getRight())
                {
                    tagX = area.getX();
                    tagY += 18;
                }
                auto pill = juce::Rectangle<int>(tagX, tagY, tagW, 13);
                const bool active =
                    (pillSpec.facet == content::PresetFacet::Mood && pillSpec.value.equalsIgnoreCase(filter.mood))
                    || (pillSpec.facet == content::PresetFacet::Genre && pillSpec.value.equalsIgnoreCase(filter.genre))
                    || (pillSpec.facet == content::PresetFacet::Tag && pillSpec.value.equalsIgnoreCase(filter.tag));

                g.setColour(active ? palette::kAccent.withAlpha(0.18f) : palette::kBackgroundBottom);
                g.fillRoundedRectangle(pill.toFloat(), 6.0f);
                g.setColour(active ? palette::kAccent.withAlpha(0.75f) : palette::kAccent.withAlpha(0.35f));
                g.drawRoundedRectangle(pill.toFloat().reduced(0.5f), 6.0f, 1.0f);
                g.setColour(active ? palette::kAccent : palette::kTextSecondary);
                g.setFont(fonts::label(8.0f));
                g.drawText(label, pill, juce::Justification::centred, true);
                tagPillHits_.add({pillSpec.value, pillSpec.facet, pill});
                tagX += tagW + 4;
            }
        }

        area.setY(tagY + 20);
        g.setColour(palette::kTextDim);
        g.drawText("DESCRIPTION", area.removeFromTop(10), juce::Justification::centredLeft, true);
        area.removeFromTop(4);
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);
        const auto desc = entry.description.isNotEmpty()
                              ? entry.description
                              : "Deep hybrid preset with multi-engine routing and expressive modulation.";
        g.drawFittedText(desc, area.removeFromTop(44), juce::Justification::topLeft, 4);
    }

    void PresetBrowserOverlay::paintBottomBar(juce::Graphics& g) const
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(bottomBarBounds_.getY(), static_cast<float>(modalBounds_.getX()),
                             static_cast<float>(modalBounds_.getRight()));

        juce::String countText = juce::String(visibleEntries_.size()) + " presets";
        if (selectedCategoryKey_.isNotEmpty() && selectedCategoryKey_ != kFavoritesCategoryKey)
            countText += " · " + selectedCategoryKey_.toUpperCase();

        const auto filter = browseFilter();
        if (filter.mood.isNotEmpty())
            countText += " · " + filter.mood.toUpperCase();
        if (filter.genre.isNotEmpty())
            countText += " · " + filter.genre.toUpperCase();
        if (filter.tag.isNotEmpty())
            countText += " · #" + filter.tag.toUpperCase();

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(countText, bottomBarBounds_.reduced(16, 0), juce::Justification::centredLeft, true);

        g.setColour(palette::kTextSecondary);
        g.drawText("◀ PREV", paginationPrevBounds_, juce::Justification::centred, true);
        g.drawText("NEXT ▶", paginationNextBounds_, juce::Justification::centred, true);
    }

    void PresetBrowserOverlay::resized()
    {
        modalBounds_ = centeredModalBounds(getLocalBounds());
        topBarBounds_ = modalBounds_.removeFromTop(kPresetExplorerTopBarHeight);
        bottomBarBounds_ = modalBounds_.removeFromBottom(kPresetExplorerBottomBarHeight);

        int facetStripHeight = 0;
        auto accountFacetRow = [&](const std::unique_ptr<MetadataFacetRow>& row) {
            if (row == nullptr || !row->isVisible())
                return;
            if (facetStripHeight > 0)
                facetStripHeight += kPresetExplorerFacetRowGap;
            facetStripHeight += kPresetExplorerFacetRowHeight;
        };
        accountFacetRow(moodFacetRow_);
        accountFacetRow(contextFacetRow_);
        accountFacetRow(tagFacetRow_);
        if (facetStripHeight > 0)
            facetStripHeight += 12;

        facetStripBounds_ = facetStripHeight > 0 ? modalBounds_.removeFromTop(facetStripHeight) : juce::Rectangle<int>();
        workspaceBounds_ = modalBounds_;

        leftColumnBounds_ = workspaceBounds_.removeFromLeft(kPresetExplorerLeftColumnWidth);
        centerColumnBounds_ = workspaceBounds_.removeFromLeft(kPresetExplorerCenterColumnWidth);
        rightColumnBounds_ = workspaceBounds_;

        closeButtonBounds_ = topBarBounds_.removeFromRight(40).withSizeKeepingCentre(24, 24);

        const int searchX = getLocalBounds().getCentreX() - kPresetExplorerSearchBarWidth / 2;
        searchField_.setBounds(searchX, topBarBounds_.getY() + 12, kPresetExplorerSearchBarWidth,
                               kPresetExplorerSearchBarHeight);

        if (!facetStripBounds_.isEmpty())
        {
            auto facetInner = facetStripBounds_.reduced(12, 6);
            auto layoutFacetRow = [&](const std::unique_ptr<MetadataFacetRow>& row) {
                if (row == nullptr || !row->isVisible())
                    return;
                row->setBounds(facetInner.removeFromTop(kPresetExplorerFacetRowHeight));
                facetInner.removeFromTop(kPresetExplorerFacetRowGap);
            };
            layoutFacetRow(moodFacetRow_);
            layoutFacetRow(contextFacetRow_);
            layoutFacetRow(tagFacetRow_);
        }

        auto leftInner = leftColumnBounds_.reduced(12);
        bankPillsBounds_ = leftInner.removeFromTop(kPresetExplorerBankPillHeight);
        leftInner.removeFromTop(12);
        leftInner.removeFromTop(1);
        leftInner.removeFromTop(12);
        categoryListBounds_ = leftInner;

        auto centerInner = centerColumnBounds_.reduced(12);
        centerInner.removeFromTop(kPresetExplorerTableHeaderHeight + 8);
        presetTableBounds_ = centerInner;

        auto actions = rightColumnBounds_.reduced(12);
        actions.removeFromTop(310);
        loadButton_.setBounds(actions.removeFromTop(kPresetExplorerLoadButtonHeight));
        actions.removeFromTop(4);
        auto btnRow = actions.removeFromTop(kPresetExplorerSecondaryButtonHeight);
        favoriteButton_.setBounds(btnRow.removeFromLeft((btnRow.getWidth() - 6) / 2));
        btnRow.removeFromLeft(6);
        compareButton_.setBounds(btnRow);

        auto pagination = bottomBarBounds_.removeFromRight(80).reduced(0, 9);
        paginationNextBounds_ = pagination.removeFromRight(36);
        pagination.removeFromRight(4);
        paginationPrevBounds_ = pagination;
    }

    int PresetBrowserOverlay::bankPillAt(juce::Point<int> pos) const
    {
        if (!bankPillsBounds_.contains(pos))
            return -1;
        const int relX = pos.x - bankPillsBounds_.getX();
        const int pillWidth = (bankPillsBounds_.getWidth() + 4) / 3;
        return juce::jlimit(0, 2, relX / pillWidth);
    }

    int PresetBrowserOverlay::categoryRowAt(juce::Point<int> pos) const
    {
        if (!categoryListBounds_.contains(pos))
            return -1;
        const int relativeY = pos.y - categoryListBounds_.getY();
        return relativeY / (kPresetExplorerCategoryRowHeight + 1);
    }

    int PresetBrowserOverlay::presetRowAt(juce::Point<int> pos) const
    {
        if (!presetTableBounds_.contains(pos))
            return -1;
        const int relativeY = pos.y - presetTableBounds_.getY() + presetListScrollY_;
        if (relativeY < 0)
            return -1;
        return relativeY / (kPresetExplorerPresetRowHeight + 2);
    }

    int PresetBrowserOverlay::tagPillAt(juce::Point<int> pos) const
    {
        for (int i = 0; i < tagPillHits_.size(); ++i)
        {
            if (tagPillHits_.getReference(i).bounds.contains(pos))
                return i;
        }
        return -1;
    }

    void PresetBrowserOverlay::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
    {
        if (!presetTableBounds_.contains(event.getPosition()))
            return;

        const int rowStride = kPresetExplorerPresetRowHeight + 2;
        const int totalHeight = visibleEntries_.size() * rowStride;
        const int maxScroll = juce::jmax(0, totalHeight - presetTableBounds_.getHeight());
        presetListScrollY_ = juce::jlimit(0, maxScroll, presetListScrollY_ - juce::roundToInt(wheel.deltaY * 48.0f));
        repaint();
    }

    void PresetBrowserOverlay::mouseDown(const juce::MouseEvent& event)
    {
        if (!modalBounds_.contains(event.getPosition()))
        {
            dismiss();
            return;
        }

        if (closeButtonBounds_.contains(event.getPosition()))
        {
            dismiss();
            return;
        }

        const int bankIndex = bankPillAt(event.getPosition());
        if (bankIndex >= 0)
        {
            selectBank(static_cast<PresetBank>(bankIndex));
            return;
        }

        const int catRow = categoryRowAt(event.getPosition());
        if (catRow >= 0 && catRow < categoryKeys_.size())
        {
            selectCategory(categoryKeys_[catRow]);
            return;
        }

        if (paginationPrevBounds_.contains(event.getPosition()))
        {
            stepSelection(-1);
            return;
        }

        if (paginationNextBounds_.contains(event.getPosition()))
        {
            stepSelection(1);
            return;
        }

        const int presetRow = presetRowAt(event.getPosition());
        if (presetRow >= 0 && presetRow < visibleEntries_.size())
        {
            selectRow(presetRow);
            const bool starColumn = event.getPosition().x < presetTableBounds_.getX() + 34;
            if (starColumn)
                toggleFavoriteSelected();
            else if (event.getNumberOfClicks() >= 2)
                loadSelected();
            return;
        }

        const int tagHit = tagPillAt(event.getPosition());
        if (tagHit >= 0)
        {
            const auto& hit = tagPillHits_.getReference(tagHit);
            const auto filter = browseFilter();
            juce::String activeValue;
            switch (hit.facet)
            {
                case content::PresetFacet::Mood: activeValue = filter.mood; break;
                case content::PresetFacet::Genre: activeValue = filter.genre; break;
                case content::PresetFacet::Tag: activeValue = filter.tag; break;
                default: break;
            }
            selectMetadataFacet(hit.facet, hit.value.equalsIgnoreCase(activeValue) ? juce::String() : hit.value);
        }
    }

    bool PresetBrowserOverlay::keyPressed(const juce::KeyPress& key)
    {
        if (!isVisible())
            return false;

        if (key == juce::KeyPress::escapeKey)
        {
            dismiss();
            return true;
        }

        if (key == juce::KeyPress::returnKey)
        {
            loadSelected();
            return true;
        }

        if (key == juce::KeyPress::upKey)
        {
            stepSelection(-1);
            return true;
        }

        if (key == juce::KeyPress::downKey)
        {
            stepSelection(1);
            return true;
        }

        return false;
    }

    void PresetBrowserOverlay::textEditorTextChanged(juce::TextEditor&) { rebuildLists(); }

    void PresetBrowserOverlay::textEditorReturnKeyPressed(juce::TextEditor&) { loadSelected(); }

    void PresetBrowserOverlay::textEditorEscapeKeyPressed(juce::TextEditor&) { dismiss(); }

    void PresetBrowserOverlay::timerCallback()
    {
        if (isVisible())
            repaint();
    }

} // namespace pw8::plugin::ui
