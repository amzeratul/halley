#pragma once

#include "halley/data_structures/vector.h"

namespace Halley {
    template <typename T>
    class RollingDataSet {
    public:
        RollingDataSet(size_t maxSize)
            : maxSize(maxSize)
        {
            data.reserve(maxSize);
        }

        void add(T v)
        {
            if (data.size() < maxSize) {
                data.push_back(std::move(v));
            } else {
                data[pos] = std::move(v);
            }
            pos = (pos + 1) % maxSize;
        }

        T getSum() const
        {
	        T a = 0;
            for (auto& v: data) {
                a += v;
            }
            return a;
        }

        T getMean() const
        {
            const auto a = getSum();
            if constexpr (std::is_integral_v<T>) {
                return (getSum() + data.size() / 2) / data.size();
            } else {
	           return getSum() / data.size();
            }
        }

        T getOldest() const
        {
            if (data.size() < maxSize) {
                return data.front();
            } else {
                return data[pos];
            }
        }

        T getLatest() const
        {
            if (data.size() < maxSize) {
                return data.back();
            } else {
                return data[(pos + maxSize - 1) % maxSize];
            }
        }

        float getFloatMean() const
        {
            return static_cast<float>(getSum()) / data.size();
        }

        size_t size() const
        {
            return data.size();
        }

        size_t getMaxSize() const
        {
            return maxSize;
        }

    private:
        Vector<T> data;
        size_t maxSize = 0;
        size_t pos = 0;
    };
}