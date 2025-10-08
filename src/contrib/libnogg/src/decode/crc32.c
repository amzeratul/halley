/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#include "include/nogg.h"
#include "src/common.h"
#include "src/decode/crc32.h"

/*************************************************************************/
/*************************************************************************/

#define CRC32_POLY  0x04c11db7  // From the Vorbis specification.

uint32_t crc_table[256];

/*-----------------------------------------------------------------------*/

void crc32_init(void)
{
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t s = i << 24;
        for (int j = 0; j < 8; j++) {
            s = (s << 1) ^ (s >= (UINT32_C(1)<<31) ? CRC32_POLY : 0);
        }
        crc_table[i] = s;
    }
}

/*************************************************************************/
/*************************************************************************/
