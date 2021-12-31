#pragma once

#include <array>
#include <numeric>

namespace Halley {
    template <typename T, size_t nSamples = 30>
    class Averaging {
    public:
        Averaging() {
            samples[0] = {};
        }

        void pushValue(T value) {
        	if constexpr (!std::is_floating_point_v<T>) {
            	totalValue -= samples[idx];
		        totalValue += value;
            }
            samples[idx] = value;

            idx = (idx + 1) % static_cast<uint32_t>(nSamples);
            nValues = std::min(nValues + 1, static_cast<uint32_t>(nSamples));
        }

        T getAverage() const {
            return computeAverage();
        }

        T getLatest() const {
            return samples[idx];
        }

    private:
        std::array<T, nSamples> samples;
        uint32_t idx = 0;
        uint32_t nValues = 0;
        T totalValue = {};

        T computeAverage() const {
            if (nValues > 0) {
                if constexpr (std::is_integral_v<T>) {
                    return (totalValue + (static_cast<T>(nValues) / 2)) / static_cast<T>(nValues);
                } else if constexpr (std::is_floating_point_v<T>) {
                	std::accumulate(samples.begin(), samples.begin() + nValues) / static_cast<T>(nValues);
                } else {
                    return totalValue / static_cast<T>(nValues);
                }
            } else {
                return 0;
            }
        }
    };
	
    template <typename T>
    class AveragingLatched {
    public:
    	AveragingLatched(uint32_t nSamples = 30)
    		: nSamples(nSamples)
    	{}
    	
        bool pushValue(T value) {
        	latest = value;
	        totalValue += value;
            ++nValues;
        	
        	if (nValues == nSamples) {
        		if constexpr (std::is_integral_v<T>) {
                    latched = (totalValue + (static_cast<T>(nSamples) / 2)) / static_cast<T>(nSamples);
                } else {
                    latched = totalValue / static_cast<T>(nSamples);
                }
        		totalValue = 0;
        		nValues = 0;
        		return true;
            }
        	return false;
        }

        T getAverage() const {
    		return latched;
        }

        T getLatest() const {
            return latest;
        }

    private:
        uint32_t nValues = 0;
    	uint32_t nSamples;
        T totalValue = {};
    	T latched = {};
    	T latest = {};
    };
}
