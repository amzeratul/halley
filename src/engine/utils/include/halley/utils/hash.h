#pragma once

#include <cstdint>
#include <gsl/gsl>
#include "utils.h"

namespace Halley {
    class Hash {
    public:
        static uint64_t hash(const Bytes& bytes);
        static uint64_t hash(gsl::span<const gsl::byte> bytes);
		
    	template <typename T>
    	static uint64_t hashValue(const T& v)
    	{
    		return hash(gsl::as_bytes(gsl::span<const T>(&v, 1)));
    	}

		static uint32_t compressTo32(uint64_t value);
    };
}
