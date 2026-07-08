#pragma once

#include "halley/data_structures/vector.h"

namespace Halley {
    template <typename T>
    class RollingDataSet {
    public:
        RollingDataSet(size_t maxSize, bool keepOrder = true)
            : maxSize(maxSize)
			, keepOrder(keepOrder)
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

        void clear()
        {
            data.clear();
            pos = 0;
        }

        T getSum() const
        {
	        T a = {};
            for (auto& v: data) {
                a += v;
            }
            return a;
        }

        T getMean() const
        {
            if constexpr (std::is_integral_v<T>) {
                return (getSum() + data.size() / 2) / data.size();
            } else {
            	return getSum() / data.size();
            }
        }

    	T getMedian()
        {
            if (data.empty()) {
                return {};
            }

            if (keepOrder) {
                auto data2 = data;
                std::sort(data2.begin(), data2.end());
                return getMedianOf(data2);
            } else {
                std::sort(data.begin(), data.end());
                return getMedianOf(data);
            }
        }

    	T getMedian() const
        {
            if (data.empty()) {
                return {};
            }

            auto data2 = data;
            std::sort(data2.begin(), data2.end());
            return getMedianOf(data2);
        }

        T getOldest() const
        {
            if (!keepOrder) {
                throw Exception("Invalid operation: sorted RollingDataSet doesn't keep order", HalleyExceptions::Utils);
            }
            if (data.size() < maxSize) {
                return data.front();
            } else {
                return data[pos];
            }
        }

        T getLatest() const
        {
            if (!keepOrder) {
                throw Exception("Invalid operation: sorted RollingDataSet doesn't keep order", HalleyExceptions::Utils);
            }
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
        bool keepOrder = false;

        static T getMedianOf(const Vector<T>& d)
        {
	        if (d.size() % 2 == 0) {
                return (d[d.size() / 2] + d[d.size() / 2 + 1]) / T(2);
            } else {
                return d[d.size() / 2];
            }
        }
    };
}