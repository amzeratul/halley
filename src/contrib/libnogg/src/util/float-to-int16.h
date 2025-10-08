/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_UTIL_FLOAT_TO_INT16_H
#define NOGG_SRC_UTIL_FLOAT_TO_INT16_H

/*************************************************************************/
/*************************************************************************/

/**
 * float_to_int16:  Convert floating-point data in 'src' to 16-bit integer
 * data in 'dest'.
 *
 * [Parameters]
 *     dest: Destination (int16) buffer pointer.
 *     src: Source (float) buffer pointer.
 *     count: Number of values to convert.
 */
#define float_to_int16 INTERNAL(float_to_int16)
extern void float_to_int16(int16_t *__restrict dest, const float *__restrict src,
                           int count);

/**
 * float_to_int16_interleave:  Convert multiple channels of floating-point
 * data in 'src' to interleaved 16-bit integer data in 'dest'.
 *
 * [Parameters]
 *     dest: Destination (int16) buffer pointer.
 *     src: Source (float) buffer array.
 *     channels: Number of source channels.
 *     count: Number of values to convert per channel.
 */
#define float_to_int16_interleave INTERNAL(float_to_int16_interleave)
extern void float_to_int16_interleave(int16_t *dest, float **src, int channels,
                                      int count);

/**
 * float_to_int16_interleave_2:  Convert two channels of floating-point
 * data in 'src' to interleaved 16-bit integer data in 'dest'.
 * Specialization of float_to_int16_interleave() for channels==2.
 *
 * The caller guarantees that dest, src0, and src1 are aligned
 * appropriately for CPU-specific optimized copies.
 *
 * [Parameters]
 *     dest: Destination (int16) buffer pointer.
 *     src0: Source (float) buffer array for first channel.
 *     src1: Source (float) buffer array for second channel.
 *     count: Number of values to convert.
 */
#define float_to_int16_interleave_2 INTERNAL(float_to_int16_interleave_2)
extern void float_to_int16_interleave_2(
    int16_t *__restrict dest, const float *__restrict src0,
    const float *__restrict src1, int count);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_UTIL_FLOAT_TO_INT16_H
