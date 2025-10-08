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
#include "src/util/open.h"

#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/******************** Buffer data type and callbacks *********************/
/*************************************************************************/

/* Parameter block passed to buffer_open(). */
typedef struct vorbis_buffer_info_t {
    const void *buffer;
    int64_t length;
} vorbis_buffer_info_t;

/* Internal callback (see comments at open_params_t.open_callback).
 * The buffer pointer is passed in the opaque parameter; the buffer length
 * will separately be stored to vorbis->data_length. */
static void *buffer_open(vorbis_t *handle, void *callback_data)
{
    vorbis_buffer_info_t *buffer_info = (vorbis_buffer_info_t *)callback_data;
    handle->buffer_data = buffer_info->buffer;
    handle->data_length = buffer_info->length;
    handle->buffer_read_pos = 0;
    return handle;
}

static int64_t buffer_length(void *opaque)
{
    vorbis_t *handle = (vorbis_t *)opaque;
    return handle->data_length;
}

static int64_t buffer_tell(void *opaque)
{
    vorbis_t *handle = (vorbis_t *)opaque;
    return handle->buffer_read_pos;
}

static void buffer_seek(void *opaque, int64_t offset)
{
    vorbis_t *handle = (vorbis_t *)opaque;
    handle->buffer_read_pos = offset;
}

static int32_t buffer_read(void *opaque, void *buffer, int32_t length)
{
    vorbis_t *handle = (vorbis_t *)opaque;
    if (length > handle->data_length - handle->buffer_read_pos) {
        length = (int32_t)(handle->data_length - handle->buffer_read_pos);
    }
    memcpy(buffer, handle->buffer_data + handle->buffer_read_pos, length);
    handle->buffer_read_pos += length;
    return length;
}

static const vorbis_callbacks_t buffer_callbacks = {
    .length = buffer_length,
    .tell   = buffer_tell,
    .seek   = buffer_seek,
    .read   = buffer_read,
};

/*************************************************************************/
/*************************** Interface routine ***************************/
/*************************************************************************/

vorbis_t *vorbis_open_buffer(
    const void *buffer, int64_t length, unsigned int options,
    vorbis_error_t *error_ret)
{
    if (!buffer || length < 0) {
        if (error_ret) {
            *error_ret = VORBIS_ERROR_INVALID_ARGUMENT;
        }
        return NULL;
    }

    vorbis_buffer_info_t buffer_info = {.buffer = buffer, .length = length};
    return open_common(
        &(open_params_t){.callbacks = &buffer_callbacks,
                         .callback_data = &buffer_info,
                         .open_callback = buffer_open,
                         .options = options,
                         .packet_mode = false},
        error_ret);
}

/*************************************************************************/
/*************************************************************************/
