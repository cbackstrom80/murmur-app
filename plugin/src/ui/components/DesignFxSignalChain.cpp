#include "DesignFxSignalChain.h"

#include "../PerformanceMetricsUi.h"
#include "../theme/FxTypeGlyphs.h"
#include "../theme/ObsidianFonts.h"
#include "../theme/ObsidianPalette.h"
#include "DesignFxUiState.h"
#include "FxEffectPlayParams.h"

namespace pw8::plugin::ui
{
    namespace
    {
        [[nodiscard]] int readEffectType(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (prefix.isEmpty())
                return 0;
            if (auto* raw = apvts.getRawParameterValue(prefix + "Type"))
                return static_cast<int>(raw->load() + 0.5f);
            return 0;
        }

        [[nodiscard]] float readMix(juce::AudioProcessorValueTreeState& apvts, const juce::String& prefix)
        {
            if (prefix.isEmpty())
                return 0.0f;
            if (auto* raw = apvts.getRawParameterValue(prefix + "Mix"))
                return raw->load();
            return 0.0f;
        }

        void paintMiniGlyph(juce::Graphics& g, juce::Rectangle<float> area, int effectType, bool active,
                            std::size_t chipIndex)
        {
            fxglyphs::paintChipGlyph(g, area, chipIndex, active);
            juce::ignoreUnused(effectType);
        }
    } // namespace

    DesignFxSignalChain::DesignFxSignalChain(juce::AudioProcessorValueTreeState& apvts) : apvts_(apvts)
    {
        startTimerHz(8);
    }

    void DesignFxSignalChain::setSelectedChip(std::size_t chipIndex)
    {
        selectedChip_ = juce::jlimit<std::size_t>(0, kChips.size() - 1, chipIndex);
        repaint();
    }

    void DesignFxSignalChain::timerCallback()
    {
        bool changed = false;
        for (std::size_t i = 0; i < kChips.size(); ++i)
        {
            const auto& chip = kChips[i];
            int type = chip.effectType;
            float mix = 0.0f;
            if (chip.engineSlot >= 0)
            {
                const auto prefix = designFxEngineSlotPrefix(chip.engineSlot);
                type = readEffectType(apvts_, prefix);
                mix = readMix(apvts_, prefix);
                if (type == 0 && chip.effectType != 0)
                    type = 0;
            }
            if (type != liveTypes_[i] || mix != mixLevels_[i])
            {
                liveTypes_[i] = type;
                mixLevels_[i] = mix;
                changed = true;
            }
        }
        if (changed)
            repaint();

        const float nextLoad = estimateFxLoadPercent(apvts_);
        if (std::abs(nextLoad - fxLoadPercent_) > 0.4f)
        {
            fxLoadPercent_ = nextLoad;
            repaint();
        }
    }

    void DesignFxSignalChain::resized() {}

    juce::Rectangle<float> DesignFxSignalChain::tileBoundsForDisplayIndex(std::size_t displayIndex) const
    {
        const auto bounds = getLocalBounds();
        auto pipeline = bounds.withTrimmedTop(layout::kDesignFxPageSignalChainLabelGap
                                                + layout::kDesignFxPageSignalChainLabelHeight
                                                + layout::kDesignFxPageSignalChainLabelGap);
        pipeline = pipeline.removeFromTop(layout::kDesignFxPageSignalChainPipelineHeight).reduced(8, 0);

        const float tileW = static_cast<float>(layout::kDesignFxPageChipWidth);
        const float tileH = static_cast<float>(layout::kDesignFxPageChipHeight);
        const float connW = static_cast<float>(layout::kDesignFxPageFlowConnectorWidth);
        const float inW = static_cast<float>(layout::kDesignFxPageTerminalInWidth);

        float x = static_cast<float>(pipeline.getX()) + inW;
        const float y = static_cast<float>(pipeline.getCentreY()) - tileH * 0.5f;

        for (std::size_t d = 0; d < kChips.size(); ++d)
        {
            if (d == displayIndex)
                return {x, y, tileW, tileH};
            x += tileW;
            if (d + 1 < kChips.size())
                x += connW;
        }
        juce::ignoreUnused(displayIndex);
        return {};
    }

    std::size_t DesignFxSignalChain::displayIndexAt(juce::Point<int> pos) const
    {
        for (std::size_t d = 0; d < kChips.size(); ++d)
        {
            if (tileBoundsForDisplayIndex(d).contains(pos.toFloat()))
                return d;
        }
        return uiState_ != nullptr ? uiState_->displayIndexForChip(selectedChip_) : selectedChip_;
    }

