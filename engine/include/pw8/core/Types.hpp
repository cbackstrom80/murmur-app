#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

// Core fixed-capacity types and strong IDs shared across pw8_core.
// No heap allocation, no exceptions, no framework dependency.

namespace pw8::core
{
    using Sample = float;

    /// Number of synthesis nodes ("operators") per layer. Architectural constant.
    inline constexpr std::size_t kNodesPerLayer = 8;

    /// Number of layers per patch.
    inline constexpr std::size_t kLayerCount = 2;

    /// Default / maximum polyphony. Configurable per-engine instance up to this ceiling.
    inline constexpr std::size_t kMaxVoices = 32;
    inline constexpr std::size_t kDefaultVoices = 16;

    inline constexpr std::size_t kMaxModRoutes = 64;
    inline constexpr std::size_t kMaxUnisonVoices = 16;
    inline constexpr std::size_t kMaxAlgorithmEdges = 32;

    /// Envelopes and LFOs per layer/voice (docs/MODULATION.md "8 envelopes / 8
    /// LFOs"). Index 0 of each is conventionally "the" amp envelope / LFO1 (the
    /// only one wired to the VCA and to voice lifetime, see voice::Voice::isFree());
    /// all 8 of each are otherwise fully general-purpose mod matrix sources. Shared
    /// between pw8::patch (schema) and pw8::voice (runtime) so both stay in lockstep.
    inline constexpr std::size_t kNumEnvelopesPerLayer = 8;
    inline constexpr std::size_t kNumLfosPerLayer = 8;

    /// A strong, zero-cost wrapper around an integral ID so different ID kinds can't be
    /// accidentally mixed at a call site (e.g. passing a NodeId where a VoiceId is expected).
    template <typename Tag, typename Underlying = std::uint32_t>
    class StrongId
    {
    public:
        using ValueType = Underlying;

        constexpr StrongId() noexcept = default;
        constexpr explicit StrongId(Underlying v) noexcept : value_(v) {}

        [[nodiscard]] constexpr Underlying get() const noexcept { return value_; }
        [[nodiscard]] constexpr bool isValid() const noexcept { return value_ != invalidValue(); }

        [[nodiscard]] static constexpr StrongId invalid() noexcept { return StrongId(invalidValue()); }

        constexpr bool operator==(const StrongId&) const noexcept = default;

    private:
        [[nodiscard]] static constexpr Underlying invalidValue() noexcept
        {
            return std::numeric_limits<Underlying>::max();
        }

        Underlying value_ = invalidValue();
    };

    namespace detail
    {
        struct NodeIdTag {};
        struct VoiceIdTag {};
        struct LayerIdTag {};
        struct ModRouteIdTag {};
        struct MacroIdTag {};
    } // namespace detail

    using NodeId = StrongId<detail::NodeIdTag, std::uint8_t>;
    using VoiceId = StrongId<detail::VoiceIdTag, std::uint32_t>;
    using LayerId = StrongId<detail::LayerIdTag, std::uint8_t>;
    using ModRouteId = StrongId<detail::ModRouteIdTag, std::uint16_t>;
    using MacroId = StrongId<detail::MacroIdTag, std::uint8_t>;

    enum class Layer : std::uint8_t
    {
        A = 0,
        B = 1
    };

    /// Simple fixed-capacity vector for control-path / precompiled data.
    /// NOT used for realtime allocation -- capacity is fixed at compile time and
    /// push_back beyond capacity is a no-op guarded by an assert in debug builds.
    template <typename T, std::size_t Capacity>
    class FixedVector
    {
    public:
        using value_type = T;

        constexpr void push_back(const T& v) noexcept
        {
            if (size_ < Capacity)
                data_[size_++] = v;
        }

        constexpr void clear() noexcept { size_ = 0; }

        [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
        [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
        [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

        [[nodiscard]] constexpr T& operator[](std::size_t i) noexcept { return data_[i]; }
        [[nodiscard]] constexpr const T& operator[](std::size_t i) const noexcept { return data_[i]; }

        [[nodiscard]] constexpr T* begin() noexcept { return data_.data(); }
        [[nodiscard]] constexpr T* end() noexcept { return data_.data() + size_; }
        [[nodiscard]] constexpr const T* begin() const noexcept { return data_.data(); }
        [[nodiscard]] constexpr const T* end() const noexcept { return data_.data() + size_; }

    private:
        std::array<T, Capacity> data_{};
        std::size_t size_ = 0;
    };

} // namespace pw8::core
