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
#include "src/util/memory.h"

#include <stdlib.h>


void vorbis_close(vorbis_t *handle)
{
    if (!handle) {
        return;
    }

    mem_free(handle, handle->decode_buf);
    stb_vorbis_close(handle->decoder);
    if (handle->callbacks.close) {
        (*handle->callbacks.close)(handle->callback_data);
    }
    if (handle->callbacks.free) {
        (*handle->callbacks.free)(handle->callback_data, handle);
    } else {
        free(handle);
    }
}
