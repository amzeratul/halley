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


int vorbis_channels(const vorbis_t *handle)
{
    return handle->channels;
}


uint32_t vorbis_rate(const vorbis_t *handle)
{
    return handle->rate;
}


int64_t vorbis_length(const vorbis_t *handle)
{
    /* stb_vorbis_stream_length_in_samples() doesn't differentiate between
     * "empty file" and "error" in the return value, so we use the error
     * flag to tell the difference. */
    (void) stb_vorbis_get_error(handle->decoder);
    const int64_t length = stb_vorbis_stream_length_in_samples(handle->decoder);
    if (stb_vorbis_get_error(handle->decoder) != VORBIS__no_error) {
        return -1;
    }
    return length;
}


int32_t vorbis_bitrate(const vorbis_t *handle)
{
    const int64_t length = vorbis_length(handle);
    if (length > 0) {
        ASSERT(handle->rate != 0);
        const int64_t bits_limit = (INT64_MAX - length/2) / handle->rate;
        if (handle->data_length > bits_limit / 8) {
            return INT32_MAX;
        }
        const int64_t bits = handle->data_length * 8;
        /* This next calculation is guaranteed not to overflow by the
         * above check. */
        const int64_t bitrate = (bits * handle->rate + length/2) / length;
        if (bitrate <= INT32_MAX) {
            return (int32_t)bitrate;
        } else {
            return INT32_MAX;
        }
    } else {  // Not a seekable stream.
        const stb_vorbis_info info = stb_vorbis_get_info(handle->decoder);
        if (info.nominal_bitrate > 0) {
            return info.nominal_bitrate;
        } else if (info.min_bitrate > 0 && info.max_bitrate > 0) {
            return (info.min_bitrate + info.max_bitrate + 1) / 2;
        } else {
            return 0;
        }
    }
}
