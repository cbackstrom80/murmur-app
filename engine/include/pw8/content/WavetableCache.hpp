#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "pw8/oscillator/WavetableTable.hpp"

// Control-thread-only cache for loaded wavetable content. Hash/path -> shared
// immutable table so multiple operators (or reloads) referencing the same file
// share one copy. Never called from process()/renderSample().

namespace pw8::content
{
    class WavetableCache
    {
    public:
        [[nodiscard]] static WavetableCache& instance() noexcept;

        /// Returns a shared table for `path`, loading from disk on first use.
        /// Returns nullptr if the file fails to load.
        [[nodiscard]] std::shared_ptr<const oscillator::WavetableTable> getOrLoad(const std::string& path);

        void clear() noexcept;

    private:
        WavetableCache() = default;

        std::mutex mutex_;
        std::unordered_map<std::string, std::weak_ptr<const oscillator::WavetableTable>> entries_;
    };

} // namespace pw8::content
