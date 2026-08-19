#include "FxEmbeddedAtlases.h"

#include "BinaryData.h"
#include "FxAnimationAtlas.h"
#include "VisualTheme.h"

namespace pw8::plugin::ui::preview
{
    namespace
    {
        [[nodiscard]] juce::Image loadEmbeddedPng(const void* data, int size)
        {
            if (data == nullptr || size <= 0)
                return {};

            juce::MemoryInputStream stream(data, static_cast<std::size_t>(size), false);
            return juce::ImageFileFormat::loadFrom(stream);
        }

        [[nodiscard]] int kindIndex(FxAnimKind kind) noexcept
        {
            return static_cast<int>(kind);
        }
    } // namespace

    const FxEmbeddedAtlases& FxEmbeddedAtlases::instance() noexcept
    {
        static FxEmbeddedAtlases atlases;
        return atlases;
    }

    FxEmbeddedAtlases::FxEmbeddedAtlases()
    {
        struct AtlasResource
        {
            FxAnimKind kind;
            const void* data;
            int size;
        };

        static const AtlasResource kResources[] = {
            { FxAnimKind::Chorus, BinaryData::chorus_atlas_png, BinaryData::chorus_atlas_pngSize },
            { FxAnimKind::TapeDrift, BinaryData::tape_atlas_png, BinaryData::tape_atlas_pngSize },
            { FxAnimKind::ReverbDecay, BinaryData::reverb_atlas_png, BinaryData::reverb_atlas_pngSize },
            { FxAnimKind::Clouds, BinaryData::clouds_atlas_png, BinaryData::clouds_atlas_pngSize },
        };

        for (const auto& resource : kResources)
        {
            const int idx = kindIndex(resource.kind);
            atlases_[static_cast<std::size_t>(idx)] = loadEmbeddedPng(resource.data, resource.size);
            loaded_[static_cast<std::size_t>(idx)] = atlases_[static_cast<std::size_t>(idx)].isValid();
        }
    }

    const juce::Image& FxEmbeddedAtlases::atlasFor(FxAnimKind kind) const noexcept
    {
        return atlases_[static_cast<std::size_t>(kindIndex(kind))];
    }

    bool FxEmbeddedAtlases::hasAtlas(FxAnimKind kind) const noexcept
    {
        return loaded_[static_cast<std::size_t>(kindIndex(kind))];
    }

    uint64_t FxEmbeddedAtlases::defaultParamKey(FxAnimKind kind) const noexcept
    {
        switch (kind)
        {
            case FxAnimKind::Chorus: return chorusAnimKey(1.0f, 5.0f, 0.5f);
            case FxAnimKind::TapeDrift: return tapeAnimKey(0.5f, 5.0f, 0.5f);
            case FxAnimKind::ReverbDecay: return reverbAnimKey(2.0f, 0.5f, 0.5f, 0.5f);
            case FxAnimKind::Clouds: return cloudsAnimKey(0.5f, 0xC10D5);
            default: return 0;
        }
    }

} // namespace pw8::plugin::ui::preview
