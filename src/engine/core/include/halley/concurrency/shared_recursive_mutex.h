#pragma once

#include <shared_mutex>
#include <thread>
#include "mutex.h"

namespace Halley {
    class SharedRecursiveMutex {
    public:
        using MutexType = SharedRecursiveMutex;

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

        MutexType& getStdMutex() { return *this; }
	    const MutexType& getStdMutex() const { return *this; }

    private:
#ifdef __PROSPERO__
        std::shared_mutex mutex{nullptr};
#else
        std::shared_mutex mutex;
#endif
        std::atomic<std::thread::id> owner;
        int count = 0;
    };
}