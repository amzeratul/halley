#pragma once

#include <cstdint>

namespace Halley {
    class MT199937AR {
    public:
		void init_genrand(uint32_t s);
		void init_by_array(uint32_t* init_key, size_t key_length);
		uint32_t genrand_int32(void);
		int32_t genrand_int31(void);
		double genrand_real1(void); // [0, 1]
		double genrand_real2(void); // [0, 1)
		double genrand_real3(void); // (0, 1)
		double genrand_res53(void); // [0,1) with 53-bit resolution

    private:
		constexpr static int mtVectorLength = 624;

    	uint32_t mt[mtVectorLength]; /* the array for the state vector  */
		int mti = mtVectorLength + 1; /* mti==N+1 means mt[N] is not initialized */
    };
}