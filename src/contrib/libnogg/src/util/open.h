/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_UTIL_OPEN_H
#define NOGG_SRC_UTIL_OPEN_H

/*************************************************************************/
/*************************************************************************/

/* Parameter structure passed to open_common(). */
typedef struct open_params_t {
    /* Callback set.  The callbacks will be copied into the decoder handle,
     * so this pointer does not need to remain live beyond the open call. */
    const vorbis_callbacks_t *callbacks;
    /* Opaque pointer to pass to callbacks. */
    void *callback_data;
    /* Stream initialization callback.  This is used to resolve a
     * chicken-and-egg problem with the buffer-based API: the stream state
     * must be available before open_common() returns, but the buffer data
     * is stored within the vorbis_t structure itself (see comments at
     * vorbis_t.buffer_data), which naturally cannot be initialized before
     * the structure has been allocated.  If this pointer is non-NULL, it
     * is called with the newly allocated handle and the callback_data
     * pointer provided above; the handle's callback_data pointer is then
     * set to the return value from this function. */
    void *(*open_callback)(vorbis_t *handle, void *callback_data);
    /* Decoder options (VORBIS_OPTION_*). */
    unsigned int options;
    /* Create a packet-mode decoder (true) or standard Ogg parser (false)? */
    bool packet_mode;
    /* Vorbis ID and setup packets, for packet mode decoders.  Ignored for
     * standard decoders. */
    const void *id_packet;
    int32_t id_packet_len;
    const void *setup_packet;
    int32_t setup_packet_len;
} open_params_t;

/**
 * open_common:  Common logic for creating a new decoder instance.
 *
 * [Parameters]
 *     params: Parameters for the open operation.
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (always VORBIS_NO_ERROR on success).  May be NULL if
 *         the error code is not needed.
 * [Return value]
 *     Newly-created handle, or NULL on error.
 */
#define open_common INTERNAL(open_common)
extern vorbis_t *open_common(const open_params_t *params,
                             vorbis_error_t *error_ret);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_UTIL_OPEN_H
