#pragma once

#include <mutex>
#include <condition_variable>
#include <chrono>

namespace Halley {
    class Mutex {
    public:
        Mutex()
            : mutex()
        {}

        std::mutex& getStdMutex() { return mutex; }
        const std::mutex& getStdMutex() const { return mutex; }

    private:
        std::mutex mutex;
    };

    class UniqueLock {
    public:
        UniqueLock(Mutex& mutex)
            : uniqueLock(mutex.getStdMutex())
        {}

        void lock() { uniqueLock.lock(); }
        bool tryLock() { return uniqueLock.try_lock(); }
        void unlock() { uniqueLock.unlock(); }

        std::unique_lock<std::mutex>& getLock() { return uniqueLock; }
        const std::unique_lock<std::mutex>& getLock() const { return uniqueLock; }

    private:
        std::unique_lock<std::mutex> uniqueLock;
    };

    class ConditionVariable {
    public:
        ConditionVariable()
            : cond()
        {}

        void wait(UniqueLock& lock)
        {
            cond.wait(lock.getLock());
        }

        template<class Rep, class Period>
        bool waitFor(UniqueLock& lock, const std::chrono::duration<Rep, Period>& time)
        {
            return cond.wait_for(lock.getLock, time) == std::cv_status::no_timeout;
        }

        void notifyOne()
        {
            cond.notify_one();
        }

        void notifyAll()
        {
            cond.notify_all();
        }

    private:
        std::condition_variable cond;
    };
};