    std::size_t DesignFxSignalChain::chipIndexAt(juce::Point<int> pos) const
    {
        const auto displayIndex = displayIndexAt(pos);
        if (uiState_ != nullptr)
            return uiState_->chipAtDisplayIndex(displayIndex);
        return displayIndex;
    }

    void DesignFxSignalChain::mouseDown(const juce::MouseEvent& event)
    {
        const auto displayIndex = displayIndexAt(event.getPosition());
        const auto idx = uiState_ != nullptr ? uiState_->chipAtDisplayIndex(displayIndex) : displayIndex;
        if (kChips[idx].disabled)
            return;

        if (displayIndex == 0)
        {
            selectedChip_ = idx;
            if (onChipSelected)
                onChipSelected(idx);
            repaint();
            return;
        }

        dragging_ = true;
        dragFromDisplay_ = displayIndex;
        dragHoverDisplay_ = displayIndex;
        selectedChip_ = idx;
        if (onChipSelected)
            onChipSelected(idx);
        repaint();
    }

    void DesignFxSignalChain::mouseDrag(const juce::MouseEvent& event)
    {
        if (!dragging_ || dragFromDisplay_ == 0)
            return;

        const auto hover = displayIndexAt(event.getPosition());
        if (hover != dragHoverDisplay_)
        {
            dragHoverDisplay_ = hover;
            repaint();
        }
    }

    void DesignFxSignalChain::mouseUp(const juce::MouseEvent& event)
    {
        if (!dragging_)
            return;

        const auto hover = displayIndexAt(event.getPosition());
        if (uiState_ != nullptr && dragFromDisplay_ != 0 && hover != 0 && dragFromDisplay_ != hover)
        {
            uiState_->reorderChipDisplay(dragFromDisplay_, hover);
            if (onDisplayOrderChanged)
                onDisplayOrderChanged();
        }

        dragging_ = false;
        dragFromDisplay_ = static_cast<std::size_t>(-1);
        dragHoverDisplay_ = static_cast<std::size_t>(-1);
        repaint();
    }

    void DesignFxSignalChain::paintChip(juce::Graphics& g, juce::Rectangle<float> tile, const ChipDef& def,
                                        std::size_t index, int liveType, float mix, bool selected,
                                        bool dragGhost) const
    {
        const bool bypassed = def.label[0] == 'B' && def.label[1] == 'Y' && liveType == 0;
        const bool moodActive = index == 4 && liveType == 8;
        const bool moodUiActive = index == 4 && selected;
        const bool active = moodUiActive || moodActive || (!def.disabled && liveType != 0 && !bypassed);
        const bool dim = def.disabled || (bypassed && !moodUiActive && !moodActive);

        g.setColour(dim ? palette::kFigmaFxChipFill.withAlpha(0.65f)
                        : palette::kFigmaFxChipFill);
        g.fillRoundedRectangle(tile, 6.0f);
        g.setColour(selected ? palette::kAccent : palette::kFigmaFxChipBorder.withAlpha(dim ? 0.35f : 1.0f));
        g.drawRoundedRectangle(tile.reduced(0.5f), 6.0f, selected ? 1.5f : 1.0f);

        auto inner = tile.reduced(static_cast<float>(layout::kDesignFxPageChipPadding));

        g.setColour(palette::kTextDim.withAlpha(0.3f));
        for (int dot = 0; dot < 3; ++dot)
            g.fillEllipse(inner.getCentreX() - 3.0f + static_cast<float>(dot) * 3.0f, inner.getY(), 2.0f, 2.0f);
        inner.removeFromTop(6.0f);

        auto header = inner.removeFromTop(10.0f);
        g.setColour(dim ? palette::kTextDim : palette::kTextPrimary);
        g.setFont(fonts::label(8.0f));
        g.drawText(def.label, header.removeFromLeft(header.getWidth() - 8.0f), juce::Justification::centredLeft);
        g.setColour(active ? palette::kAccent : palette::kTextDim);
        g.fillEllipse(header.getRight() - 5.0f, header.getY() + 2.5f, 5.0f, 5.0f);

        auto glyph = inner.removeFromTop(22.0f);
        paintMiniGlyph(g, glyph.withSizeKeepingCentre(16.0f, 16.0f), def.effectType, active, index);

        auto tray = inner.removeFromTop(5.0f);
        g.setColour(palette::kBackgroundTop);
        g.fillRoundedRectangle(tray, 2.0f);
        g.setColour(palette::kBorder);
        g.drawRoundedRectangle(tray, 2.0f, 0.8f);
        if (active)
        {
            const float mixLevel =
                (index == 4 && (moodUiActive || moodActive)) ? juce::jmax(mix, 0.55f) : mix;
            const float fillW = juce::jmax(2.0f, (tray.getWidth() - 2.0f) * juce::jlimit(0.0f, 1.0f, mixLevel));
            g.setColour(palette::kAccent);
            g.fillRoundedRectangle(tray.getX() + 1.0f, tray.getY() + 1.0f, fillW, tray.getHeight() - 2.0f, 1.0f);
        }

        if (dragGhost)
        {
            g.setColour(palette::kAccent.withAlpha(0.18f));
            g.fillRoundedRectangle(tile, 6.0f);
        }

        juce::ignoreUnused(index);
    }

