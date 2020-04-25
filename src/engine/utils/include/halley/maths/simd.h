#pragma once

#if defined(_M_X64) || defined(__x86_64__)
	#define HAS_SSE
	#if !defined(__linux__)
		#define HAS_AVX
	#endif
#endif

#if defined(_M_IX86) || defined(__i386)
	// Might not be available, but do we really care about such old processors?
	#define HAS_SSE
#endif

#ifdef HAS_SSE
	#include <xmmintrin.h> // SSE
	#include <emmintrin.h> // SSE2
	#include <pmmintrin.h> // SSE3
	//#include <tmmintrin.h> // SSSE3
	//#include <smmintrin.h> // SSE4.1
	//#include <nmmintrin.h> // SSE4.1
#endif

#ifdef HAS_AVS
	#include <immintrin.h>
#endif

namespace Halley {
    class SIMDVec4 {
    public:
		SIMDVec4() = default;
		SIMDVec4(const SIMDVec4& other) = default;
		SIMDVec4(SIMDVec4&& other) noexcept = default;

		SIMDVec4& operator=(const SIMDVec4& other) = default;
		SIMDVec4& operator=(SIMDVec4&& other) noexcept = default;

		static inline SIMDVec4 loadZero()
		{
#if defined(HAS_SSE)
			return(_mm_setzero_ps());
#else
			return SIMDVec4(0, 0, 0, 0);
#endif
		}

        static inline SIMDVec4 loadAligned(const float* src)
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_load_ps(src));
#else
			SIMDVec4 result;
			memcpy(&result.x, src, sizeof(float) * 4);
			return result;
#endif
        }

		static inline SIMDVec4 loadUnaligned(const float* src)
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_loadu_ps(src));
#else
			SIMDVec4 result;
			memcpy(&result.x, src, sizeof(float) * 4);
			return result;
#endif
        }

		static inline SIMDVec4 loadSingleValue(float src)
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_load1_ps(&src));
#else
			return SIMDVec4(src, src, src, src);
#endif
        }

		inline void storeAligned(float* dst) const
        {
#if defined(HAS_SSE)
			_mm_store_ps(dst, x);
#else
			memcpy(dst, &x, sizeof(float) * 4);
#endif
        }

		inline void storeUnaligned(float* dst) const
        {
#if defined(HAS_SSE)
			_mm_storeu_ps(dst, x);
#else
			memcpy(dst, &x, sizeof(float) * 4);
#endif
        }

		inline SIMDVec4 operator+(const SIMDVec4& other) const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_add_ps(x, other.x));
#else
			return SIMDVec4(x[0] + other.x[0], x[1] + other.x[1], x[2] + other.x[2], x[3] + other.x[3]);
#endif
        }

		inline SIMDVec4 operator-(const SIMDVec4& other) const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_sub_ps(x, other.x));
#else
			return SIMDVec4(x[0] - other.x[0], x[1] - other.x[1], x[2] - other.x[2], x[3] - other.x[3]);
#endif
        }

		inline SIMDVec4 operator*(const SIMDVec4& other) const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_mul_ps(x, other.x));
#else
			return SIMDVec4(x[0] * other.x[0], x[1] * other.x[1], x[2] * other.x[2], x[3] * other.x[3]);
#endif
        }

		inline SIMDVec4 operator/(const SIMDVec4& other) const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_div_ps(x, other.x));
#else
			return SIMDVec4(x[0] / other.x[0], x[1] / other.x[1], x[2] / other.x[2], x[3] / other.x[3]);
#endif
        }

		inline SIMDVec4 min(const SIMDVec4& other) const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_min_ps(x, other.x));
#else
			return SIMDVec4(std::min(x[0], other.x[0]), std::min(x[1], other.x[1]), std::min(x[2], other.x[2]), std::min(x[3], other.x[3]));
#endif
        }

		inline SIMDVec4 max(const SIMDVec4& other) const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_max_ps(x, other.x));
#else
			return SIMDVec4(std::max(x[0], other.x[0]), std::max(x[1], other.x[1]), std::max(x[2], other.x[2]), std::max(x[3], other.x[3]));
#endif
        }

		inline SIMDVec4 sqrt() const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_sqrt_ps(x));
#else
			return SIMDVec4(std::sqrt(x[0]), std::sqrt(x[1]), std::sqrt(x[2]), std::sqrt(x[3]));
#endif
        }

		inline SIMDVec4 reciprocal() const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_rcp_ps(x));
#else
			return SIMDVec4(1.0f / x[0], 1.0f / x[1], 1.0f / x[2], 1.0f / x[3]);
#endif
        }

		inline SIMDVec4 reciprocalSqrt() const
        {
#if defined(HAS_SSE)
			return SIMDVec4(_mm_rsqrt_ps(x));
#else
			return SIMDVec4(1.0f / std::sqrt(x[0]), 1.0f / std::sqrt(x[1]), 1.0f / std::sqrt(x[2]), 1.0f / std::sqrt(x[3]));
#endif
        }

		// Returns a[0] + a[1], a[2] + a[3], b[0] + b[1], b[2] + b[3]
		static inline SIMDVec4 horizontalAdd(SIMDVec4 a, SIMDVec4 b)
		{
#if defined(HAS_SSE)
			return _mm_hadd_ps(a.x, b.x);
#else
			return SIMDVec4(a.x[0] + a.x[1], a.x[2] + a.x[3], b.x[0] + b.x[1], b.x[2] + b.x[3]);
#endif			
		}

		static inline void transpose(SIMDVec4& a, SIMDVec4& b, SIMDVec4& c, SIMDVec4& d)
		{
#if defined(HAS_SSE)
			_MM_TRANSPOSE4_PS(a.x, b.x, c.x, d.x);
#else
			std::swap(a.x[1], b.x[0]);
			std::swap(a.x[2], c.x[0]);
			std::swap(a.x[3], d.x[0]);
			std::swap(b.x[2], c.x[1]);
			std::swap(b.x[3], d.x[1]);
			std::swap(c.x[3], d.x[2]);
#endif
		}

    private:
#if defined(HAS_SSE)
        __m128 x;

		SIMDVec4(__m128 x) : x(x) {}
#else
        alignas(16) float x[4];

		SIMDVec4(float a, float b, float c, float d)
		{
			x[0] = a;
			x[1] = b;
			x[2] = c;
			x[3] = d;
		}
#endif
    };
}
