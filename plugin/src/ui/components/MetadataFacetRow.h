#pragma once

#include <functional>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

// One metadata facet row: label + horizontal chip strip (All + values).
namespace pw8::plugin::ui
{
    class MetadataFacetRow : public juce::Component
    {
    public:
        explicit MetadataFacetRow(const juce::String& rowLabel);

        void setValues(const juce::StringArray& values);
        void setSelectedValue(const juce::String& value);
        [[nodiscard]] juce::String getSelectedValue() const { return selectedValue_; }
        [[nodiscard]] bool hasFacetValues() const noexcept { return !values_.isEmpty(); }

        std::function<void()> onChange;

        void resized() override;

    private:
        void rebuildChips();
        void selectValue(const juce::String& value, bool notify);

        juce::Label label_;
        juce::Viewport viewport_;
        juce::Component chipHost_;
        juce::String rowLabel_;
        juce::StringArray values_;
        juce::String selectedValue_;
        std::vector<std::unique_ptr<juce::TextButton>> chips_;
    };

} // namespace pw8::plugin::ui
