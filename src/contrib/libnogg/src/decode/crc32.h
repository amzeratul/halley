/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_CRC32_H
#define NOGG_SRC_DECODE_CRC32_H

/*************************************************************************/
/*************************************************************************/

/* CRC32 lookup table.  For internal use only. */
extern uint32_t crc_table[256];

/**
 * crc32_init:  Initialize the CRC32 lookup table.  This function may be
 * safely called from multiple threads.
 */
#define crc32_init INTERNAL(crc32_init)
extern void crc32_init(void);

/**
 * crc32_update:  Update a CRC32 value for a byte of input and return the
 * updated value.
 *
 * [Parameters]
 *     crc: Current CRC32 value.
 *     byte: New input byte.
 * [Return value]
 *     New CRC32 value.
 */
static inline UNUSED CONST_FUNCTION uint32_t crc32_update(
    uint32_t crc, uint8_t byte)
{
    return (crc << 8) ^ crc_table[byte ^ (crc >> 24)];
}

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_CRC32_H
