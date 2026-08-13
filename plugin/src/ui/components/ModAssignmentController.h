#pragma once

#include <functional>
#include <optional>

#include "pw8/modulation/ModMatrixTypes.hpp"

namespace pw8::plugin::ui
{
    /// Shared "armed source" state for click-to-route modulation assignment.
    /// One chip arms a source; any mod target (knob or quick-assign button) completes the route.
    class ModAssignmentController
    {
    public:
        [[nodiscard]] bool isArmed() const noexcept { return armedSource_.has_value(); }

        [[nodiscard]] std::optional<modulation::ModSource> armedSource() const noexcept { return armedSource_; }

        /// Arms `source`, or disarms if it is already armed (toggle).
        void arm(modulation::ModSource source)
        {
            if (armedSource_ == source)
            {
                disarm();
                return;
            }
            armedSource_ = source;
            notifyChanged();
        }

        void disarm()
        {
            if (!armedSource_.has_value())
                return;
            armedSource_.reset();
            notifyChanged();
        }

        std::function<void()> onChanged;

    private:
        void notifyChanged()
        {
            if (onChanged)
                onChanged();
        }

        std::optional<modulation::ModSource> armedSource_;
    };

} // namespace pw8::plugin::ui
