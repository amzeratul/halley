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
#include "src/util/memory.h"

#include <stdlib.h>

/*************************************************************************/
/**************************** Local routines *****************************/
/*************************************************************************/

#ifdef ENABLE_ASM_X86_AVX2

/**
 * x86_cpuid:  Return the specified data from the x86 CPUID instruction.
 *
 * [Parameters]
 *     eax: EAX (primary selector) value for the CPUID instruction.
 *     ecx: ECX (secondary selector) value for the CPUID instruction.
 *     index: Index of register to return (0=EAX, 1=EBX, 2=ECX, 3=EDX).
 */
static inline uint32_t x86_cpuid(uint32_t eax, uint32_t ecx, int index)
{
    if (index == 0) {
        uint32_t result;
        __asm__("cpuid" : "=a" (result) : "a" (eax), "c" (ecx));
        return result;
    } else if (index == 1) {
        uint32_t result;
        __asm__("cpuid" : "=b" (result) : "a" (eax), "c" (ecx));
        return result;
    } else if (index == 2) {
        uint32_t result;
        __asm__("cpuid" : "=c" (result) : "a" (eax), "c" (ecx));
        return result;
    } else if (index == 3) {
        uint32_t result;
        __asm__("cpuid" : "=d" (result) : "a" (eax), "c" (ecx));
        return result;
    } else {
        ASSERT(!"invalid index");
    }
}

#endif  // ENABLE_ASM_X86_AVX2

/*************************************************************************/
/*************************** Interface routine ***************************/
/*************************************************************************/

vorbis_t *open_common(const open_params_t *params, vorbis_error_t *error_ret)
{
    ASSERT(params != NULL);
    ASSERT(params->callbacks != NULL);

    vorbis_t *handle = NULL;
    vorbis_error_t error = VORBIS_NO_ERROR;

    /* Validate the parameters. */
    if ((params->callbacks->malloc != NULL) != (params->callbacks->free != NULL)) {
        error = VORBIS_ERROR_INVALID_ARGUMENT;
        goto exit;
    }
    if (params->packet_mode) {
        if (!params->id_packet || params->id_packet_len <= 0
         || !params->setup_packet || params->setup_packet_len <= 0) {
            error = VORBIS_ERROR_INVALID_ARGUMENT;
            goto exit;
        }
    } else {
        if (!params->callbacks->read) {
            error = VORBIS_ERROR_INVALID_ARGUMENT;
            goto exit;
        }
        if (params->callbacks->length
         && (!params->callbacks->tell || !params->callbacks->seek)) {
            error = VORBIS_ERROR_INVALID_ARGUMENT;
            goto exit;
        }
    }

    /* Perform CPU runtime checks. */
#ifdef ENABLE_ASM_X86_AVX2
    if (!(x86_cpuid(7, 0, 1) & (1u << 5))) {  // AVX2
        error = VORBIS_ERROR_NO_CPU_SUPPORT;
        goto exit;
    }
    /* All current CPUs with AVX2 also support FMA, so this should be a
     * no-op, but play it safe. */
    if (!(x86_cpuid(1, 0, 2) & (1u << 12))) {  // FMA (FMA3)
        error = VORBIS_ERROR_NO_CPU_SUPPORT;
        goto exit;
    }
#endif

    /* Allocate and initialize a handle structure. */
    if (params->callbacks->malloc) {
        ASSERT(!params->open_callback);
        handle = (*params->callbacks->malloc)(
            params->callback_data, sizeof(*handle), 0);
    } else {
        handle = malloc(sizeof(*handle));
    }
    if (!handle) {
        error = VORBIS_ERROR_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    handle->packet_mode = params->packet_mode;
    handle->read_int16_only =
        ((params->options & VORBIS_OPTION_READ_INT16_ONLY) != 0);
    handle->callbacks = *params->callbacks;
    if (handle->packet_mode) {
        handle->callbacks.length = NULL;
        handle->callbacks.tell = NULL;
        handle->callbacks.seek = NULL;
        handle->callbacks.read = NULL;
        handle->callbacks.close = NULL;
    }
    if (params->open_callback) {
        handle->callback_data =
            (*params->open_callback)(handle, params->callback_data);
    } else {
        handle->callback_data = params->callback_data;
    }
    if (handle->callbacks.length) {
        handle->data_length =
            (*handle->callbacks.length)(handle->callback_data);
    } else {
        handle->data_length = -1;
    }
    handle->read_error_flag = 0;
    handle->eos_flag = 0;
    handle->frame_pos = 0;
    handle->decode_buf_len = 0;
    handle->decode_buf_pos = 0;

    /* Create an stb_vorbis handle for the stream. */
    int stb_error;
    if (params->packet_mode) {
        handle->decoder = stb_vorbis_open_packet(
            handle, params->id_packet, params->id_packet_len,
            params->setup_packet, params->setup_packet_len,
            params->options, &stb_error);
    } else {
        handle->decoder = stb_vorbis_open_callbacks(
            handle->callbacks.read, handle->callbacks.seek,
            handle->callbacks.tell, handle->callback_data,
            handle, handle->data_length, params->options, &stb_error);
    }
    if (!handle->decoder) {
        if (stb_error == VORBIS_outofmem) {
            error = VORBIS_ERROR_INSUFFICIENT_RESOURCES;
        } else if (stb_error == VORBIS_unexpected_eof
                || stb_error == VORBIS_reached_eof
                || stb_error == VORBIS_missing_capture_pattern
                || stb_error == VORBIS_invalid_stream_structure_version
                || stb_error == VORBIS_invalid_first_page
                || stb_error == VORBIS_invalid_stream) {
            error = VORBIS_ERROR_STREAM_INVALID;
        } else {
            error = VORBIS_ERROR_DECODE_SETUP_FAILED;
        }
        goto error_free_handle;
    }

    /* Save the audio parameters. */
    stb_vorbis_info info = stb_vorbis_get_info(handle->decoder);
    handle->channels = info.channels;
    handle->rate = info.sample_rate;

    /* Allocate a decoding buffer based on the maximum decoded frame size.
     * We align this to a 64-byte boundary to help optimizations which
     * require aligned data. */
    const int sample_size = (handle->read_int16_only ? 2 : 4);
    const int32_t decode_buf_size =
        sample_size * handle->channels * info.max_frame_size;
    handle->decode_buf = mem_alloc(handle, decode_buf_size, 64);
    if (!handle->decode_buf) {
        error = VORBIS_ERROR_INSUFFICIENT_RESOURCES;
        goto error_close_decoder;
    }

  exit:
    if (error_ret) {
        *error_ret = error;
    }
    return handle;

  error_close_decoder:
    stb_vorbis_close(handle->decoder);
  error_free_handle:
    if (params->callbacks->free) {
        ASSERT(!params->open_callback);
        (*params->callbacks->free)(params->callback_data, handle);
    } else {
        free(handle);
    }
    handle = NULL;
    goto exit;
}

/*************************************************************************/
/*************************************************************************/
