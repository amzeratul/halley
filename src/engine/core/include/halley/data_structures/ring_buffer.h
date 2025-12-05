#pragma once

#include <gsl/span>
#include <atomic>
#include <new>

namespace Halley {
	// This is a lock-free, wait-free ring buffer
	// It is only thread safe for one producer and one consumer at any given time

    // Inspired by "Single Producer Single Consumer Lock-free FIFO From the Ground Up - Charles Frasch - CppCon 2023"
    // https://www.youtube.com/watch?v=K3P_Lmq6pw0
	
    template <typename T>
    class RingBuffer {
    public:
    	explicit RingBuffer(size_t capacity)
    	{
            entries.resize(capacity);
    	}

        // Copy is not thread-safe
        RingBuffer(const RingBuffer& other)
	        : entries(other.entries)
			, readPos(other.readPos.load())
			, writePos(other.writePos.load())
        {}

        RingBuffer& operator=(const RingBuffer& other)
    	{
            readPos = other.readPos.load();
            writePos = other.writePos.load();
            entries = other.entries;
            return *this;
    	}

        [[nodiscard]] constexpr size_t capacity() const
    	{
            return entries.size();
    	}

    	[[nodiscard]] size_t availableToRead() const
    	{
            return writePos.load(std::memory_order_acquire) - readPos.load(std::memory_order_relaxed);
    	}

        [[nodiscard]] size_t availableToWrite() const
        {
            return capacity() - (writePos.load(std::memory_order_relaxed) - readPos.load(std::memory_order_acquire));
        }

    	[[nodiscard]] bool canWrite(size_t n) const
    	{
            return availableToWrite() >= n;
    	}

    	[[nodiscard]] bool canRead(size_t n) const
    	{
            return availableToRead() >= n;
    	}

    	[[nodiscard]] bool empty() const
    	{
            return availableToRead() == 0;
    	}
    	
        void writeOne(T e)
        {
            assert(canWrite(1));

            const auto pos = writePos.load(std::memory_order_relaxed);
            entries[pos % capacity()] = std::move(e);
            writePos.store(pos + 1, std::memory_order_release);
        }

    	T readOne()
    	{
            assert(canRead(1));

            const auto pos = readPos.load(std::memory_order_relaxed);
            T v = std::move(entries[pos % capacity()]);
            readPos.store(pos + 1, std::memory_order_release);

            return v;
    	}

    	void write(gsl::span<const T> es)
    	{
            const size_t numToWrite = es.size();
            assert(canWrite(numToWrite));

            const auto startPos = writePos.load(std::memory_order_relaxed);
            const auto endPos = startPos + numToWrite;

            const auto sz = capacity();
            const auto p0 = startPos % sz; // Actual first index
            const auto b0sz = std::min(sz - p0, numToWrite); // Number of indices in the first batch
            const auto b1sz = numToWrite - b0sz; // Number of indices in the second batch

            copyData(entries.span().subspan(p0, b0sz), es.subspan(0, b0sz));
            copyData(entries.span().subspan(0, b1sz), es.subspan(b0sz, b1sz));

            writePos.store(endPos, std::memory_order_release);
    	}

    	void write(gsl::span<T> es)
    	{
            const size_t numToWrite = es.size();
            assert(canWrite(numToWrite));

            const auto startPos = writePos.load(std::memory_order_relaxed);
            const auto endPos = startPos + numToWrite;

            const auto sz = capacity();
            const auto p0 = startPos % sz; // Actual first index
            const auto b0sz = std::min(sz - p0, numToWrite); // Number of indices in the first batch
            const auto b1sz = numToWrite - b0sz; // Number of indices in the second batch

            moveData(entries.span().subspan(p0, b0sz), es.subspan(0, b0sz));
            moveData(entries.span().subspan(0, b1sz), es.subspan(b0sz, b1sz));

            writePos.store(endPos, std::memory_order_release);
    	}

    	void read(gsl::span<T> es)
    	{
            const size_t numToRead = es.size();
            assert(canRead(numToRead));

            const auto startPos = readPos.load(std::memory_order_relaxed);
            const auto endPos = startPos + numToRead;

            const auto sz = capacity();
            const auto p0 = startPos % sz; // Actual first index
            const auto b0sz = std::min(sz - p0, numToRead); // Number of indices in the first batch
            const auto b1sz = numToRead - b0sz; // Number of indices in the second batch

            moveData(es.subspan(0, b0sz), entries.span().subspan(p0, b0sz));
            moveData(es.subspan(b0sz, b1sz), entries.span().subspan(0, b1sz));

            readPos.store(endPos, std::memory_order_release);
    	}

    private:
        // Explanation on memory alignment stuff
        // Multiple cores trying to access the same cache line will result in large performance penalty due to false sharing
        // To avoid that issue, aligning as the interference size guarantees that each of those lives on their own cache line

        Vector<T> entries;
        alignas(std::hardware_destructive_interference_size) std::atomic<size_t> readPos;
        alignas(std::hardware_destructive_interference_size) std::atomic<size_t> writePos;

        // Any sane version of C++ will ensure that; but we'll assert since the performance penalty for this being false would be enormous
        static_assert(std::atomic<size_t>::is_always_lock_free);

        constexpr void moveData(gsl::span<T> dst, gsl::span<T> src)
        {
            assert(dst.size() == src.size());
            std::move(src.data(), src.data() + src.size(), dst.data()); // Don't use gsl::span iterator as that's bounds checked and disables memcpy
        }

        constexpr void copyData(gsl::span<T> dst, gsl::span<const T> src)
        {
            assert(dst.size() == src.size());
            std::copy(src.data(), src.data() + src.size(), dst.data()); // Don't use gsl::span iterator as that's bounds checked and disables memcpy
        }
    };
}
