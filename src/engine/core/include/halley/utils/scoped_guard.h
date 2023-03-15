#pragma once

#include <utility>

namespace Halley {
    template <typename F>
    class ScopedGuard {
    public:
        ScopedGuard(F f) : f(f) {}
        ScopedGuard(const ScopedGuard& other) = delete;
        ScopedGuard(ScopedGuard&& other)
            : f(std::move(other.f))
        {
            active = other.active;
            other.active = false;
        }

        ~ScopedGuard()
        {
            if (active) {
                f();
            }
        }

        ScopedGuard& operator=(const ScopedGuard& other) = delete;

        ScopedGuard& operator=(ScopedGuard&& other) noexcept
        {
            f = std::move(other.f);
            active = other.active;
            other.active = false;
            return *this;
        }

    private:
        F f;
        bool active = true;
    };
}
