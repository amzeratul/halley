#pragma once

#include <gsl/span>
#include <atomic>

namespace Halley {
	// This is a lock-free, wait-free ring buffer
	// It is only thread safe for one producer and one consumer at any given time
	// Loosely based on implementation from Game Audio Programming 2
	
    template <typename T>
    class RingBuffer {
    public:
    	explicit RingBuffer(size_t capacity)
	        : numEntries(0)
    	{
            entries.resize(capacity);
    	}

    	size_t availableToRead() const
    	{
            return numEntries.load();
    	}

        size_t availableToWrite() const
        {
            return entries.size() - numEntries.load();
        }

    	bool canWrite(size_t n) const
    	{
            return numEntries.load() + n <= entries.size();
    	}

    	bool canRead(size_t n) const
    	{
            return numEntries.load() >= n;
    	}

    	bool empty() const
    	{
            return numEntries.load() == 0;
    	}
    	
        void writeOne(T e)
        {
            Expects(canWrite(1));
            entries[writePos] = std::move(e);
            writePos = (writePos + 1) % entries.size();
            ++numEntries;
        }

        void write(gsl::span<T> es)
        {
            const size_t numToWrite = size_t(es.size());
            Expects(canWrite(numToWrite));
            const size_t spaceToEnd = entries.size() - writePos;
            const size_t nToWrite1 = std::min(spaceToEnd, numToWrite);

            for (size_t i = 0; i < nToWrite1; ++i) {
                entries[writePos + i] = std::move(es[i]);
            }

            const size_t nToWrite2 = numToWrite - nToWrite1;
            for (size_t i = 0; i < nToWrite2; ++i) {
                entries[i] = std::move(es[i + nToWrite1]);
            }

            writePos = (writePos + numToWrite) % entries.size();
            numEntries.fetch_add(numToWrite);
        }

    	void write(gsl::span<const T> es)
    	{
            const size_t numToWrite = size_t(es.size());
            Expects(canWrite(numToWrite));
            const size_t spaceToEnd = entries.size() - writePos;
            const size_t nToWrite1 = std::min(spaceToEnd, numToWrite);

    		for (size_t i = 0; i < nToWrite1; ++i) {
                entries[writePos + i] = es[i];
    		}

            const size_t nToWrite2 = numToWrite - nToWrite1;
    		for (size_t i = 0; i < nToWrite2; ++i) {
                entries[i] = es[i + nToWrite1];
    		}

            writePos = (writePos + numToWrite) % entries.size();
            numEntries.fetch_add(numToWrite);
    	}

    	T readOne()
    	{
            Expects(canRead(1));
            T v = entries[readPos];
            readPos = (readPos + 1) % entries.size();
            --numEntries;
            return v;
    	}

    	void read(gsl::span<T> es)
    	{
            const size_t numToRead = size_t(es.size());
            Expects(canRead(numToRead));
            const size_t spaceToEnd = entries.size() - readPos;
            const size_t nToRead1 = std::min(spaceToEnd, numToRead);

            for (size_t i = 0; i < nToRead1; ++i) {
                es[i] = std::move(entries[readPos + i]);
            }

            const size_t nToRead2 = numToRead - nToRead1;
            for (size_t i = 0; i < nToRead2; ++i) {
                es[i + nToRead1] = std::move(entries[i]);
            }

            readPos = (readPos + numToRead) % entries.size();
            numEntries.fetch_sub(numToRead);
    	}

    private:
        size_t readPos = 0;
        size_t writePos = 0;
        std::atomic<size_t> numEntries;
        std::vector<T> entries;
    };
}
