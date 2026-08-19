#pragma once

#include <functional>
#include <optional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/FavoritesStore.h"
#include "content/PresetRatingsStore.h"
#include "content/PresetIndex.h"
#include "MetadataFacetRow.h"
#include "processor/PatchworkEightProcessor.h"
#include "pw8/patch/Patch.hpp"

namespace pw8::plugin::ui
{
    /// Figma `murmur-preset-explorer-overlay` (74:959) — centered modal popup over dimmed host UI.
    class PresetBrowserOverlay : public juce::Component,
                                 private juce::TextEditor::Listener,
                                 private juce::KeyListener
    {
    public:
        PresetBrowserOverlay(PatchworkEightProcessor& processor, content::PresetIndex& presetIndex,
                             content::FavoritesStore& favoritesStore,
                             content::PresetRatingsStore& ratingsStore);

        std::function<void()> onClosed;
        std::function<void()> onFiltersChanged;

        void showOverlay();
        void dismiss();

        [[nodiscard]] content::PresetMetadataFilter browseFilter() const;

        void paint(juce::Graphics& g) override;
        void paintOverChildren(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
        bool keyPressed(const juce::KeyPress& key) override;

    private:
        enum class PresetBank : std::uint8_t
        {
            Factory = 0,
            User,
            Community
        };

        static constexpr const char* kAllCategoryKey = "__all__";
        static constexpr const char* kFavoritesCategoryKey = "__favorites__";

        void textEditorTextChanged(juce::TextEditor&) override;
        void textEditorReturnKeyPressed(juce::TextEditor&) override;
        void textEditorEscapeKeyPressed(juce::TextEditor&) override;

        void rebuildLists(bool resetPresetScroll);
        void rebuildFacetRows();
        void selectCategory(const juce::String& categoryKey);
        void selectMetadataFacet(content::PresetFacet facet, const juce::String& value);
        void selectRow(int row);
        void selectBank(PresetBank bank);
        void loadSelected();
        void toggleFavoriteSelected();
        void toggleFavoriteAtRow(int row);
        void setRatingAtRow(int row, int stars);
        void syncActionButtons();
        void toggleCompare();
        void restoreCompareIfNeeded();
        void exportSelected();
        void stepSelection(int direction);
        void syncSelectionToCurrentPreset();
        void scrollSelectedRowIntoView();
        void clearSearch();

        [[nodiscard]] bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

        [[nodiscard]] bool entryMatchesBank(const content::PresetEntry& entry) const;
        [[nodiscard]] int categoryRowAt(juce::Point<int> pos) const;
        [[nodiscard]] int presetRowAt(juce::Point<int> pos) const;
        [[nodiscard]] int bankPillAt(juce::Point<int> pos) const;
        [[nodiscard]] juce::String formatAuthorForEntry(const content::PresetEntry& entry) const;
        [[nodiscard]] juce::String formatDateForEntry(const content::PresetEntry& entry) const;
        [[nodiscard]] content::PresetMetadataFilter browseFilterWithoutFacet(content::PresetFacet facet) const;
        [[nodiscard]] content::PresetIndex::EntryPredicate bankEntryPredicate() const;
        [[nodiscard]] int tagPillAt(juce::Point<int> pos) const;
        [[nodiscard]] juce::String emptyStateMessage() const;

        void paintModalPanel(juce::Graphics& g) const;
        void paintTopBar(juce::Graphics& g) const;
        void paintLeftColumn(juce::Graphics& g) const;
        void paintCenterColumn(juce::Graphics& g) const;
        void paintPresetNameCell(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& name,
                                 bool selected) const;
        void paintRightColumn(juce::Graphics& g) const;
        void paintBottomBar(juce::Graphics& g) const;

        void paintPresetRatingStars(juce::Graphics& g, juce::Rectangle<int> bounds, int rating,
                                    bool selected) const;
        void paintHeartCell(juce::Graphics& g, juce::Rectangle<int> bounds, bool favorited,
                            bool selected) const;
        [[nodiscard]] int ratingStarAt(juce::Point<int> pos, int row) const;
        [[nodiscard]] bool heartCellAt(juce::Point<int> pos) const;

        PatchworkEightProcessor& processor_;
        content::PresetIndex& presetIndex_;
        content::FavoritesStore& favoritesStore_;
        content::PresetRatingsStore& ratingsStore_;

        juce::TextEditor searchField_;
        juce::TextButton loadButton_{"LOAD PRESET"};
        juce::TextButton favoriteButton_{"FAVORITE"};
        juce::TextButton compareButton_{"A/B TEST"};
        juce::TextButton exportButton_{"EXPORT"};
        std::unique_ptr<MetadataFacetRow> moodFacetRow_;
        std::unique_ptr<MetadataFacetRow> contextFacetRow_;
        std::unique_ptr<MetadataFacetRow> tagFacetRow_;

        juce::StringArray categoryKeys_;
        juce::StringArray categoryLabels_;
        juce::Array<int> categoryCounts_;
        juce::Array<content::PresetEntry> visibleEntries_;
        juce::String selectedCategoryKey_;
        PresetBank selectedBank_ = PresetBank::Factory;
        int selectedRow_ = -1;
        int presetListScrollY_ = 0;
        int categoryListScrollY_ = 0;

        struct TagPillHit
        {
            juce::String value;
            content::PresetFacet facet = content::PresetFacet::Tag;
            juce::Rectangle<int> bounds;
        };

        mutable juce::Array<TagPillHit> tagPillHits_;

        juce::Rectangle<int> modalBounds_;
        juce::Rectangle<int> topBarBounds_;
        juce::Rectangle<int> facetStripBounds_;
        juce::Rectangle<int> workspaceBounds_;
        juce::Rectangle<int> leftColumnBounds_;
        juce::Rectangle<int> centerColumnBounds_;
        juce::Rectangle<int> rightColumnBounds_;
        juce::Rectangle<int> bottomBarBounds_;
        juce::Rectangle<int> bankPillsBounds_;
        juce::Rectangle<int> categoryListBounds_;
        juce::Rectangle<int> presetTableBounds_;
        juce::String lastSearchQuery_;
        juce::Rectangle<int> searchClearBounds_;
        juce::Rectangle<int> searchIconBounds_;
        juce::Rectangle<int> closeButtonBounds_;
        juce::Rectangle<int> paginationPrevBounds_;
        juce::Rectangle<int> paginationNextBounds_;

        bool compareActive_ = false;
        std::optional<patch::Patch> compareStashPatch_;
    };

} // namespace pw8::plugin::ui
