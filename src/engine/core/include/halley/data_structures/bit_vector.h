#pragma once

#include "vector.h"

namespace Halley {
	class ConfigNode;

	class BitVector {
    public:
        BitVector() = default;
        BitVector(const ConfigNode& node);

        ConfigNode toConfigNode() const;

        size_t getSize() const
        {
	        return data.size() * 8;
        }

        void resize(size_t nBits)
        {
	        data.resize(getBytesNeeded(nBits), 0);
        }

    	void reserve(size_t nBits)
        {
	        data.reserve(getBytesNeeded(nBits));
        }

        bool getBit(size_t idx) const
        {
	        return data.at(getIndex(idx)) & getMask(idx);
        }

    	void setBit(size_t idx, bool newValue)
        {
	        uint8_t& values = data.at(getIndex(idx));
            const auto mask = getMask(idx);
            values = (values & (~mask)) | (newValue ? mask : static_cast<uint8_t>(0));
        }

    	bool toggleBit(size_t idx)
        {
            const bool newValue = !getBit(idx);
	        setBit(idx, newValue);
            return newValue;
        }

        gsl::span<uint8_t> getRawData()
        {
	        return data.span();
        }

        gsl::span<const uint8_t> getRawData() const
        {
	        return data.const_span();
        }

        Vector<uint8_t>& getVector()
        {
	        return data;
        }

        const Vector<uint8_t>& getVector() const
        {
	        return data;
        }

    private:
        Vector<uint8_t> data;

        static size_t getBytesNeeded(size_t nBits)
        {
	        return (nBits + 7) / 8;
        }

        static size_t getIndex(size_t bit)
        {
	        return bit / 8;
        }

        static uint8_t getMask(size_t bit)
        {
	        return (static_cast<uint8_t>(1) << static_cast<uint8_t>(bit % 8));
        }
    };
}