    void DesignFxSignalChain::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds();
        auto labelRow = bounds.removeFromTop(layout::kDesignFxPageSignalChainLabelGap
                                             + layout::kDesignFxPageSignalChainLabelHeight);
        labelRow.removeFromTop(layout::kDesignFxPageSignalChainLabelGap);

        g.setColour(palette::kTextDim);
        g.setFont(fonts::label(9.0f));
        g.drawText("SYSTEM FX ROUTING PIPELINE (DRAG TO REORDER)", labelRow.removeFromLeft(labelRow.getWidth() / 2),
                   juce::Justification::centredLeft);

        g.setColour(palette::kAccent);
        g.setFont(fonts::micro(8.0f));
        g.drawText("LATENCY FLUID SYNC ACTIVE", labelRow, juce::Justification::centredRight);

        bounds.removeFromTop(layout::kDesignFxPageSignalChainLabelGap);
        auto pipeline = bounds.removeFromTop(layout::kDesignFxPageSignalChainPipelineHeight);

        g.setColour(palette::kFigmaFxStatusBarFill);
        g.fillRoundedRectangle(pipeline.toFloat(), 8.0f);
        g.setColour(palette::kFigmaFxCardBorderDim);
        g.drawRoundedRectangle(pipeline.toFloat().reduced(0.5f), 8.0f, 1.0f);

        auto flow = pipeline.reduced(8, 6);

        g.setColour(palette::kAccent);
        g.fillEllipse(static_cast<float>(flow.getX()), static_cast<float>(flow.getCentreY() - 4), 8.0f, 8.0f);
        g.setColour(palette::kAccent);
        g.setFont(fonts::label(8.0f));
        g.drawText("IN", flow.getX() + 12, flow.getCentreY() - 5, 16, 10, juce::Justification::centredLeft);
        g.fillRect(static_cast<float>(flow.getX() + 25), static_cast<float>(flow.getCentreY() - 1), 10.0f, 2.0f);

        const float tileW = static_cast<float>(layout::kDesignFxPageChipWidth);
        const float tileH = static_cast<float>(layout::kDesignFxPageChipHeight);
        const float connW = static_cast<float>(layout::kDesignFxPageFlowConnectorWidth);
        float x = static_cast<float>(flow.getX() + layout::kDesignFxPageTerminalInWidth);
        const float y = static_cast<float>(flow.getCentreY()) - tileH * 0.5f;

        for (std::size_t d = 0; d < kChips.size(); ++d)
        {
            const std::size_t i = uiState_ != nullptr ? uiState_->chipAtDisplayIndex(d) : d;
            const bool ghost = dragging_ && d == dragFromDisplay_;
            const bool dropHint = dragging_ && d == dragHoverDisplay_ && d != dragFromDisplay_;
            auto tile = juce::Rectangle<float>(x, y, tileW, tileH);
            if (dropHint)
            {
                g.setColour(palette::kAccent.withAlpha(0.25f));
                g.drawRoundedRectangle(tile.expanded(2.0f), 8.0f, 2.0f);
            }
            paintChip(g, tile, kChips[i], i, liveTypes_[i], mixLevels_[i], i == selectedChip_, ghost);
            x += tileW;
            if (i + 1 < kChips.size())
            {
                g.setColour(palette::kAccent.withAlpha(0.8f));
                g.fillRect(x, y + tileH * 0.5f - 1.0f, connW, 2.0f);
                x += connW;
            }
        }

        g.setColour(palette::kAccent.withAlpha(0.8f));
        g.fillRect(x, y + tileH * 0.5f - 1.0f, 10.0f, 2.0f);
        g.setColour(palette::kAccent);
        g.drawText("OUT", static_cast<int>(x + 14.0f), static_cast<int>(y + tileH * 0.5f - 5.0f), 24, 10,
                   juce::Justification::centredLeft);
        g.fillEllipse(x + 36.0f, y + tileH * 0.5f - 4.0f, 8.0f, 8.0f);
    }

} // namespace pw8::plugin::ui
