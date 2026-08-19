#include "DesignFxCardBrowser.h"

#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "DesignFxSignalChain.h"
#include "state/PluginState.h"

namespace pw8::plugin::ui
{
    namespace
    {
        inline const juce::Colour kCardFill = palette::kFigmaFxCardFill;
        inline const juce::Colour kCardBorder = palette::kFigmaFxCardBorder;
        inline const juce::Colour kCardCategoryText = palette::kFigmaFxMutedText;
        inline const juce::Colour kCardFooterHint = palette::kFigmaFxFooterHint;

        [[nodiscard]] int readEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        void setEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix, int typeOrdinal)
        {
            if (auto* param = apvts.getParameter(prefix + "Type"))
                param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(typeOrdinal)));
        }

        [[nodiscard]] float readParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix,
                                      const char* suffix, float fallback = 0.0f)
        {
            if (auto* raw = apvts.getRawParameterValue(prefix + suffix))
                return raw->load();
            return fallback;
        }

        [[nodiscard]] std::size_t preferredQuasarFxSlotIndex(juce::AudioProcessorValueTreeState& apvts)
        {
            for (std::size_t i = 3; i < 7; ++i)
            {
                const auto prefix = pw8::plugin::masterFxParamId(i - 3, "");
                if (readEffectType(apvts, prefix) == 13)
                    return i;
            }
            return 5;
        }

        struct CardKnobDef
        {
            const char* label;
            const char* suffix;
            bool percent;
            bool isDb;
        };

        struct CardDef
        {
            const char* title;
            const char* category;
            const char* blurb;
            int chipIndex;
            int engineSlot;
            int defaultEffectType;
            juce::uint32 accentArgb;
            bool quasar;
            std::array<CardKnobDef, 3> knobs{};
        };

        const CardDef kCards[layout::kDesignFxCardBrowserCount] = {
            {"SATURATOR", "DISTORTION", "Symmetrical wave shaping & drive", 1, 0, 1, 0xffe8a33d, false,
             {{{"DRIVE", "SaturationDrive", false, true}, {"WET", "Mix", true, false}, {"CLIP", "", false, false}}}},
            {"CHORUS", "MODULATION", "Multi-voice ensemble dimension", 2, 1, 2, 0xff45c8e8, false,
             {{{"RATE", "ChorusRate", false, false}, {"DEPTH", "ChorusDepth", true, false}, {"FEED", "", true, false}}}},
            {"TAPE", "SATURATION", "Warm analog heat & tape flutter", 3, 2, 3, 0xffd4a055, false,
             {{{"SAT", "TapeDrive", false, false}, {"WOW", "TapeDriftDepth", true, false}, {"BIAS", "", false, false}}}},
            {"MOOD FILTER", "FILTER", "Classic lowpass transistor sweep", 4, 2, 8, 0xffb06cff, false,
             {{{"CUTOFF", "", false, false}, {"RESO", "", true, false}, {"ENV", "", true, false}}}},
            {"FREQ SHIFT", "FREQUENCY", "Symmetric frequency spectrum offset", 5, 3, 5, 0xff55d48a, false,
             {{{"SHIFT", "FreqShiftHz", false, false}, {"FEED", "FreqShiftFeedback", true, false}, {"WET", "Mix", true, false}}}},
            {"FRACTURE", "GLITCH", "Shattered buffer delay & decimate", 6, 4, 6, 0xffff6cab, false,
             {{{"SIZE", "FractalBaseDelayMs", false, false}, {"DENS", "FractalMorph", true, false}, {"BITS", "", false, false}}}},
            {"REVERB", "REVERB SPACE", "Diffuse algorithmic algorithmic space", 7, 2, 7, 0xff5a8cff, false,
             {{{"DECAY", "ReverbDecaySeconds", false, false}, {"SIZE", "ReverbSize", false, false}, {"WET", "Mix", true, false}}}},
            {"EQ", "EQUALIZER", "3-band sweepable parametric EQ", 8, 5, 8, 0xff4fd4c4, false,
             {{{"LOW", "EqLowGainDb", false, false}, {"MID", "EqMidGainDb", false, false}, {"HIGH", "EqHighGainDb", false, false}}}},
            {"COMP / LIM", "DYNAMICS", "Stereo bus dynamic peak leveling", 9, 6, 9, 0xffe87050, false,
             {{{"THR", "CompThresholdDb", false, true}, {"RATIO", "CompRatio", false, false}, {"GAIN", "CompMakeupDb", false, true}}}},
            {"QUASAR", "DYNAMICS", "Binaural spatial processing & stereo width", -1, -1, 13, 0xff7c4dff, true,
             {{{"THRESH", "", false, true}, {"CEIL", "", false, false}, {"REL", "", false, false}}}},
        };

        [[nodiscard]] juce::String formatKnobValue(const CardDef& card, int knobIndex, float value)
        {
            const auto& knob = card.knobs[static_cast<std::size_t>(knobIndex)];
            if (knob.suffix[0] == '\0')
            {
                if (knobIndex == 2 && card.chipIndex == 1)
                    return "SOFT";
                if (knobIndex == 2 && card.chipIndex == 3)
                    return juce::String(value, 1);
                if (card.quasar && knobIndex == 1)
                    return juce::String(value, 1);
                if (card.quasar && knobIndex == 2)
                    return juce::String(juce::roundToInt(value * 1000.0f)) + "ms";
                return knob.percent ? juce::String(juce::roundToInt(value * 100.0f)) + "%" : juce::String(value, 1);
            }

            if (knob.isDb)
                return juce::String(juce::roundToInt(value)) + "dB";
            if (knob.percent)
                return juce::String(juce::roundToInt(value * 100.0f)) + "%";
            if (juce::String(knob.suffix).containsIgnoreCase("Seconds"))
                return juce::String(value, 1) + "s";
            if (juce::String(knob.suffix).containsIgnoreCase("Hz"))
                return juce::String(juce::roundToInt(value)) + "Hz";
            return juce::String(value, 1);
        }

        [[nodiscard]] float normalizedKnobValue(const CardDef& card, int knobIndex, float raw)
        {
            if (card.knobs[static_cast<std::size_t>(knobIndex)].percent)
                return juce::jlimit(0.0f, 1.0f, raw);
            if (card.knobs[static_cast<std::size_t>(knobIndex)].isDb)
                return juce::jlimit(0.0f, 1.0f, (raw + 60.0f) / 60.0f);
            return juce::jlimit(0.0f, 1.0f, raw / juce::jmax(1.0f, std::abs(raw)));
        }

        void paintMiniKnob(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour accent, juce::Colour valueColour,
                           float norm, const juce::String& label, const juce::String& value)
        {
            const int dial = layout::kDesignFxCardMiniKnobSize;
            auto dialBounds = bounds.withSizeKeepingCentre(dial, dial);
            g.setColour(palette::kFigmaFxVizFill);
            g.fillEllipse(dialBounds.toFloat());
            g.setColour(palette::kFigmaFxChipBorder);
            g.drawEllipse(dialBounds.toFloat(), 1.0f);

            juce::Path arc;
            arc.addCentredArc(static_cast<float>(dialBounds.getCentreX()), static_cast<float>(dialBounds.getCentreY()),
                              static_cast<float>(dial) * 0.5f - 2.0f, static_cast<float>(dial) * 0.5f - 2.0f, 0.0f,
                              juce::MathConstants<float>::pi * 1.25f,
                              juce::MathConstants<float>::pi * 1.25f + norm * juce::MathConstants<float>::pi * 1.5f, true);
            g.setColour(accent);
            g.strokePath(arc, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            auto readout = bounds.withTrimmedTop(dial + 4);
            g.setColour(kCardCategoryText);
            g.setFont(fonts::micro(6.0f));
            g.drawText(label, readout.removeFromTop(8), juce::Justification::centred);
            g.setColour(valueColour);
            g.setFont(fonts::denseBold(7.0f));
            g.drawText(value, readout, juce::Justification::centred);
        }

        void paintCardVis(juce::Graphics& g, juce::Rectangle<int> vis, int cardIndex, juce::Colour accent)
        {
            g.setColour(palette::kFigmaFxVizFill);
            g.fillRoundedRectangle(vis.toFloat(), 4.0f);
            g.setColour(palette::kFigmaFxCardBorderDim);
            g.drawRoundedRectangle(vis.toFloat(), 4.0f, 1.0f);

            auto plot = vis.reduced(10, 8);
            g.setColour(palette::kBorder.withAlpha(0.45f));
            g.drawHorizontalLine(plot.getCentreY(), static_cast<float>(plot.getX()),
                                 static_cast<float>(plot.getRight()));

            g.setColour(accent.withAlpha(0.85f));
            switch (cardIndex)
            {
                case 0:
                {
                    juce::Path wave;
                    for (int x = 0; x < plot.getWidth(); ++x)
                    {
                        const float t = static_cast<float>(x) / static_cast<float>(plot.getWidth());
                        const float y = plot.getCentreY() + std::sin(t * 18.0f) * 10.0f * (1.0f - t * 0.35f);
                        if (x == 0)
                            wave.startNewSubPath(static_cast<float>(plot.getX() + x), y);
                        else
                            wave.lineTo(static_cast<float>(plot.getX() + x), y);
                    }
                    g.strokePath(wave, juce::PathStrokeType(1.4f));
                    break;
                }
                case 2:
                {
                    g.drawEllipse(static_cast<float>(plot.getX() + 20), static_cast<float>(plot.getCentreY() - 12), 24.0f,
                                  24.0f, 1.2f);
                    g.drawEllipse(static_cast<float>(plot.getRight() - 44), static_cast<float>(plot.getCentreY() - 12),
                                  24.0f, 24.0f, 1.2f);
                    g.drawLine(static_cast<float>(plot.getX() + 44), static_cast<float>(plot.getCentreY()),
                               static_cast<float>(plot.getRight() - 44), static_cast<float>(plot.getCentreY()), 1.0f);
                    break;
                }
                case 3:
                {
                    juce::Path curve;
                    curve.startNewSubPath(static_cast<float>(plot.getX()), static_cast<float>(plot.getBottom() - 6));
                    curve.quadraticTo(static_cast<float>(plot.getCentreX()), static_cast<float>(plot.getY() + 4),
                                      static_cast<float>(plot.getRight()), static_cast<float>(plot.getBottom() - 10));
                    g.strokePath(curve, juce::PathStrokeType(1.5f));
                    break;
                }
                case 4:
                    for (int i = 0; i < 7; ++i)
                    {
                        const int h = 8 + (i * 3) % 24;
                        g.fillRoundedRectangle(static_cast<float>(plot.getX() + 8 + i * 22),
                                               static_cast<float>(plot.getBottom() - h), 6.0f, static_cast<float>(h), 2.0f);
                    }
                    break;
                case 8:
                    g.fillEllipse(static_cast<float>(plot.getX() + 30), static_cast<float>(plot.getCentreY() - 8), 6.0f, 6.0f);
                    g.fillEllipse(static_cast<float>(plot.getCentreX()), static_cast<float>(plot.getBottom() - 14), 6.0f,
                                  6.0f);
                    g.fillEllipse(static_cast<float>(plot.getRight() - 36), static_cast<float>(plot.getCentreY() + 2), 6.0f,
                                  6.0f);
                    break;
                case 9:
                {
                    juce::Path gr;
                    gr.startNewSubPath(static_cast<float>(plot.getX()), static_cast<float>(plot.getBottom() - 8));
                    gr.lineTo(static_cast<float>(plot.getCentreX() - 20), static_cast<float>(plot.getY() + 12));
                    gr.lineTo(static_cast<float>(plot.getRight()), static_cast<float>(plot.getBottom() - 18));
                    g.strokePath(gr, juce::PathStrokeType(1.5f));
                    break;
                }
                default:
                {
                    juce::Path wave;
                    for (int x = 0; x < plot.getWidth(); ++x)
                    {
                        const float t = static_cast<float>(x) / static_cast<float>(plot.getWidth());
                        const float y = plot.getCentreY() + std::sin(t * 12.0f + cardIndex) * 8.0f;
                        if (x == 0)
                            wave.startNewSubPath(static_cast<float>(plot.getX() + x), y);
                        else
                            wave.lineTo(static_cast<float>(plot.getX() + x), y);
                    }
                    g.strokePath(wave, juce::PathStrokeType(1.2f));
                    break;
                }
            }
        }
    } // namespace

    DesignFxCardBrowser::DesignFxCardBrowser(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts) {}

    int DesignFxCardBrowser::chipIndexForCard(int cardIndex) const noexcept
    {
        if (cardIndex < 0 || cardIndex >= layout::kDesignFxCardBrowserCount)
            return -1;
        return kCards[static_cast<std::size_t>(cardIndex)].chipIndex;
    }

    bool DesignFxCardBrowser::isQuasarCard(int cardIndex) const noexcept
    {
        if (cardIndex < 0 || cardIndex >= layout::kDesignFxCardBrowserCount)
            return false;
        return kCards[static_cast<std::size_t>(cardIndex)].quasar;
    }

    juce::Rectangle<int> DesignFxCardBrowser::cardBounds(int cardIndex) const
    {
        const int col = cardIndex % layout::kDesignFxCardColumns;
        const int row = cardIndex / layout::kDesignFxCardColumns;
        const int gridWidth = layout::kDesignFxCardColumns * layout::kDesignFxCardWidth
                              + (layout::kDesignFxCardColumns - 1) * layout::kDesignFxCardGap;
        const int xOffset = juce::jmax(0, (getWidth() - gridWidth) / 2);

        return juce::Rectangle<int>(
            xOffset + col * (layout::kDesignFxCardWidth + layout::kDesignFxCardGap),
            row * (layout::kDesignFxCardHeight + layout::kDesignFxCardGap), layout::kDesignFxCardWidth,
            layout::kDesignFxCardHeight);
    }

    juce::String DesignFxCardBrowser::cardParamPrefix(int cardIndex) const
    {
        const auto& card = kCards[static_cast<std::size_t>(cardIndex)];
        if (card.quasar)
            return pw8::plugin::masterFxParamId(preferredQuasarFxSlotIndex(apvts_) - 3, "");
        if (card.engineSlot < 0)
            return {};
        return designFxEngineSlotPrefix(card.engineSlot);
    }

    bool DesignFxCardBrowser::isCardEnabled(int cardIndex) const
    {
        if (cardIndex < 0 || cardIndex >= layout::kDesignFxCardBrowserCount)
            return false;

        const auto prefix = cardParamPrefix(cardIndex);
        if (prefix.isEmpty())
            return false;

        const auto& card = kCards[static_cast<std::size_t>(cardIndex)];
        if (card.quasar)
            return readEffectType(apvts_, prefix) == 13;
        return readEffectType(apvts_, prefix) != 0;
    }

    void DesignFxCardBrowser::toggleCardEnabled(int cardIndex)
    {
        if (cardIndex < 0 || cardIndex >= layout::kDesignFxCardBrowserCount)
            return;

        const auto& card = kCards[static_cast<std::size_t>(cardIndex)];
        const auto prefix = cardParamPrefix(cardIndex);
        if (prefix.isEmpty())
            return;

        if (card.quasar)
        {
            const int current = readEffectType(apvts_, prefix);
            setEffectType(apvts_, prefix, current == 13 ? 0 : 13);
            repaint();
            return;
        }

        const int current = readEffectType(apvts_, prefix);
        if (current == 0)
            setEffectType(apvts_, prefix, card.defaultEffectType);
        else
            setEffectType(apvts_, prefix, 0);
        repaint();
    }

    void DesignFxCardBrowser::paintCard(juce::Graphics& g, int cardIndex, juce::Rectangle<int> bounds, bool enabled)
    {
        const auto& card = kCards[static_cast<std::size_t>(cardIndex)];
        const auto accent = juce::Colour(card.accentArgb);
        const auto prefix = cardParamPrefix(cardIndex);

        g.setColour(kCardFill);
        g.fillRoundedRectangle(bounds.toFloat(), static_cast<float>(layout::kDesignFxCardRadius));
        g.setColour(kCardBorder);
        g.drawRoundedRectangle(bounds.toFloat(), static_cast<float>(layout::kDesignFxCardRadius), 1.5f);

        auto inner = bounds.reduced(layout::kDesignFxCardPadding);
        g.setColour(accent);
        g.fillRoundedRectangle(inner.removeFromTop(layout::kDesignFxCardAccentStripHeight)
                                   .withWidth(layout::kDesignFxCardAccentStripWidth)
                                   .toFloat(),
                               1.5f);

        inner.removeFromTop(layout::kDesignFxCardSectionGap);
        auto titleRow = inner.removeFromTop(14);
        g.setColour(palette::kTextPrimary);
        g.setFont(fonts::denseBold(11.0f));
        g.drawText(card.title, titleRow, juce::Justification::centredLeft);
        g.setColour(kCardCategoryText);
        g.setFont(fonts::denseBold(7.0f));
        g.drawText(card.category, titleRow, juce::Justification::centredRight);

        inner.removeFromTop(2);
        g.setColour(kCardCategoryText);
        g.setFont(fonts::micro(8.0f));
        g.drawFittedText(card.blurb, inner.removeFromTop(10), juce::Justification::topLeft, 1);

        inner.removeFromTop(layout::kDesignFxCardSectionGap);
        const auto vis = inner.removeFromTop(layout::kDesignFxCardMiniVisHeight)
                               .withWidth(layout::kDesignFxCardMiniVisWidth);
        paintCardVis(g, vis, cardIndex, accent);

        inner.removeFromTop(layout::kDesignFxCardSectionGap);
        auto knobRow = inner.removeFromTop(46);
        const int knobGap = (knobRow.getWidth() - 3 * layout::kDesignFxCardMiniKnobColWidth) / 2;
        const auto valueColour = card.quasar ? palette::kAccentWarm : palette::kAccent;
        for (int k = 0; k < 3; ++k)
        {
            auto col = knobRow.removeFromLeft(layout::kDesignFxCardMiniKnobColWidth);
            if (k < 2)
                knobRow.removeFromLeft(knobGap);
            const auto& knobDef = card.knobs[static_cast<std::size_t>(k)];
            float raw = 0.5f;
            if (prefix.isNotEmpty() && knobDef.suffix[0] != '\0')
                raw = readParam(apvts_, prefix, knobDef.suffix, 0.5f);
            paintMiniKnob(g, col, accent, valueColour, normalizedKnobValue(card, k, raw), knobDef.label,
                          formatKnobValue(card, k, raw));
        }

        inner.removeFromTop(layout::kDesignFxCardSectionGap);
        g.setColour(palette::kFigmaFxChipBorder.withAlpha(0.55f));
        g.drawHorizontalLine(inner.getY(), static_cast<float>(bounds.getX() + layout::kDesignFxCardPadding),
                             static_cast<float>(bounds.getRight() - layout::kDesignFxCardPadding));

        auto footer = bounds.reduced(layout::kDesignFxCardPadding).removeFromBottom(14);
        g.setColour(kCardFooterHint);
        g.setFont(fonts::micro(7.0f));
        g.drawText("CLICK TO EDIT ↗", footer.removeFromLeft(footer.getWidth() - 34), juce::Justification::centredLeft);

        juce::String toggleText = enabled ? "ON" : "OFF";
        g.setFont(fonts::denseBold(8.0f));
        const int toggleW = juce::jmax(layout::kDesignFxCardToggleWidth, g.getCurrentFont().getStringWidth(toggleText) + 12);
        auto toggle = footer.removeFromRight(toggleW).withSizeKeepingCentre(toggleW, layout::kDesignFxCardToggleHeight);
        toggleBounds_[static_cast<std::size_t>(cardIndex)] = toggle;
        if (enabled)
        {
            g.setColour(palette::kFigmaFxToggleOnFill);
            g.fillRoundedRectangle(toggle.toFloat(), 4.0f);
            g.setColour(palette::kFigmaFxToggleOnBorder);
            g.drawRoundedRectangle(toggle.toFloat(), 4.0f, 1.0f);
            g.setColour(palette::kFigmaFxToggleOnText);
        }
        else
        {
            g.setColour(palette::kFigmaFxVizFill);
            g.fillRoundedRectangle(toggle.toFloat(), 4.0f);
            g.setColour(palette::kFigmaFxCardBorderDim);
            g.drawRoundedRectangle(toggle.toFloat(), 4.0f, 1.0f);
            g.setColour(kCardCategoryText);
        }
        g.drawText(toggleText, toggle, juce::Justification::centred);
    }

    void DesignFxCardBrowser::paint(juce::Graphics& g)
    {
        g.fillAll(palette::kBackgroundBottom);

        for (int i = 0; i < layout::kDesignFxCardBrowserCount; ++i)
            paintCard(g, i, cardBounds(i), isCardEnabled(i));
    }

    void DesignFxCardBrowser::mouseDown(const juce::MouseEvent& event)
    {
        for (int i = 0; i < layout::kDesignFxCardBrowserCount; ++i)
        {
            if (toggleBounds_[static_cast<std::size_t>(i)].contains(event.getPosition()))
            {
                toggleCardEnabled(i);
                if (onCardToggle)
                    onCardToggle(i);
                return;
            }

            if (cardBounds(i).contains(event.getPosition()))
            {
                if (onCardSelected)
                    onCardSelected(i);
                return;
            }
        }
    }

    void DesignFxCardBrowser::resized()
    {
        toggleBounds_.fill({});
        repaint();
    }

} // namespace pw8::plugin::ui
