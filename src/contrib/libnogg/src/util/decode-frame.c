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
#include "src/x86.h"

#include <string.h>

#ifdef ENABLE_ASM_ARM_NEON
# include <arm_neon.h>
#endif

/*************************************************************************/
/**************************** Helper routines ****************************/
/*************************************************************************/

/**
 * interleave:  Interleave source channels into a destination buffer.
 *
 * [Parameters]
 *     dest: Destination buffer pointer.
 *     src: Source buffer pointer array.
 *     channels: Number of channels.
 *     samples: Number of samples per channel.
 */
static void interleave(float *dest, float **src, int channels, int samples)
{
    for (int i = 0; i < samples; i++) {
        for (int c = 0; c < channels; c++) {
            *dest++ = src[c][i];
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * interleave_2:  Interleave two source channels into a destination buffer.
 * Specialization of interleave() for channels==2.
 *
 * [Parameters]
 *     dest: Destination buffer pointer.
 *     src: Source buffer pointer array.
 *     samples: Number of samples per channel.
 */
static void interleave_2(float *dest, float **src, int samples)
{
    const float *src0 = src[0];
    const float *src1 = src[1];

#if defined(ENABLE_ASM_ARM_NEON)
    for (; samples >= 4; src0 += 4, src1 += 4, dest += 8, samples -= 4) {
        float32x4x2_t data;
        data.val[0] = vld1q_f32(src0);
        data.val[1] = vld1q_f32(src1);
        vst2q_f32(dest, data);
    }
#elif defined(ENABLE_ASM_X86_AVX2)
    for (; samples >= 8; src0 += 8, src1 += 8, dest += 16, samples -= 8) {
        const __m256i data0 = _mm256_load_si256((const void *)src0);
        const __m256i data1 = _mm256_load_si256((const void *)src1);
        const __m256i sample0145 = _mm256_unpacklo_epi32(data0, data1);
        const __m256i sample2367 = _mm256_unpackhi_epi32(data0, data1);
        _mm256_store_si256((void *)(dest+0), _mm256_permute2x128_si256(
                               sample0145, sample2367, 0x20));
        _mm256_store_si256((void *)(dest+8), _mm256_permute2x128_si256(
                               sample0145, sample2367, 0x31));
    }
#elif defined(ENABLE_ASM_X86_SSE2)
    for (; samples >= 4; src0 += 4, src1 += 4, dest += 8, samples -= 4) {
        const __m128i data0 = _mm_load_si128((const void *)src0);
        const __m128i data1 = _mm_load_si128((const void *)src1);
        _mm_store_si128((void *)(dest+0), _mm_unpacklo_epi32(data0, data1));
        _mm_store_si128((void *)(dest+4), _mm_unpackhi_epi32(data0, data1));
    }
#endif

    for (int i = 0; i < samples; i++) {
        dest[i*2+0] = src0[i];
        dest[i*2+1] = src1[i];
    }
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

vorbis_error_t decode_frame(vorbis_t *handle, const void *packet,
                            int32_t packet_len)
{
    handle->frame_pos += handle->decode_buf_len;
    handle->decode_buf_pos = 0;
    handle->decode_buf_len = 0;

    (void) stb_vorbis_get_error(handle->decoder);  // Clear any pending error.
    float **outputs;
    int samples = 0;
    if (handle->packet_mode) {
        ASSERT(packet != NULL);
        ASSERT(packet_len > 0);
        (void) stb_vorbis_decode_packet_float(handle->decoder, packet,
                                              packet_len, &outputs, &samples);
    } else {
        do {
            stb_vorbis_reset_eof(handle->decoder);
            if (!stb_vorbis_get_frame_float(handle->decoder,
                                            &outputs, &samples)) {
                break;
            }
            handle->frame_pos = stb_vorbis_tell_pcm(handle->decoder) - samples;
        } while (samples == 0);
    }

    if (samples > 0) {
        const int channels = handle->channels;
        if (handle->read_int16_only) {
            int16_t *decode_buf = handle->decode_buf;
            if (channels == 1) {
                float_to_int16(decode_buf, outputs[0], samples);
            } else if (channels == 2) {
                float_to_int16_interleave_2(decode_buf, outputs[0],
                                            outputs[1], samples);
            } else {
                float_to_int16_interleave(decode_buf, outputs, channels,
                                          samples);
            }
        } else {
            float *decode_buf = handle->decode_buf;
            if (channels == 1) {
                memcpy(decode_buf, outputs[0], sizeof(*decode_buf) * samples);
            } else if (channels == 2) {
                interleave_2(decode_buf, outputs, samples);
            } else {
                interleave(decode_buf, outputs, channels, samples);
            }
        }
    }
    handle->decode_buf_len = samples;

    const STBVorbisError stb_error = stb_vorbis_get_error(handle->decoder);
    if (samples == 0 && stb_error == VORBIS__no_error) {
        return VORBIS_ERROR_STREAM_END;
    } else if (stb_error == VORBIS_invalid_packet
            || stb_error == VORBIS_continued_packet_flag_invalid
            || stb_error == VORBIS_wrong_page_number) {
        return VORBIS_ERROR_DECODE_RECOVERED;
    } else if (stb_error != VORBIS__no_error) {
        return VORBIS_ERROR_DECODE_FAILED;
    } else {
        return VORBIS_NO_ERROR;
    }
}

/*************************************************************************/
/*************************************************************************/
