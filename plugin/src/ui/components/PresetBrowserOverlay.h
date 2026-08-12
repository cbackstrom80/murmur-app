#pragma once

#include <functional>

#include <juce_gui_basics/juce_gui_basics.h>

#include "content/PresetIndex.h"
#include "processor/PatchworkEightProcessor.h"

// Overlay preset browser: search, category filter, scrollable list (docs/PATCH_BROWSER.md phase 3).
namespace pw8::plugin::ui
{
    class PresetBrowserOverlay : public juce::Component, private juce::TextEditor::Listener, private juce::ListBoxModel
    {
    public:
        PresetBrowserOverlay(PatchworkEightProcessor& processor, content::PresetIndex& presetIndex);

        std::function<void()> onClosed;

        void showOverlay();
        void dismiss();

        [[nodiscard]] juce::String browseQuery() const { return searchField_.getText(); }
        [[nodiscard]] juce::String browseCategory() const;

        void paint(juce::Graphics& g) override;
        void resized() override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        void textEditorTextChanged(juce::TextEditor&) override;
        void textEditorReturnKeyPressed(juce::TextEditor&) override;
        void textEditorEscapeKeyPressed(juce::TextEditor&) override;

        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent&) override;

        void rebuildList();
        void loadRow(int row);

        PatchworkEightProcessor& processor_;
        content::PresetIndex& presetIndex_;

        juce::Component panel_;
        juce::Label titleLabel_{"", "PRESETS"};
        juce::TextEditor searchField_;
        juce::ComboBox categoryBox_;
        juce::ListBox listBox_{"Presets", this};
        juce::TextButton closeButton_{"CLOSE"};

        juce::Array<content::PresetEntry> visibleEntries_;
    };

} // namespace pw8::plugin::ui
