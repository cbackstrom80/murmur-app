#include "FxTypeGlyphs.h"

#include "BinaryData.h"
#include "ObsidianPalette.h"

#include <array>
#include <memory>

namespace pw8::plugin::ui::fxglyphs
{
    namespace
    {
        constexpr juce::uint32 kFigmaAccentSoft = 0xff7FE7E0;
        constexpr juce::uint32 kFigmaAccentBright = 0xff00FFD0;
        constexpr juce::uint32 kFigmaMuted = 0xff545A66;

        struct GlyphAsset
        {
            const char* data;
            int size;
        };

        [[nodiscard]] GlyphAsset glyphAssetForChip(std::size_t chipIndex) noexcept
        {
            switch (chipIndex)
            {
                case 0: return {BinaryData::fx_bypass_svg, BinaryData::fx_bypass_svgSize};
                case 1: return {BinaryData::fx_sat_svg, BinaryData::fx_sat_svgSize};
                case 2: return {BinaryData::fx_chr_svg, BinaryData::fx_chr_svgSize};
                case 3: return {BinaryData::fx_tape_svg, BinaryData::fx_tape_svgSize};
                case 4: return {BinaryData::fx_mood_svg, BinaryData::fx_mood_svgSize};
                case 5: return {BinaryData::fx_fshf_svg, BinaryData::fx_fshf_svgSize};
                case 6: return {BinaryData::fx_frac_svg, BinaryData::fx_frac_svgSize};
                case 7: return {BinaryData::fx_rev_svg, BinaryData::fx_rev_svgSize};
                case 8: return {BinaryData::fx_eq_svg, BinaryData::fx_eq_svgSize};
                case 9: return {BinaryData::fx_comp_svg, BinaryData::fx_comp_svgSize};
                case 10: return {BinaryData::fx_lim_svg, BinaryData::fx_lim_svgSize};
                case 11: return {BinaryData::fx_voc_svg, BinaryData::fx_voc_svgSize};
                default: return {nullptr, 0};
            }
        }

        [[nodiscard]] std::size_t chipIndexForEffectType(int effectType) noexcept
        {
            switch (effectType)
            {
                case 0: return 0;
                case 1: return 1;
                case 2: return 2;
                case 3: return 3;
                case 5: return 5;
                case 6: return 6;
                case 7: return 7;
                case 8: return 8;
                case 9: return 9;
                case 10: return 10;
                case 11: return 11;
                default: return 0;
            }
        }

        [[nodiscard]] juce::Drawable* drawableForChip(std::size_t chipIndex)
        {
            static std::array<std::unique_ptr<juce::Drawable>, 12> cache{};
            if (chipIndex >= cache.size())
                return nullptr;

            if (cache[chipIndex] == nullptr)
            {
                const auto asset = glyphAssetForChip(chipIndex);
                if (asset.data == nullptr || asset.size <= 0)
                    return nullptr;

                if (auto xml = juce::parseXML(juce::String::fromUTF8(asset.data, asset.size)))
                    cache[chipIndex] = juce::Drawable::createFromSVG(*xml);
            }

            return cache[chipIndex].get();
        }

        void paintGlyph(juce::Graphics& g, juce::Rectangle<float> bounds, std::size_t chipIndex, bool active)
        {
            if (auto* drawable = drawableForChip(chipIndex))
            {
                auto copy = drawable->createCopy();
                const auto colour = active ? palette::kAccent.withAlpha(0.85f) : palette::kTextDim.withAlpha(0.35f);
                copy->replaceColour(juce::Colour(kFigmaAccentSoft), colour);
                copy->replaceColour(juce::Colour(kFigmaAccentBright), colour);
                copy->replaceColour(juce::Colour(kFigmaMuted), colour);
                copy->drawWithin(g, bounds, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize,
                                 1.0f);
                return;
            }

            g.setColour(active ? palette::kAccent.withAlpha(0.85f) : palette::kTextDim.withAlpha(0.35f));
            g.drawEllipse(bounds.reduced(bounds.getWidth() * 0.2f), 1.0f);
        }
    } // namespace

    void paintChipGlyph(juce::Graphics& g, juce::Rectangle<float> bounds, std::size_t chipIndex, bool active)
    {
        paintGlyph(g, bounds, chipIndex, active);
    }

    void paintEffectTypeGlyph(juce::Graphics& g, juce::Rectangle<float> bounds, int effectType, bool active)
    {
        paintGlyph(g, bounds, chipIndexForEffectType(effectType), active);
    }

} // namespace pw8::plugin::ui::fxglyphs
