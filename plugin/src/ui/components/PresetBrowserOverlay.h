#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/FavoritesStore.h"
#include "content/PresetIndex.h"
#include "MetadataFacetRow.h"
#include "processor/PatchworkEightProcessor.h"

namespace pw8::plugin::ui
{
    /// Figma `murmur-preset-explorer-overlay` (74:959) — centered modal popup over dimmed host UI.
    class PresetBrowserOverlay : public juce::Component, private juce::TextEditor::Listener, private juce::Timer
    {
    public:
        PresetBrowserOverlay(PatchworkEightProcessor& processor, content::PresetIndex& presetIndex,
                             content::FavoritesStore& favoritesStore);

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

        static constexpr const char* kFavoritesCategoryKey = "__favorites__";

        void textEditorTextChanged(juce::TextEditor&) override;
        void textEditorReturnKeyPressed(juce::TextEditor&) override;
        void textEditorEscapeKeyPressed(juce::TextEditor&) override;
        void timerCallback() override;

        void rebuildLists();
        void rebuildFacetRows();
        void selectCategory(const juce::String& categoryKey);
        void selectMetadataFacet(content::PresetFacet facet, const juce::String& value);
        void selectRow(int row);
        void selectBank(PresetBank bank);
        void loadSelected();
        void toggleFavoriteSelected();
        void stepSelection(int direction);

        [[nodiscard]] bool entryMatchesBank(const content::PresetEntry& entry) const;
        [[nodiscard]] int categoryRowAt(juce::Point<int> pos) const;
        [[nodiscard]] int presetRowAt(juce::Point<int> pos) const;
        [[nodiscard]] int bankPillAt(juce::Point<int> pos) const;
        [[nodiscard]] juce::String formatAuthorForEntry(const content::PresetEntry& entry) const;
        [[nodiscard]] juce::String formatDateForEntry(const content::PresetEntry& entry) const;
        [[nodiscard]] int ratingForEntry(const content::PresetEntry& entry) const;
        [[nodiscard]] juce::Array<int> activeEnginesForEntry(const content::PresetEntry& entry) const;
        [[nodiscard]] content::PresetMetadataFilter browseFilterWithoutFacet(content::PresetFacet facet) const;
        [[nodiscard]] juce::StringArray uniqueFacetValuesForBank(content::PresetFacet facet) const;
        [[nodiscard]] int tagPillAt(juce::Point<int> pos) const;

        void paintModalPanel(juce::Graphics& g) const;
        void paintTopBar(juce::Graphics& g) const;
        void paintLeftColumn(juce::Graphics& g) const;
        void paintCenterColumn(juce::Graphics& g) const;
        void paintRightColumn(juce::Graphics& g) const;
        void paintBottomBar(juce::Graphics& g) const;
        void paintStarRating(juce::Graphics& g, juce::Rectangle<int> area, int filledStars, bool large) const;

        PatchworkEightProcessor& processor_;
        content::PresetIndex& presetIndex_;
        content::FavoritesStore& favoritesStore_;

        juce::TextEditor searchField_;
        juce::TextButton loadButton_{"LOAD PRESET"};
        juce::TextButton favoriteButton_{"FAVORITE"};
        juce::TextButton compareButton_{"A/B TEST"};
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
        juce::Rectangle<int> closeButtonBounds_;
        juce::Rectangle<int> paginationPrevBounds_;
        juce::Rectangle<int> paginationNextBounds_;
    };

} // namespace pw8::plugin::ui
