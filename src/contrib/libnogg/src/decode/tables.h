/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_TABLES_H
#define NOGG_SRC_DECODE_TABLES_H

#include "include/nogg.h"
#include "src/common.h"

/*************************************************************************/
/*************************************************************************/

/* These tables are only defined if the library was built with the
 * USE_LOOKUP_TABLES option. */

#ifdef USE_LOOKUP_TABLES

/* Tables for setup.c:init_blocksize(). */
#define table_A INTERNAL(table_A)
extern const float * const table_A[8];
#define table_B INTERNAL(table_B)
extern const float * const table_B[8];
#define table_C INTERNAL(table_C)
extern const float * const table_C[8];
#define table_C INTERNAL(table_C)
extern const float * const table_C[8];
#define table_bitrev INTERNAL(table_bitrev)
extern const uint16_t * const table_bitrev[8];
#define table_weights INTERNAL(table_weights)
extern const float * const table_weights[8];

#endif  // USE_LOOKUP_TABLES

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_TABLES_H
