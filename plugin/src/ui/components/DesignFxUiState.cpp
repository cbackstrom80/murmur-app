#include "DesignFxUiState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] juce::File designFxUiPrefsFile()
        {
            return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                .getChildFile("Patchwork Eight")
                .getChildFile("design-fx-ui.json");
        }
    } // namespace

    void DesignFxUiState::saveUserPreferences() const
    {
        juce::DynamicObject::Ptr root = new juce::DynamicObject();
        juce::Array<juce::var> order;
        for (std::size_t i = 0; i < kChipCount; ++i)
            order.add(static_cast<int>(chipDisplayOrder_[i]));
        root->setProperty("chipOrder", order);
        root->setProperty("eqAnalyzer", eqAnalyzerActive_);
        root->setProperty("limTruePeak", limTruePeakActive_);
        root->setProperty("moodPill", moodPill_);

        juce::Array<juce::var> knobRows;
        for (std::size_t chip = 0; chip < byChip_.size(); ++chip)
        {
            juce::Array<juce::var> row;
            for (std::size_t knob = 0; knob < kKnobsPerChip; ++knob)
                row.add(byChip_[chip][knob]);
            knobRows.add(row);
        }
        root->setProperty("knobValues", knobRows);

        const juce::File file = designFxUiPrefsFile();
        file.getParentDirectory().createDirectory();
        file.replaceWithText(juce::JSON::toString(juce::var(root.get()), true));
    }

    void DesignFxUiState::loadUserPreferences()
    {
        const juce::File file = designFxUiPrefsFile();
        if (!file.existsAsFile())
            return;

        const auto parsed = juce::JSON::parse(file);
        if (!parsed.isObject())
            return;

        const auto order = parsed.getProperty("chipOrder", juce::var());
        if (order.isArray())
        {
            std::array<bool, kChipCount> seen{};
            std::array<std::size_t, kChipCount> next{};
            std::size_t count = 0;
            for (int i = 0; i < order.size() && count < kChipCount; ++i)
            {
                const auto chip = static_cast<std::size_t>(static_cast<int>(order[i]));
                if (chip >= kChipCount || seen[chip])
                    continue;
                seen[chip] = true;
                next[count++] = chip;
            }
            for (std::size_t chip = 0; chip < kChipCount; ++chip)
            {
                if (!seen[chip])
                    next[count++] = chip;
            }
            if (count == kChipCount)
                chipDisplayOrder_ = next;
        }

        eqAnalyzerActive_ = static_cast<bool>(parsed.getProperty("eqAnalyzer", eqAnalyzerActive_));
        limTruePeakActive_ = static_cast<bool>(parsed.getProperty("limTruePeak", limTruePeakActive_));
        const juce::String pill = parsed.getProperty("moodPill", moodPill_).toString();
        if (pill.isNotEmpty())
            moodPill_ = pill;

        const auto knobRows = parsed.getProperty("knobValues", juce::var());
        if (knobRows.isArray())
        {
            for (int chip = 0; chip < knobRows.size() && chip < static_cast<int>(byChip_.size()); ++chip)
            {
                const auto row = knobRows[chip];
                if (!row.isArray())
                    continue;
                for (int knob = 0; knob < row.size() && knob < static_cast<int>(kKnobsPerChip); ++knob)
                    byChip_[static_cast<std::size_t>(chip)][static_cast<std::size_t>(knob)] =
                        juce::jlimit(0.0f, 1.0f, static_cast<float>(row[knob]));
            }
        }
    }

} // namespace pw8::plugin::ui
