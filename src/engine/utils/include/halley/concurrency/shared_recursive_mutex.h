#pragma once

#include <shared_mutex>

namespace Halley {
    class SharedRecursiveMutex {
    public:
        SharedRecursiveMutex() = default;
        SharedRecursiveMutex(const SharedRecursiveMutex& other) = delete;
        SharedRecursiveMutex(SharedRecursiveMutex&& other) = delete;
        SharedRecursiveMutex& operator=(const SharedRecursiveMutex& other) = delete;
        SharedRecursiveMutex& operator=(SharedRecursiveMutex&& other) = delete;

        void lock();
        [[nodiscard]] bool try_lock();
        void unlock();

        void lock_shared();
        [[nodiscard]] bool try_lock_shared();
        void unlock_shared();

    private:
        std::shared_mutex mutex;
        std::atomic<std::thread::id> owner;
        int count = 0;
    };
}