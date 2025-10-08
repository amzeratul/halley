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

#include <stddef.h>


int vorbis_submit_packet(vorbis_t *handle, const void *packet,
                         int32_t packet_len, vorbis_error_t *error_ret)
{
    vorbis_error_t error;

    if (!packet || packet_len <= 0) {
        error = VORBIS_ERROR_INVALID_ARGUMENT;
        goto exit;
    }
    if (!handle->packet_mode) {
        error = VORBIS_ERROR_INVALID_OPERATION;
        goto exit;
    }
    if (handle->decode_buf_pos < handle->decode_buf_len) {
        error = VORBIS_ERROR_INVALID_OPERATION;
        goto exit;
    }
    error = decode_frame(handle, packet, packet_len);
    if (error == VORBIS_ERROR_STREAM_END) {
        /* The packet had no audio data (e.g., the first packet in the
         * stream).  This is not an error for our purposes. */
        error = VORBIS_NO_ERROR;
    }

  exit:
    if (error_ret) {
        *error_ret = error;
    }
    return error == VORBIS_NO_ERROR;
}
