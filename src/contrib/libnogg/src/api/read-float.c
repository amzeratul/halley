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
#include "src/util/decode-frame.h"

#include <string.h>


int32_t vorbis_read_float(
    vorbis_t *handle, float *buf, int32_t len, vorbis_error_t *error_ret)
{
    int32_t count = 0;
    int error = VORBIS_NO_ERROR;

    if (!buf || len < 0) {
        error = VORBIS_ERROR_INVALID_ARGUMENT;
        goto out;
    }
    if (handle->read_int16_only) {
        error = VORBIS_ERROR_INVALID_OPERATION;
        goto out;
    }

    const int channels = handle->channels;
    while (count < len) {
        if (handle->decode_buf_pos >= handle->decode_buf_len) {
            if (handle->packet_mode) {
                error = VORBIS_ERROR_STREAM_END;
                break;
            } else {
                error = decode_frame(handle, NULL, 0);
                if (error) {
                    break;
                }
            }
        }
        const int copy = min(
            len - count, handle->decode_buf_len - handle->decode_buf_pos);
        memcpy(buf, ((float *)handle->decode_buf
                     + handle->decode_buf_pos * channels),
               copy * channels * sizeof(*buf));
        buf += copy * channels;
        count += copy;
        handle->decode_buf_pos += copy;
    }

  out:
    if (error_ret) {
        *error_ret = error;
    }
    return count;
}
