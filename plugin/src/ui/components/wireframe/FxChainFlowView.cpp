#include "FxChainFlowView.h"

#include "../../theme/ObsidianFonts.h"
#include "../../theme/ObsidianPalette.h"

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

        g.setColour(selected ? palette::kAccentDim : palette::kPanelRaised);
        g.fillRoundedRectangle(box, 5.0f);
        g.setColour(selected ? palette::kAccent : palette::kBorderBright);
        g.drawRoundedRectangle(box.reduced(0.5f), 5.0f, selected ? 1.8f : 1.0f);

        auto inner = box.reduced(4.0f, 3.0f);
        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(8.0f));
        g.drawText(slotShortLabel(slotIndex), inner.removeFromTop(11.0f), juce::Justification::centred);

        g.setColour(active ? palette::kAccent.withAlpha(juce::jlimit(0.45f, 1.0f, mix + 0.25f)) : palette::kTextDim);
        g.setFont(fonts::label(9.5f));
        g.drawText(active ? spec.flowAbbrev : "OFF", inner.removeFromTop(14.0f), juce::Justification::centred);

        // Mini schematic strip
        auto schematic = inner.reduced(2.0f, 1.0f);
        g.setColour(palette::kBorderBright.withAlpha(active ? 0.55f : 0.2f));
        g.drawRoundedRectangle(schematic, 3.0f, 0.8f);
        if (active)
        {
            juce::Path p;
            p.startNewSubPath(schematic.getX() + 4.0f, schematic.getCentreY());
            switch (effectType)
            {
                case 1: // sat curve
                    for (int i = 0; i <= 12; ++i)
                    {
                        const float t = static_cast<float>(i) / 12.0f;
                        const float x = schematic.getX() + 4.0f + t * (schematic.getWidth() - 8.0f);
                        const float v = std::tanh((t * 2.0f - 1.0f) * 2.0f);
                        const float y = schematic.getCentreY() - v * schematic.getHeight() * 0.35f;
                        if (i == 0)
                            p.startNewSubPath(x, y);
                        else
                            p.lineTo(x, y);
                    }
                    break;
                case 3:
                case 5:
                case 6: // delay loop
                    p.lineTo(schematic.getRight() - 4.0f, schematic.getCentreY());
                    p.quadraticTo(schematic.getRight() - 4.0f, schematic.getY() + 3.0f, schematic.getCentreX(),
                                  schematic.getY() + 3.0f);
                    p.quadraticTo(schematic.getX() + 4.0f, schematic.getY() + 3.0f, schematic.getX() + 4.0f,
                                  schematic.getCentreY());
                    break;
                case 7: // reverb ring
                    g.drawEllipse(schematic.getCentreX() - schematic.getWidth() * 0.18f,
                                 schematic.getCentreY() - schematic.getHeight() * 0.22f, schematic.getWidth() * 0.36f,
                                 schematic.getHeight() * 0.44f, 1.0f);
                    break;
                case 9: // comp knee
                    p.lineTo(schematic.getX() + schematic.getWidth() * 0.45f, schematic.getCentreY());
                    p.lineTo(schematic.getRight() - 4.0f, schematic.getBottom() - 4.0f);
                    break;
                default:
                    p.lineTo(schematic.getRight() - 4.0f, schematic.getCentreY());
                    break;
            }
            g.setColour(palette::kAccent.withAlpha(0.85f));
            g.strokePath(p, juce::PathStrokeType(1.2f));
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
        g.drawText("LAYER", juce::Rectangle<float>(x, full.getY(), labelW, full.getHeight()),
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
