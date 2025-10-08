/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_X86_H
#define NOGG_SRC_X86_H

/*
 * This header includes the system headers needed for x86 SSE2/AVX2
 * intrinsics, and also defines macros to work around incompatibilities
 * between compilers.  Sources using these intrinsics should include
 * this header rather than directly including system headers such as
 * <xmmintrin.h> or <emmintrin.h>.
 *
 * This header includes a conditional check on the relevant #defines, so
 * users do not need their own check.
 */

#if defined(ENABLE_ASM_X86_AVX2) || defined(ENABLE_ASM_X86_SSE2)

/*************************************************************************/
/*************************************************************************/

#include <xmmintrin.h>
#include <emmintrin.h>
#ifdef ENABLE_ASM_X86_AVX2
# include <immintrin.h>
#endif

/* Most compilers allow us to cast directly between the __m128 and __m128i
 * types used to represent SSE2 register values treated as packed float and
 * raw integer respectively, but Microsoft's Visual C++ compiler requires
 * the use of an explicit (nonstandard) function call. */
#ifdef _MSC_VER
# define CAST_M128(x)   _mm_castsi128_ps((x))
# define CAST_M128I(x)  _mm_castps_si128((x))
#else
# define CAST_M128(x)   ((__m128)(x))
# define CAST_M128I(x)  ((__m128i)(x))
#endif

/*************************************************************************/
/*************************************************************************/

#endif  // ENABLE_ASM_X86_AVX2 || ENABLE_ASM_X86_SSE2
#endif  // NOGG_SRC_X86_H
