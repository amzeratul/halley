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
#include "src/decode/common.h"
#include "src/decode/io.h"

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

uint8_t get8(stb_vorbis *handle)
{
    uint8_t byte;
    if (UNLIKELY((*handle->read_callback)(handle->io_opaque, &byte, 1) != 1)) {
        handle->eof = true;
        return 0;
    }
    return byte;
}

/*-----------------------------------------------------------------------*/

bool getn(stb_vorbis *handle, uint8_t *buffer, int count)
{
    if (UNLIKELY((*handle->read_callback)(handle->io_opaque,
                                          buffer, count) != count)) {
        handle->eof = true;
        return false;
    }
    return true;
}

/*-----------------------------------------------------------------------*/

void skip(stb_vorbis *handle, int count)
{
    if (handle->stream_len >= 0) {
        const int64_t current = (*handle->tell_callback)(handle->io_opaque);
        if (count > handle->stream_len - current) {
            count = (int)(handle->stream_len - current);
            handle->eof = true;
        }
        (*handle->seek_callback)(handle->io_opaque, current + count);
    } else {
        uint8_t skip_buf[256];
        while (count > 0) {
            const int skip_count = min(count, (int)sizeof(skip_buf));
            /* We ignore the result of getn() here because this function
             * is never called in a context where it can read past the end
             * of the stream, and even if a read fails for other reasons,
             * ignoring the error here is harmless because handle->eof will
             * be set anyway (and we expect the caller to check handle->eof
             * since we do not return success or failure ourselves). */
            (void) getn(handle, skip_buf, skip_count);
            count -= skip_count;
        }
    }
}

/*************************************************************************/
/*************************************************************************/
