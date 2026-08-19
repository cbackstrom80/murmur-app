#include "PresetBrowserOverlay.h"

#include <cmath>

#include "../PlayModeLayout.h"
#include "../theme/ObsidianDraw.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "pw8/patch/Patch.hpp"

namespace pw8::plugin::ui
{
    namespace
    {
        using layout::kChromeBarHeightDesign;
        using layout::kPresetBrowserActionButtonHeight;
        using layout::kPresetBrowserCategoryRowHeight;
        using layout::kPresetBrowserCenterColumnWidth;
        using layout::kPresetBrowserDetailWaveformHeight;
        using layout::kPresetBrowserFooterHeight;
        using layout::kPresetBrowserLeftColumnWidth;
        using layout::kPresetBrowserLoadButtonHeight;
        using layout::kPresetBrowserPageColumnGap;
        using layout::kPresetBrowserPageOuterMargin;
        using layout::kPresetBrowserPresetRowHeight;
        using layout::kPresetBrowserRightColumnWidth;
        using layout::kPresetBrowserSearchBarWidth;
        using layout::kPresetBrowserSearchFieldHeight;
        using layout::kPresetBrowserTableHeaderHeight;
        using layout::kPresetExplorerFacetRowGap;
        using layout::kPresetExplorerFacetRowHeight;
        using layout::kPresetExplorerBankPillHeight;

        void styleModalButton(juce::TextButton& btn, bool primary)
        {
            btn.setColour(juce::TextButton::buttonColourId, primary ? palette::kAccent : palette::kPanelRaised);
            btn.setColour(juce::TextButton::textColourOffId, primary ? palette::kBackgroundTop : palette::kTextSecondary);
        }

        [[nodiscard]] juce::Rectangle<int> pageBounds(juce::Rectangle<int> host)
        {
            return host.reduced(kPresetBrowserPageOuterMargin);
        }

        [[nodiscard]] juce::String utf8Symbol(const char* bytes)
        {
            return juce::String(juce::CharPointer_UTF8(bytes));
        }

        constexpr int kExplorerHeartColWidth = 24;
        constexpr int kExplorerRatingColWidth = 72;
        constexpr int kExplorerStarCellWidth = 14;
        constexpr int kExplorerNameColWidth = 168;
        constexpr int kExplorerAuthorColWidth = 108;
        constexpr int kExplorerDateColWidth = 64;

        struct PresetTableColumns
        {
            int heartX = 0;
            int ratingX = 0;
            int nameX = 0;
            int authorX = 0;
            int dateX = 0;

            static PresetTableColumns forTable(const juce::Rectangle<int>& table)
            {
                const int x = table.getX();
                const int tableW = table.getWidth();
                PresetTableColumns cols;
                cols.heartX = x + 4;
                cols.ratingX = x + 4 + kExplorerHeartColWidth;
                const int fixedTail = kExplorerRatingColWidth + kExplorerDateColWidth + 12;
                const int nameAuthorW = juce::jmax(200, tableW - kExplorerHeartColWidth - fixedTail);
                cols.nameX = cols.ratingX + kExplorerRatingColWidth;
                cols.authorX = cols.nameX + nameAuthorW * 3 / 5;
                cols.dateX = cols.authorX + nameAuthorW * 2 / 5;
                return cols;
            }
        };
    } // namespace

    PresetBrowserOverlay::PresetBrowserOverlay(PatchworkEightProcessor& processor, content::PresetIndex& presetIndex,
                                               content::FavoritesStore& favoritesStore,
                                               content::PresetRatingsStore& ratingsStore)
        : processor_(processor), presetIndex_(presetIndex), favoritesStore_(favoritesStore), ratingsStore_(ratingsStore)
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
        searchField_.addKeyListener(this);
        addAndMakeVisible(searchField_);

        styleModalButton(loadButton_, true);
        loadButton_.onClick = [this] { loadSelected(); };
        addAndMakeVisible(loadButton_);

        styleModalButton(favoriteButton_, false);
        favoriteButton_.onClick = [this] { toggleFavoriteSelected(); };
        addAndMakeVisible(favoriteButton_);

        styleModalButton(compareButton_, false);
        compareButton_.onClick = [this] { toggleCompare(); };
        addAndMakeVisible(compareButton_);

