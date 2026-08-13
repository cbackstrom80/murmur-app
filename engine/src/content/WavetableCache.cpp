#include "pw8/content/WavetableCache.hpp"

#include "pw8/oscillator/WavetableTableLoader.hpp"

namespace pw8::content
{
    WavetableCache& WavetableCache::instance() noexcept
    {
        static WavetableCache cache;
        return cache;
    }

    std::shared_ptr<const oscillator::WavetableTable> WavetableCache::getOrLoad(const std::string& path)
    {
        if (path.empty())
            return nullptr;

        {
            std::lock_guard lock(mutex_);
            const auto it = entries_.find(path);
            if (it != entries_.end())
                if (auto existing = it->second.lock())
                    return existing;
        }

        auto loadResult = oscillator::loadWavetableFromFile(path);
        if (!loadResult.ok)
            return nullptr;

        auto table = std::make_shared<const oscillator::WavetableTable>(std::move(loadResult.table));

        {
            std::lock_guard lock(mutex_);
            entries_[path] = table;
        }

        return table;
    }

    void WavetableCache::clear() noexcept
    {
        std::lock_guard lock(mutex_);
        entries_.clear();
    }

} // namespace pw8::content
