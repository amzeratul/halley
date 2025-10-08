/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_IO_H
#define NOGG_SRC_DECODE_IO_H

/*************************************************************************/
/*************************************************************************/

/**
 * get8:  Read a byte from the stream and return it as an 8-bit unsigned
 * integer.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Byte read, or 0 on EOF.
 */
#define get8 INTERNAL(get8)
extern uint8_t get8(stb_vorbis *handle);

/**
 * getn:  Read the given number of bytes from the stream and store them in
 * the given buffer.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     buffer: Buffer into which to read data.
 *     count: Number of bytes to read.
 * [Return value]
 *     True on success, false on EOF.
 */
#define getn INTERNAL(getn)
extern bool getn(stb_vorbis *handle, uint8_t *buffer, int count);

/**
 * skip:  Skip over the given number of bytes in the stream.  The resultant
 * offset is assumed to lie within the range [0, handle->stream_len].
 *
 * [Parameters]
 *     handle: Stream handle.
 *     count: Number of bytes to skip.
 */
#define skip INTERNAL(skip)
extern void skip(stb_vorbis *handle, int count);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_IO_H
