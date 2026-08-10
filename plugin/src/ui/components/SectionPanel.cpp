#include "SectionPanel.h"

#include "../theme/ObsidianPalette.h"

namespace pw8::plugin::ui
{
    namespace
    {
        constexpr int kTitleHeight = 22;
        constexpr int kPadding = 8;
    } // namespace

    SectionPanel::SectionPanel(const juce::String& title) : title_(title.toUpperCase()) {}

    void SectionPanel::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(palette::kBorder);
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        if (title_.isNotEmpty())
        {
            g.setColour(palette::kTextDim);
            g.setFont(juce::Font(juce::FontOptions(11.0f)).withExtraKerningFactor(0.08f));
            g.drawText(title_, bounds.removeFromTop(static_cast<float>(kTitleHeight)).reduced(kPadding, 0),
                       juce::Justification::centredLeft);
        }
    }

    void SectionPanel::resized() {}

    juce::Rectangle<int> SectionPanel::getContentBounds() const
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(kTitleHeight);
        return bounds.reduced(kPadding);
    }

} // namespace pw8::plugin::ui
