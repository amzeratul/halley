#pragma once

#include <cstdint>
#include <gsl/gsl>
#include "utils.h"

struct XXH64_state_s;
typedef struct XXH64_state_s XXH64_state_t;

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


		class Hasher
		{
		public:
			Hasher();
			~Hasher();

			template<typename T>
			void feed(const T& data)
			{
				feedBytes(gsl::as_bytes(gsl::span<const T>(&data, 1)));
			}
			
			void feedBytes(gsl::span<const gsl::byte> bytes);

			[[nodiscard]] uint64_t digest();

		private:
			bool ready;
			XXH64_state_t* data;
		};
    };
}
