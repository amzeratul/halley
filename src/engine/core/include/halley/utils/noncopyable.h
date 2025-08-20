#pragma once

namespace Halley {
    class NonCopyable {
    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;

        NonCopyable(NonCopyable&&) noexcept {}
        NonCopyable& operator=(NonCopyable&&) noexcept
        {
	        return *this;
        }

    	NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
}
