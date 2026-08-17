#include "FxChainFlowView.h"

#include "../../theme/FxTypeGlyphs.h"
#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui::wireframe
{
    namespace
    {
        [[nodiscard]] int readEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        [[nodiscard]] float readMix(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Mix"))
                return raw->load();
            return 1.0f;
        }

        [[nodiscard]] juce::String slotShortLabel(std::size_t index)
        {
            if (index < 3)
                return "I" + juce::String(static_cast<int>(index + 1));
            return "M" + juce::String(static_cast<int>(index - 2));
        }
    } // namespace

    FxChainFlowView::FxChainFlowView(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        startTimerHz(8);
    }

    void FxChainFlowView::setSlotPrefixes(const std::array<juce::String, 7>& prefixes)
    {
        prefixes_ = prefixes;
        timerCallback();
    }

    void FxChainFlowView::setSelectedSlot(std::size_t index)
    {
        selectedSlot_ = juce::jlimit<std::size_t>(0, 6, index);
        repaint();
    }

    void FxChainFlowView::timerCallback()
    {
        bool changed = false;
        const bool postFader =
            apvts_.getRawParameterValue(kFxRoutingPrePostId) != nullptr
            && apvts_.getRawParameterValue(kFxRoutingPrePostId)->load() >= 0.5f;
        if (postFader != insertsPostFader_)
        {
            insertsPostFader_ = postFader;
            changed = true;
        }

        for (std::size_t i = 0; i < prefixes_.size(); ++i)
        {
            const int t = prefixes_[i].isEmpty() ? 0 : readEffectType(apvts_, prefixes_[i]);
            const float m = prefixes_[i].isEmpty() ? 0.0f : readMix(apvts_, prefixes_[i]);
            if (t != effectTypes_[i] || m != mixValues_[i])
            {
                effectTypes_[i] = t;
                mixValues_[i] = m;
                changed = true;
            }
        }
        if (changed)
            repaint();
    }

    void FxChainFlowView::resized() {}

    std::size_t FxChainFlowView::slotIndexAt(juce::Point<int> pos) const
    {
        const auto bounds = getLocalBounds().toFloat();
        const float gap = 6.0f;
        const float labelW = 52.0f;
        const float arrowW = 18.0f;
        const float outW = 28.0f;
        const float slotW =
            (bounds.getWidth() - labelW * 2.0f - arrowW * 6.0f - outW - gap * 11.0f) / 7.0f;

        auto x = bounds.getX() + 6.0f + labelW + gap;
        const float y = bounds.getY();
        const float h = bounds.getHeight();

        for (std::size_t i = 0; i < 7; ++i)
        {
            juce::Rectangle<float> box(x, y + 4.0f, slotW, h - 8.0f);
            if (box.contains(pos.toFloat()))
                return i;
            x += slotW + gap;
            if (i == 2)
                x += labelW + gap; // skip "MASTER" label width
            else if (i < 6)
                x += arrowW + gap;
        }
        return selectedSlot_;
    }

    void FxChainFlowView::mouseDown(const juce::MouseEvent& event)
    {
        const auto idx = slotIndexAt(event.getPosition());
        selectedSlot_ = idx;
        if (onSlotSelected)
            onSlotSelected(idx);
        repaint();
    }

    void FxChainFlowView::paintSlotBox(juce::Graphics& g, juce::Rectangle<float> box, std::size_t slotIndex,
                                       int effectType, float mix) const
    {
        const bool selected = slotIndex == selectedSlot_;
        const bool active = effectType != 0;
        const auto& spec = pw8::plugin::ui::fxPlaySpecForType(effectType);

        g.setColour(selected ? palette::kAccentDim.withAlpha(0.55f) : palette::kPanelRaised);
        g.fillRoundedRectangle(box, 5.0f);
        g.setColour(selected ? palette::kAccent : palette::kBorder.withAlpha(0.75f));
        g.drawRoundedRectangle(box.reduced(0.5f), 5.0f, selected ? 1.8f : 1.0f);
        if (selected)
        {
            g.setColour(palette::kAccent.withAlpha(0.18f));
            g.fillRoundedRectangle(box.reduced(1.5f), 4.0f);
        }

        auto inner = box.reduced(4.0f, 3.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(7.5f));
        g.drawText(slotShortLabel(slotIndex), inner.removeFromTop(10.0f), juce::Justification::centred);

        g.setColour(active ? palette::kAccent.withAlpha(juce::jlimit(0.55f, 1.0f, mix + 0.3f)) : palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        const juce::String abbrev = active ? spec.flowAbbrev : "OFF";
        g.drawText(abbrev, inner.removeFromTop(13.0f), juce::Justification::centred);

        // Figma SEC_02 mini glyph
        auto schematic = inner.reduced(2.0f, 1.0f);
        g.setColour(palette::kBackgroundBottom.withAlpha(0.65f));
        g.fillRoundedRectangle(schematic, 3.0f);
        g.setColour(palette::kBorderBright.withAlpha(active ? 0.55f : 0.2f));
        g.drawRoundedRectangle(schematic, 3.0f, 0.8f);
        if (active)
            fxglyphs::paintEffectTypeGlyph(g, schematic.reduced(2.0f), effectType, true);

        auto mixBar = inner.reduced(1.0f, 0.0f);
        mixBar = mixBar.removeFromBottom(3.0f);
        g.setColour(palette::kBackgroundBottom);
        g.fillRoundedRectangle(mixBar, 1.5f);
        if (active)
        {
            auto mixFill = mixBar.reduced(0.5f);
            mixFill.setWidth(mixFill.getWidth() * juce::jlimit(0.08f, 1.0f, mix));
            g.setColour(palette::kAccentWarm.withAlpha(0.85f));
            g.fillRoundedRectangle(mixFill, 1.0f);
        }
    }

    void FxChainFlowView::paint(juce::Graphics& g)
    {
        const auto full = getLocalBounds().toFloat();
        g.setColour(palette::kPanel);
        g.fillRoundedRectangle(full, 6.0f);

        const float gap = 6.0f;
        const float labelW = 52.0f;
        const float arrowW = 18.0f;
        const float outW = 28.0f;
        const float slotW =
            (full.getWidth() - labelW * 2.0f - arrowW * 6.0f - outW - gap * 11.0f) / 7.0f;

        auto drawArrow = [&](float ax, float y)
        {
            g.setColour(palette::kBorderBright);
            g.drawArrow(juce::Line<float>(ax, y, ax + arrowW - 4.0f, y), 1.2f, 7.0f, 7.0f);
        };

        float x = full.getX() + 6.0f;
        const float yMid = full.getCentreY();

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.5f));
        g.drawText(insertsPostFader_ ? "POST" : "PRE",
                   juce::Rectangle<int>(static_cast<int>(x), static_cast<int>(full.getY()), static_cast<int>(labelW),
                                        static_cast<int>(full.getHeight()) - 12),
                   juce::Justification::centred);
        g.setFont(fonts::label(7.0f));
        g.setColour(palette::kTextSecondary);
        g.drawText("TAP", juce::Rectangle<int>(static_cast<int>(x), static_cast<int>(full.getBottom()) - 12,
                                               static_cast<int>(labelW), 12),
                   juce::Justification::centred);
        x += labelW + gap;

        for (std::size_t i = 0; i < 7; ++i)
        {
            juce::Rectangle<float> box(x, full.getY() + 4.0f, slotW, full.getHeight() - 8.0f);
            paintSlotBox(g, box, i, effectTypes_[i], mixValues_[i]);
            x += slotW + gap;

            if (i == 2)
            {
                g.setColour(palette::kAccentWarm.withAlpha(0.85f));
                g.setFont(fonts::label(8.5f));
                g.drawText("MASTER", juce::Rectangle<float>(x, full.getY(), labelW, full.getHeight()),
                          juce::Justification::centred);
                x += labelW + gap;
            }
            else if (i < 6)
            {
                drawArrow(x, yMid);
                x += arrowW + gap;
            }
        }

        g.setColour(palette::kTextDim);
        g.setFont(fonts::value(9.0f));
        g.drawText("OUT", juce::Rectangle<float>(x, full.getY(), outW, full.getHeight()),
                   juce::Justification::centredLeft);
    }

} // namespace pw8::plugin::ui::wireframe
