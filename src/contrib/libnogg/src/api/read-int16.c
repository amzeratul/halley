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
#include "src/util/float-to-int16.h"

#include <string.h>


int32_t vorbis_read_int16(
    vorbis_t *handle, int16_t *buf, int32_t len, vorbis_error_t *error_ret)
{
    int32_t count = 0;
    int error = VORBIS_NO_ERROR;

    if (!buf || len < 0) {
        error = VORBIS_ERROR_INVALID_ARGUMENT;
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
        if (handle->read_int16_only) {
            memcpy(buf, ((int16_t *)handle->decode_buf
                         + handle->decode_buf_pos * channels),
                   copy * channels * sizeof(*buf));
        } else {
            const float *src =
                (float *)handle->decode_buf + handle->decode_buf_pos * channels;
            float_to_int16(buf, src, copy * channels);
        }
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