        styleModalButton(exportButton_, false);
        exportButton_.onClick = [this] { exportSelected(); };
        addAndMakeVisible(exportButton_);

        auto wireFacetRow = [this](std::unique_ptr<MetadataFacetRow>& row, const juce::String& label) {
            row = std::make_unique<MetadataFacetRow>(label);
            row->onChange = [this] { rebuildLists(false); };
            addAndMakeVisible(*row);
        };
        wireFacetRow(moodFacetRow_, "MOOD");
        wireFacetRow(contextFacetRow_, "CONTEXT");
        wireFacetRow(tagFacetRow_, "TAG");
    }

    content::PresetIndex::EntryPredicate PresetBrowserOverlay::bankEntryPredicate() const
    {
        return [this](const content::PresetEntry& entry) { return entryMatchesBank(entry); };
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
        lastSearchQuery_.clear();
        selectedRow_ = -1;
        presetListScrollY_ = 0;
        categoryListScrollY_ = 0;
        if (moodFacetRow_ != nullptr)
            moodFacetRow_->setSelectedValue({});
        if (contextFacetRow_ != nullptr)
            contextFacetRow_->setSelectedValue({});
        if (tagFacetRow_ != nullptr)
            tagFacetRow_->setSelectedValue({});
        rebuildLists(true);
        syncSelectionToCurrentPreset();
        syncActionButtons();
        setVisible(true);
        setInterceptsMouseClicks(true, true);
        toFront(true);
        searchField_.grabKeyboardFocus();
        resized();
        repaint();
    }

    void PresetBrowserOverlay::dismiss()
    {
        restoreCompareIfNeeded();
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
            auto filter = browseFilterWithoutFacet(content::PresetFacet::Mood);
            filter.favoritesOnly = false;
            moodFacetRow_->setValues(presetIndex_.uniqueFacetValues(content::PresetFacet::Mood, filter, nullptr,
                                                                    bankEntryPredicate()));
            moodFacetRow_->setSelectedValue(mood);
            moodFacetRow_->setVisible(moodFacetRow_->hasFacetValues());
        }
        if (contextFacetRow_ != nullptr)
        {
            auto filter = browseFilterWithoutFacet(content::PresetFacet::Genre);
            filter.favoritesOnly = false;
            contextFacetRow_->setValues(presetIndex_.uniqueFacetValues(content::PresetFacet::Genre, filter, nullptr,
                                                                      bankEntryPredicate()));
            contextFacetRow_->setSelectedValue(context);
            contextFacetRow_->setVisible(contextFacetRow_->hasFacetValues());
        }
        if (tagFacetRow_ != nullptr)
        {
            auto filter = browseFilterWithoutFacet(content::PresetFacet::Tag);
            filter.favoritesOnly = false;
            tagFacetRow_->setValues(presetIndex_.uniqueFacetValues(content::PresetFacet::Tag, filter, nullptr,
                                                                  bankEntryPredicate()));
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
        rebuildLists(false);
    }

    void PresetBrowserOverlay::rebuildLists(bool resetPresetScroll)
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
        categoryLabels_.add("ALL PRESETS");
        categoryCounts_.add(bankEntries.size());

        categoryKeys_.add(kFavoritesCategoryKey);
        categoryLabels_.add(utf8Symbol("\xe2\x99\xa5") + " FAVORITES");
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
            categoryLabels_.add(cat.toUpperCase());
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

        if (resetPresetScroll)
            presetListScrollY_ = 0;
        if (selectedRow_ >= visibleEntries_.size())
            selectedRow_ = visibleEntries_.isEmpty() ? -1 : visibleEntries_.size() - 1;

        resized();
        repaint();
        syncActionButtons();

        if (onFiltersChanged)
            onFiltersChanged();
    }

    void PresetBrowserOverlay::syncSelectionToCurrentPreset()
    {
        const auto currentPath = processor_.getCurrentPresetPath();

        PresetBank targetBank = selectedBank_;
        juce::String targetCategory = selectedCategoryKey_;

        if (currentPath.isNotEmpty())
        {
            for (const auto& entry : presetIndex_.allEntries())
            {
                if (entry.absolutePath != currentPath)
                    continue;

                const auto path = entry.absolutePath.toLowerCase();
                if (path.contains("/user/"))
                    targetBank = PresetBank::User;
                else if (path.contains("/community/") || path.contains("/comm/"))
                    targetBank = PresetBank::Community;
                else
                    targetBank = PresetBank::Factory;

                const auto category = entry.category.trim();
                targetCategory = category.isEmpty() ? kAllCategoryKey : category;
                break;
            }
        }

        const bool contextChanged = targetBank != selectedBank_ || targetCategory != selectedCategoryKey_;
        selectedBank_ = targetBank;
        selectedCategoryKey_ = targetCategory;
        if (contextChanged)
            rebuildLists(false);

        if (currentPath.isEmpty())
        {
            if (selectedRow_ < 0 && !visibleEntries_.isEmpty())
                selectedRow_ = 0;
            scrollSelectedRowIntoView();
            return;
        }

        for (int i = 0; i < visibleEntries_.size(); ++i)
        {
            if (visibleEntries_.getReference(i).absolutePath == currentPath)
            {
                selectedRow_ = i;
                scrollSelectedRowIntoView();
                return;
            }
        }

        if (selectedRow_ < 0 && !visibleEntries_.isEmpty())
            selectedRow_ = 0;
        scrollSelectedRowIntoView();
    }

    void PresetBrowserOverlay::scrollSelectedRowIntoView()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()) || presetTableBounds_.isEmpty())
            return;

        const int rowStride = kPresetBrowserPresetRowHeight + 2;
        const int rowTop = selectedRow_ * rowStride;
        const int rowBottom = rowTop + kPresetBrowserPresetRowHeight;
        if (rowTop < presetListScrollY_)
            presetListScrollY_ = rowTop;
        else if (rowBottom > presetListScrollY_ + presetTableBounds_.getHeight())
            presetListScrollY_ = rowBottom - presetTableBounds_.getHeight();

        const int totalHeight = visibleEntries_.size() * rowStride;
        const int maxScroll = juce::jmax(0, totalHeight - presetTableBounds_.getHeight());
        presetListScrollY_ = juce::jlimit(0, maxScroll, presetListScrollY_);
    }

    void PresetBrowserOverlay::clearSearch()
    {
        searchField_.setText({}, juce::dontSendNotification);
        lastSearchQuery_.clear();
        selectedRow_ = -1;
        rebuildLists(true);
        syncSelectionToCurrentPreset();
        searchField_.grabKeyboardFocus();
    }

    void PresetBrowserOverlay::selectCategory(const juce::String& categoryKey)
    {
        selectedCategoryKey_ = categoryKey;
        selectedRow_ = -1;
        rebuildLists(true);
    }

    void PresetBrowserOverlay::selectRow(int row)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;
        selectedRow_ = row;
        scrollSelectedRowIntoView();
        syncActionButtons();
        repaint();
    }

    void PresetBrowserOverlay::syncActionButtons()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
        {
            favoriteButton_.setButtonText(utf8Symbol("\xe2\x99\xa5") + " FAVORITE");
            return;
        }

        const auto& entry = visibleEntries_.getReference(selectedRow_);
        const bool favorited = favoritesStore_.isFavorite(entry.absolutePath);
        favoriteButton_.setButtonText(favorited ? utf8Symbol("\xe2\x99\xa5") + " FAV'D"
                                              : utf8Symbol("\xe2\x99\xa5") + " FAVORITE");
    }

    void PresetBrowserOverlay::toggleFavoriteAtRow(int row)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;
        favoritesStore_.toggleFavorite(visibleEntries_.getReference(row).absolutePath);
        rebuildLists(false);
        syncActionButtons();
    }

    void PresetBrowserOverlay::setRatingAtRow(int row, int stars)
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()))
            return;

        const auto& entry = visibleEntries_.getReference(row);
        const int current = ratingsStore_.ratingForPath(entry.absolutePath);
        const int next = (stars == current) ? 0 : juce::jlimit(1, 5, stars);
        ratingsStore_.setRating(entry.absolutePath, next);
        repaint();
    }

    void PresetBrowserOverlay::selectBank(PresetBank bank)
    {
        if (selectedBank_ == bank)
            return;
        selectedBank_ = bank;
        selectedCategoryKey_ = kAllCategoryKey;
        selectedRow_ = -1;
        rebuildLists(true);
    }

    void PresetBrowserOverlay::loadSelected()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
            return;
        restoreCompareIfNeeded();
        processor_.loadPatchFromFile(visibleEntries_.getReference(selectedRow_).absolutePath);
        dismiss();
    }

    void PresetBrowserOverlay::toggleFavoriteSelected()
    {
        toggleFavoriteAtRow(selectedRow_);
    }

    void PresetBrowserOverlay::restoreCompareIfNeeded()
    {
        if (!compareActive_ || !compareStashPatch_.has_value())
            return;

        processor_.loadPatch(*compareStashPatch_);
        compareActive_ = false;
        compareStashPatch_.reset();
        compareButton_.setButtonText("A/B TEST");
    }

    void PresetBrowserOverlay::toggleCompare()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
            return;

        if (!compareActive_)
        {
            processor_.syncCurrentPatchFromApvts();
            compareStashPatch_ = processor_.getCurrentPatch();
            processor_.loadPatchFromFile(visibleEntries_.getReference(selectedRow_).absolutePath);
            compareActive_ = true;
            compareButton_.setButtonText("RESTORE A");
        }
        else
        {
            restoreCompareIfNeeded();
        }

        repaint();
    }

    void PresetBrowserOverlay::exportSelected()
    {
        if (!juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
            return;

        const auto& entry = visibleEntries_.getReference(selectedRow_);
        juce::File source(entry.absolutePath);
        if (!source.existsAsFile())
            return;

        auto userDir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                           .getChildFile("MURMUR")
                           .getChildFile("Presets")
                           .getChildFile("user");
        userDir.createDirectory();

        const auto safeName = entry.name.trim().replaceCharacter('/', '-');
        auto dest = userDir.getNonexistentChildFile(safeName, ".pw8");
        if (!source.copyFileTo(dest))
            return;

        juce::SystemClipboard::copyTextToClipboard(dest.getFullPathName());
    }

    void PresetBrowserOverlay::stepSelection(int direction)
    {
        if (visibleEntries_.isEmpty())
            return;

        if (selectedRow_ < 0)
            selectedRow_ = 0;
        else
            selectedRow_ = juce::jlimit(0, visibleEntries_.size() - 1, selectedRow_ + direction);

        scrollSelectedRowIntoView();
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

    juce::String PresetBrowserOverlay::emptyStateMessage() const
    {
        if (selectedBank_ == PresetBank::User)
            return "No user presets yet.\nSave patches to ~/Library/Application Support/MURMUR/Presets/user/";
        if (selectedBank_ == PresetBank::Community)
            return "Community bank coming soon.\nFactory presets are under FACTORY.";
        if (selectedCategoryKey_ == kFavoritesCategoryKey)
            return "No favorites yet.\nClick " + utf8Symbol("\xe2\x99\xa1") + " on a preset to heart it here.";
        if (browseFilter().isNarrowed())
            return "No presets match these filters.\nClear MOOD / CONTEXT / TAG chips or search.";
        return "No presets in this bank.";
    }

    void PresetBrowserOverlay::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundTop);
        paintModalPanel(g);
    }

    void PresetBrowserOverlay::paintModalPanel(juce::Graphics& g) const
    {
        g.setColour(palette::kPanelRaised.darker(0.2f));
        g.fillRect(modalBounds_);
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawRect(modalBounds_, 1);
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

    void PresetBrowserOverlay::paintPresetNameCell(juce::Graphics& g, juce::Rectangle<int> bounds,
                                                   const juce::String& name, bool selected) const
    {
        g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
        g.setFont(fonts::label(selected ? 10.0f : 9.0f));
        g.drawText(name.toUpperCase(), bounds, juce::Justification::centredLeft, true);
    }

    void PresetBrowserOverlay::paintHeartCell(juce::Graphics& g, juce::Rectangle<int> bounds, bool favorited,
                                              bool selected) const
    {
        juce::ignoreUnused(selected);
        g.setFont(fonts::title(11.0f));
        g.setColour(favorited ? juce::Colour(0xffff5a7a) : palette::kTextDim.withAlpha(0.45f));
        g.drawText(favorited ? utf8Symbol("\xe2\x99\xa5") : utf8Symbol("\xe2\x99\xa1"), bounds,
                   juce::Justification::centred, false);
    }

    void PresetBrowserOverlay::paintPresetRatingStars(juce::Graphics& g, juce::Rectangle<int> bounds, int rating,
                                                      bool selected) const
    {
        juce::ignoreUnused(selected);
        auto starRow = bounds.withSizeKeepingCentre(kExplorerStarCellWidth * 5, bounds.getHeight());
        g.setFont(fonts::label(9.0f));
        for (int star = 1; star <= 5; ++star)
        {
            auto cell = starRow.removeFromLeft(kExplorerStarCellWidth);
            const bool filled = star <= rating;
            g.setColour(filled ? palette::kAccentWarm : palette::kTextDim.withAlpha(0.35f));
            g.drawText(filled ? utf8Symbol("\xe2\x98\x85") : utf8Symbol("\xe2\x98\x86"), cell,
                       juce::Justification::centred, false);
        }
    }

    void PresetBrowserOverlay::paintTopBar(juce::Graphics& g) const
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(topBarBounds_.getBottom(), static_cast<float>(modalBounds_.getX()),
                             static_cast<float>(modalBounds_.getRight()));

        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::title(11.0f));
        g.drawText("PRESET BROWSER", topBarBounds_.reduced(16, 0), juce::Justification::centredLeft, true);

        const int searchX = modalBounds_.getCentreX() - kPresetBrowserSearchBarWidth / 2;
        const auto searchBounds =
            juce::Rectangle<int>(searchX, topBarBounds_.getY() + 16, kPresetBrowserSearchBarWidth,
                                 kPresetBrowserSearchFieldHeight);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(searchBounds.toFloat(), 4.0f);
        g.setColour(palette::kBorder.withAlpha(0.45f));
        g.drawRoundedRectangle(searchBounds.toFloat().reduced(0.5f), 4.0f, 1.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("⌕", searchBounds.withWidth(24), juce::Justification::centred, false);

        if (searchField_.getText().isNotEmpty())
        {
            g.setColour(palette::kTextDim);
            g.drawRoundedRectangle(searchClearBounds_.toFloat(), 3.0f, 1.0f);
            g.drawText("✕", searchClearBounds_, juce::Justification::centred, false);
        }

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
        const int rowStride = kPresetBrowserCategoryRowHeight + 1;
        const int totalHeight = categoryKeys_.size() * rowStride;
        const int maxScroll = juce::jmax(0, totalHeight - categoryListBounds_.getHeight());
        const int listScrollY = juce::jlimit(0, maxScroll, categoryListScrollY_);

        g.saveState();
        g.reduceClipRegion(categoryListBounds_);
        rowArea = rowArea.withY(rowArea.getY() - listScrollY);

        for (int i = 0; i < categoryKeys_.size(); ++i)
        {
            auto row = rowArea.removeFromTop(kPresetBrowserCategoryRowHeight);
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
        g.restoreState();
    }

    void PresetBrowserOverlay::paintCenterColumn(juce::Graphics& g) const
    {
        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawVerticalLine(centerColumnBounds_.getRight(), static_cast<float>(workspaceBounds_.getY()),
                           static_cast<float>(workspaceBounds_.getBottom()));

        auto header = presetTableBounds_.withHeight(kPresetBrowserTableHeaderHeight);
        const auto cols = PresetTableColumns::forTable(presetTableBounds_);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(utf8Symbol("\xe2\x99\xa5"), cols.heartX, header.getY(), kExplorerHeartColWidth, 10,
                   juce::Justification::centred);
        g.drawText("RATING", cols.ratingX, header.getY(), kExplorerRatingColWidth, 10, juce::Justification::centredLeft);
        g.drawText("PRESET NAME", cols.nameX, header.getY(), kExplorerNameColWidth, 10,
                   juce::Justification::centredLeft);
        g.drawText("AUTHOR", cols.authorX, header.getY(), kExplorerAuthorColWidth, 10,
                   juce::Justification::centredLeft);
        g.drawText("DATE", cols.dateX, header.getY(), kExplorerDateColWidth, 10, juce::Justification::centredLeft);

        g.setColour(palette::kBorder.withAlpha(0.35f));
        g.drawHorizontalLine(presetTableBounds_.getY(), static_cast<float>(presetTableBounds_.getX()),
                             static_cast<float>(presetTableBounds_.getRight()));

        if (visibleEntries_.isEmpty())
        {
            g.setColour(palette::kTextDim);
            g.setFont(fonts::label(10.0f));
            g.drawFittedText(emptyStateMessage(), presetTableBounds_.reduced(24), juce::Justification::centred, 4);
            return;
        }

        auto table = presetTableBounds_;
        const int rowStride = kPresetBrowserPresetRowHeight + 2;
        const int totalHeight = visibleEntries_.size() * rowStride;
        const int maxScroll = juce::jmax(0, totalHeight - table.getHeight());
        const int listScrollY = juce::jlimit(0, maxScroll, presetListScrollY_);

        g.saveState();
        g.reduceClipRegion(table);
        table = table.withY(table.getY() - listScrollY);

        for (int row = 0; row < visibleEntries_.size(); ++row)
        {
            auto rowBounds = table.removeFromTop(kPresetBrowserPresetRowHeight);
            table.removeFromTop(2);
            const bool selected = row == selectedRow_;
            if (selected)
            {
                g.setColour(palette::kAccent.withAlpha(0.12f));
                g.fillRoundedRectangle(rowBounds.toFloat(), 4.0f);
                draw::strokeGlowPath(g, draw::roundedRectPath(rowBounds.toFloat(), 4.0f), 0.8f, 1.0f, true);
            }

            const auto& entry = visibleEntries_.getReference(row);
            const bool favorited = favoritesStore_.isFavorite(entry.absolutePath);
            const int rating = ratingsStore_.ratingForPath(entry.absolutePath);

            paintHeartCell(g, juce::Rectangle<int>(cols.heartX, rowBounds.getY(), kExplorerHeartColWidth,
                                                   rowBounds.getHeight()),
                           favorited, selected);
            paintPresetRatingStars(g, juce::Rectangle<int>(cols.ratingX, rowBounds.getY(), kExplorerRatingColWidth,
                                                           rowBounds.getHeight()),
                                   rating, selected);

            g.setColour(selected ? palette::kTextPrimary : palette::kTextSecondary);
            paintPresetNameCell(g, juce::Rectangle<int>(cols.nameX, rowBounds.getY(), kExplorerNameColWidth,
                                                        rowBounds.getHeight()),
                                entry.name, selected);
            g.drawText(formatAuthorForEntry(entry), cols.authorX, rowBounds.getY(), kExplorerAuthorColWidth,
                       rowBounds.getHeight(), juce::Justification::centredLeft, true);
            g.setColour(palette::kTextDim);
            g.drawText(formatDateForEntry(entry), cols.dateX, rowBounds.getY(), kExplorerDateColWidth,
                       rowBounds.getHeight(), juce::Justification::centredLeft, true);
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

        const int rating = ratingsStore_.ratingForPath(entry.absolutePath);
        auto ratingRow = area.removeFromTop(16);
        paintPresetRatingStars(g, ratingRow.withWidth(kExplorerRatingColWidth), rating, true);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(rating > 0 ? juce::String(rating) + ".0" : "UNRATED", ratingRow.withTrimmedLeft(78),
                   juce::Justification::centredLeft, true);
        area.removeFromTop(8);

        auto waveBox = area.removeFromTop(kPresetBrowserDetailWaveformHeight);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(waveBox.toFloat(), 6.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText("PREVIEW", waveBox.reduced(8).removeFromTop(10), juce::Justification::centredLeft, true);
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);
        g.drawFittedText("Waveform preview coming soon.\nLoad the preset to hear it.",
                         waveBox.reduced(8).withTrimmedTop(14), juce::Justification::centred, 3);

        area.removeFromTop(12);
        g.setColour(palette::kTextDim);
        g.drawText("ENGINES", area.removeFromTop(10), juce::Justification::centredLeft, true);
        area.removeFromTop(4);
        g.setFont(fonts::label(9.0f));
        g.setColour(palette::kTextSecondary);
        g.drawText("Active: " + entry.enginesSummary.toUpperCase(), area.removeFromTop(16),
                   juce::Justification::centredLeft, true);

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
        g.drawText("◀ PREV PRESET", paginationPrevBounds_, juce::Justification::centred, true);
        g.drawText("NEXT PRESET ▶", paginationNextBounds_, juce::Justification::centred, true);
    }

    void PresetBrowserOverlay::resized()
    {
        modalBounds_ = pageBounds(getLocalBounds());
        topBarBounds_ = modalBounds_.removeFromTop(kChromeBarHeightDesign);
        modalBounds_.removeFromTop(kPresetBrowserPageColumnGap);
        bottomBarBounds_ = modalBounds_.removeFromBottom(kPresetBrowserFooterHeight);
        modalBounds_.removeFromBottom(kPresetBrowserPageColumnGap);

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
        if (facetStripHeight > 0)
            modalBounds_.removeFromTop(kPresetBrowserPageColumnGap);
        workspaceBounds_ = modalBounds_;

        leftColumnBounds_ = workspaceBounds_.removeFromLeft(kPresetBrowserLeftColumnWidth);
        workspaceBounds_.removeFromLeft(kPresetBrowserPageColumnGap);
        centerColumnBounds_ = workspaceBounds_.removeFromLeft(kPresetBrowserCenterColumnWidth);
        workspaceBounds_.removeFromLeft(kPresetBrowserPageColumnGap);
        rightColumnBounds_ = workspaceBounds_;

        closeButtonBounds_ = topBarBounds_.removeFromRight(40).withSizeKeepingCentre(24, 24);

        const int searchX = topBarBounds_.getCentreX() - kPresetBrowserSearchBarWidth / 2;
        searchField_.setBounds(searchX, topBarBounds_.getY() + 16, kPresetBrowserSearchBarWidth,
                               kPresetBrowserSearchFieldHeight);
        searchClearBounds_ =
            juce::Rectangle<int>(searchField_.getRight() - 22, searchField_.getY() + 4, 16, 16);
        searchIconBounds_ = searchField_.getBounds().removeFromLeft(24);

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
        centerInner.removeFromTop(kPresetBrowserTableHeaderHeight + 8);
        presetTableBounds_ = centerInner;

        auto actions = rightColumnBounds_.reduced(12);
        actions.removeFromTop(310);
        loadButton_.setBounds(actions.removeFromTop(kPresetBrowserLoadButtonHeight));
        actions.removeFromTop(4);
        auto btnRow = actions.removeFromTop(kPresetBrowserActionButtonHeight);
        favoriteButton_.setBounds(btnRow.removeFromLeft((btnRow.getWidth() - 6) / 2));
        btnRow.removeFromLeft(6);
        compareButton_.setBounds(btnRow);
        actions.removeFromTop(4);
        exportButton_.setBounds(actions.removeFromTop(kPresetBrowserActionButtonHeight));

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
        const int relativeY = pos.y - categoryListBounds_.getY() + categoryListScrollY_;
        if (relativeY < 0)
            return -1;
        return relativeY / (kPresetBrowserCategoryRowHeight + 1);
    }

    int PresetBrowserOverlay::presetRowAt(juce::Point<int> pos) const
    {
        if (!presetTableBounds_.contains(pos))
            return -1;
        const int relativeY = pos.y - presetTableBounds_.getY() + presetListScrollY_;
        if (relativeY < 0)
            return -1;
        return relativeY / (kPresetBrowserPresetRowHeight + 2);
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
        const int delta = juce::roundToInt(wheel.deltaY * 48.0f);

        if (categoryListBounds_.contains(event.getPosition()))
        {
            const int rowStride = kPresetBrowserCategoryRowHeight + 1;
            const int totalHeight = categoryKeys_.size() * rowStride;
            const int maxScroll = juce::jmax(0, totalHeight - categoryListBounds_.getHeight());
            categoryListScrollY_ = juce::jlimit(0, maxScroll, categoryListScrollY_ - delta);
            repaint();
            return;
        }

        if (!presetTableBounds_.contains(event.getPosition()))
            return;

        const int rowStride = kPresetBrowserPresetRowHeight + 2;
        const int totalHeight = visibleEntries_.size() * rowStride;
        const int maxScroll = juce::jmax(0, totalHeight - presetTableBounds_.getHeight());
        presetListScrollY_ = juce::jlimit(0, maxScroll, presetListScrollY_ - delta);
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

        if (searchClearBounds_.contains(event.getPosition()))
        {
            clearSearch();
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
            if (heartCellAt(event.getPosition()))
            {
                toggleFavoriteAtRow(presetRow);
                return;
            }

            const int star = ratingStarAt(event.getPosition(), presetRow);
            if (star > 0)
            {
                setRatingAtRow(presetRow, star);
                return;
            }

            if (event.getNumberOfClicks() >= 2)
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

    bool PresetBrowserOverlay::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
    {
        if (!isVisible())
            return false;

        if (originatingComponent == &searchField_)
        {
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
            if (key == juce::KeyPress::escapeKey && searchField_.getText().isNotEmpty())
            {
                clearSearch();
                return true;
            }
        }

        return keyPressed(key);
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

        if (key == juce::KeyPress::homeKey)
        {
            if (!visibleEntries_.isEmpty())
                selectRow(0);
            return true;
        }

        if (key == juce::KeyPress::endKey)
        {
            if (!visibleEntries_.isEmpty())
                selectRow(visibleEntries_.size() - 1);
            return true;
        }

        if (key == juce::KeyPress::pageUpKey)
        {
            const int pageRows = juce::jmax(1, presetTableBounds_.getHeight() / (kPresetBrowserPresetRowHeight + 2));
            stepSelection(-pageRows);
            return true;
        }

        if (key == juce::KeyPress::pageDownKey)
        {
            const int pageRows = juce::jmax(1, presetTableBounds_.getHeight() / (kPresetBrowserPresetRowHeight + 2));
            stepSelection(pageRows);
            return true;
        }

        if (key == juce::KeyPress('f', juce::ModifierKeys::noModifiers, 0))
        {
            toggleFavoriteSelected();
            return true;
        }

        if (key == juce::KeyPress('/', juce::ModifierKeys::noModifiers, 0)
            || key == juce::KeyPress('f', juce::ModifierKeys::commandModifier, 0)
            || key == juce::KeyPress('f', juce::ModifierKeys::ctrlModifier, 0))
        {
            searchField_.grabKeyboardFocus();
            return true;
        }

        return false;
    }

    void PresetBrowserOverlay::textEditorTextChanged(juce::TextEditor&)
    {
        const auto query = searchField_.getText();
        const bool queryChanged = query != lastSearchQuery_;
        lastSearchQuery_ = query;

        juce::String keepPath;
        if (juce::isPositiveAndBelow(selectedRow_, visibleEntries_.size()))
            keepPath = visibleEntries_.getReference(selectedRow_).absolutePath;

        rebuildLists(queryChanged);

        if (keepPath.isNotEmpty())
        {
            for (int i = 0; i < visibleEntries_.size(); ++i)
            {
                if (visibleEntries_.getReference(i).absolutePath == keepPath)
                {
                    selectedRow_ = i;
                    scrollSelectedRowIntoView();
                    repaint();
                    return;
                }
            }
        }

        syncSelectionToCurrentPreset();
        repaint();
    }

    void PresetBrowserOverlay::textEditorReturnKeyPressed(juce::TextEditor&) { loadSelected(); }

    void PresetBrowserOverlay::textEditorEscapeKeyPressed(juce::TextEditor&) { dismiss(); }

    bool PresetBrowserOverlay::heartCellAt(juce::Point<int> pos) const
    {
        if (!presetTableBounds_.contains(pos))
            return false;

        const auto cols = PresetTableColumns::forTable(presetTableBounds_);
        const int rowStride = kPresetBrowserPresetRowHeight + 2;
        const int relativeY = pos.y - presetTableBounds_.getY() + presetListScrollY_;
        if (relativeY < 0)
            return false;
        const int rowTop = (relativeY / rowStride) * rowStride;
        juce::Rectangle<int> heartBounds(cols.heartX, presetTableBounds_.getY() + rowTop - presetListScrollY_,
                                         kExplorerHeartColWidth, kPresetBrowserPresetRowHeight);
        return heartBounds.contains(pos);
    }

    int PresetBrowserOverlay::ratingStarAt(juce::Point<int> pos, int row) const
    {
        if (!juce::isPositiveAndBelow(row, visibleEntries_.size()) || !presetTableBounds_.contains(pos))
            return 0;

        const auto cols = PresetTableColumns::forTable(presetTableBounds_);
        const int rowStride = kPresetBrowserPresetRowHeight + 2;
        const int rowTop = presetTableBounds_.getY() + row * rowStride - presetListScrollY_;
        const juce::Rectangle<int> ratingBounds(cols.ratingX, rowTop, kExplorerRatingColWidth,
                                                kPresetBrowserPresetRowHeight);
        if (!ratingBounds.contains(pos))
            return 0;

        const int localX = pos.x - ratingBounds.getX();
        return juce::jlimit(1, 5, localX / kExplorerStarCellWidth + 1);
    }

} // namespace pw8::plugin::ui
