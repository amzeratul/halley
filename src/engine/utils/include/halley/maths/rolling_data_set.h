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

        T getMean() const
        {
            T a = 0;
            for (auto& v: data) {
                a += v;
            }
            return (a + data.size() / 2) / data.size();
        }

        float getFloatMean() const
        {
            float a = 0;
            for (auto& v: data) {
                a += v;
            }
            return a / static_cast<float>(data.size());
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
        size_t maxSize;
        size_t pos;
    };
}