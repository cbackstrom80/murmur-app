#pragma once

#include <cstdint>

#include <juce_graphics/juce_graphics.h>

namespace pw8::plugin::ui::preview
{

enum class FxAnimKind : int;

/// Pre-baked horizontal atlases embedded via BinaryData (128×64 frames, 24 frames).
/// Used for default param keys; runtime bake handles everything else.
struct FxEmbeddedAtlases
{
    static constexpr int kFrameWidth = 128;
    static constexpr int kFrameHeight = 64;
    static constexpr int kFrameCount = 24;

    [[nodiscard]] static const FxEmbeddedAtlases& instance() noexcept;

    [[nodiscard]] const juce::Image& atlasFor(FxAnimKind kind) const noexcept;
    [[nodiscard]] bool hasAtlas(FxAnimKind kind) const noexcept;
    [[nodiscard]] uint64_t defaultParamKey(FxAnimKind kind) const noexcept;

private:
    FxEmbeddedAtlases();

    std::array<juce::Image, 5> atlases_{};
    std::array<bool, 5> loaded_{};
};

} // namespace pw8::plugin::ui::preview
