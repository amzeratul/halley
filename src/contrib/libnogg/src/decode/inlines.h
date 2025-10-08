/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

/*
 * This header defines some simple inline functions used for various
 * purposes that don't fit anywhere else.  These functions are all
 * declared UNUSED to prevent warnings from compilers which complain
 * about unused static inline functions.
 */

#ifndef NOGG_SRC_DECODE_INLINES_H
#define NOGG_SRC_DECODE_INLINES_H

/*************************************************************************/
/*************************************************************************/

/**
 * extract_32:  Extract a little-endian 32-bit value from a buffer.
 *
 * [Parameters]
 *     ptr: Pointer to 32-bit value in buffer.
 * [Return value]
 *     Value read from buffer.
 */
static inline UNUSED PURE_FUNCTION uint32_t extract_32(const uint8_t *ptr)
{
    return (uint32_t)ptr[0] <<  0
         | (uint32_t)ptr[1] <<  8
         | (uint32_t)ptr[2] << 16
         | (uint32_t)ptr[3] << 24;
}

/*-----------------------------------------------------------------------*/

/**
 * extract_64:  Extract a little-endian 64-bit value from a buffer.
 *
 * [Parameters]
 *     ptr: Pointer to 64-bit value in buffer.
 * [Return value]
 *     Value read from buffer.
 */
static inline UNUSED PURE_FUNCTION uint64_t extract_64(const uint8_t *ptr)
{
    return (uint64_t)ptr[0] <<  0
         | (uint64_t)ptr[1] <<  8
         | (uint64_t)ptr[2] << 16
         | (uint64_t)ptr[3] << 24
         | (uint64_t)ptr[4] << 32
         | (uint64_t)ptr[5] << 40
         | (uint64_t)ptr[6] << 48
         | (uint64_t)ptr[7] << 56;
}

/*-----------------------------------------------------------------------*/

/**
 * bit_reverse:  Reverse the order of the bits in a 32-bit integer.
 */
static inline UNUSED CONST_FUNCTION uint32_t bit_reverse(uint32_t n)
{
#if IS_CLANG(4,0)  // GCC: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=50481
    return __builtin_bitreverse32(n);
#else
    n = ((n & UINT32_C(0xAAAAAAAA)) >>  1) | ((n & UINT32_C(0x55555555)) << 1);
    n = ((n & UINT32_C(0xCCCCCCCC)) >>  2) | ((n & UINT32_C(0x33333333)) << 2);
    n = ((n & UINT32_C(0xF0F0F0F0)) >>  4) | ((n & UINT32_C(0x0F0F0F0F)) << 4);
    n = ((n & UINT32_C(0xFF00FF00)) >>  8) | ((n & UINT32_C(0x00FF00FF)) << 8);
    return (n >> 16) | (n << 16);
#endif
}

/*-----------------------------------------------------------------------*/

/**
 * square:  Return the square of the floating-point argument.
 */
static inline UNUSED CONST_FUNCTION float square(float x) {return x*x;}

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_INLINES_H
