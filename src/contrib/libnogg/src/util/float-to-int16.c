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
#include "src/util/float-to-int16.h"
#include "src/x86.h"

#include <math.h>

#ifdef ENABLE_ASM_ARM_NEON
# include <arm_neon.h>
#endif

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

void float_to_int16(int16_t *__restrict dest, const float *__restrict src,
                    int count)
{
#if defined(ENABLE_ASM_ARM_NEON)
    const float32x4_t k32767 = {32767, 32767, 32767, 32767};
    const float32x4_t k0_5 = {0.5, 0.5, 0.5, 0.5};
    const uint32x4_t k7FFFFFFF = {0x7FFFFFFF, 0x7FFFFFFF,
                                  0x7FFFFFFF, 0x7FFFFFFF};
    for (; count >= 8; src += 8, dest += 8, count -= 8) {
        const float32x4_t in0 = vld1q_f32(src);
        const float32x4_t in1 = vld1q_f32(src + 4);
        const float32x4_t in0_scaled = vmulq_f32(in0, k32767);
        const float32x4_t in1_scaled = vmulq_f32(in1, k32767);
        const uint32x4_t in0_abs = vandq_u32((uint32x4_t)in0_scaled,
                                             k7FFFFFFF);
        const uint32x4_t in1_abs = vandq_u32((uint32x4_t)in1_scaled,
                                             k7FFFFFFF);
        const uint32x4_t in0_sign = vbicq_u32((uint32x4_t)in0_scaled,
                                              k7FFFFFFF);
        const uint32x4_t in1_sign = vbicq_u32((uint32x4_t)in1_scaled,
                                              k7FFFFFFF);
        const uint32x4_t in0_sat = vminq_u32(in0_abs, (uint32x4_t)k32767);
        const uint32x4_t in1_sat = vminq_u32(in1_abs, (uint32x4_t)k32767);
        /* Note that we have to add 0.5 to the absolute values here because
         * vcvt always rounds toward zero. */
        const float32x4_t in0_adj = vaddq_f32((float32x4_t)in0_sat, k0_5);
        const float32x4_t in1_adj = vaddq_f32((float32x4_t)in1_sat, k0_5);
        const float32x4_t out0 = (float32x4_t)vorrq_u32((uint32x4_t)in0_adj,
                                                        in0_sign);
        const float32x4_t out1 = (float32x4_t)vorrq_u32((uint32x4_t)in1_adj,
                                                        in1_sign);
        const int32x4_t out0_32 = vcvtq_s32_f32(out0);
        const int32x4_t out1_32 = vcvtq_s32_f32(out1);
#if defined(__GNUC__) && !defined(__clang__) && !defined(__aarch64__)
        /* GCC doesn't seem to be smart enough to put out0_16 and out1_16
         * in paired registers (and it ignores any explicit registers we
         * specify with asm("REG")), so do it manually. */
        int16x8_t out_16;
        __asm__(
            "vmovn.i32 %e0, %q1\n"
            "vmovn.i32 %f0, %q2\n"
            : "=&w" (out_16)
            : "w" (out0_32), "w" (out1_32)
        );
#else
        const int16x4_t out0_16 = vmovn_s32(out0_32);
        const int16x4_t out1_16 = vmovn_s32(out1_32);
        const int16x8_t out_16 = vcombine_s16(out0_16, out1_16);
#endif
        vst1q_s16(dest, out_16);
    }

#elif defined(ENABLE_ASM_X86_AVX2)

    const uint32_t saved_mxcsr = _mm_getcsr();
    uint32_t mxcsr = saved_mxcsr;
    mxcsr &= ~(3<<13);  // RC (00 = round to nearest)
    mxcsr |= 1<<7;      // EM_INVALID
    _mm_setcsr(mxcsr);

    const __m256 k32767 = _mm256_set1_ps(32767.0f);
    const __m256 k7FFFFFFF = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
    const __m256 k80000000 = _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000));

    if ((((uintptr_t)src | (uintptr_t)dest) & 31) == 0) {
        for (; count >= 16; src += 16, dest += 16, count -= 16) {
            const __m256 in0 = _mm256_load_ps(src);
            const __m256 in1 = _mm256_load_ps(src+8);
            const __m256 in0_scaled = _mm256_mul_ps(in0, k32767);
            const __m256 in1_scaled = _mm256_mul_ps(in1, k32767);
            const __m256 in0_abs = _mm256_and_ps(in0_scaled, k7FFFFFFF);
            const __m256 in1_abs = _mm256_and_ps(in1_scaled, k7FFFFFFF);
            const __m256 in0_sign = _mm256_and_ps(in0_scaled, k80000000);
            const __m256 in1_sign = _mm256_and_ps(in1_scaled, k80000000);
            const __m256 in0_sat = _mm256_min_ps(in0_abs, k32767);
            const __m256 in1_sat = _mm256_min_ps(in1_abs, k32767);
            const __m256 out0 = _mm256_or_ps(in0_sat, in0_sign);
            const __m256 out1 = _mm256_or_ps(in1_sat, in1_sign);
            const __m256i out0_32 = _mm256_cvtps_epi32(out0);
            const __m256i out1_32 = _mm256_cvtps_epi32(out1);
            const __m256i out_16_0213 = _mm256_packs_epi32(out0_32, out1_32);
            const __m256i out_16 =
                _mm256_permute4x64_epi64(out_16_0213, _MM_SHUFFLE(3,1,2,0));
            _mm256_store_si256((void *)dest, out_16);
        }
    } else {
        for (; count >= 16; src += 16, dest += 16, count -= 16) {
            const __m256 in0 = _mm256_loadu_ps(src);
            const __m256 in1 = _mm256_loadu_ps(src+8);
            const __m256 in0_scaled = _mm256_mul_ps(in0, k32767);
            const __m256 in1_scaled = _mm256_mul_ps(in1, k32767);
            const __m256 in0_abs = _mm256_and_ps(in0_scaled, k7FFFFFFF);
            const __m256 in1_abs = _mm256_and_ps(in1_scaled, k7FFFFFFF);
            const __m256 in0_sign = _mm256_and_ps(in0_scaled, k80000000);
            const __m256 in1_sign = _mm256_and_ps(in1_scaled, k80000000);
            const __m256 in0_sat = _mm256_min_ps(in0_abs, k32767);
            const __m256 in1_sat = _mm256_min_ps(in1_abs, k32767);
            const __m256 out0 = _mm256_or_ps(in0_sat, in0_sign);
            const __m256 out1 = _mm256_or_ps(in1_sat, in1_sign);
            const __m256i out0_32 = _mm256_cvtps_epi32(out0);
            const __m256i out1_32 = _mm256_cvtps_epi32(out1);
            const __m256i out_16_0213 = _mm256_packs_epi32(out0_32, out1_32);
            const __m256i out_16 =
                _mm256_permute4x64_epi64(out_16_0213, _MM_SHUFFLE(3,1,2,0));
            _mm256_storeu_si256((void *)dest, out_16);
        }
    }

    _mm_setcsr(saved_mxcsr);

#elif defined(ENABLE_ASM_X86_SSE2)

    const uint32_t saved_mxcsr = _mm_getcsr();
    uint32_t mxcsr = saved_mxcsr;
    mxcsr &= ~(3<<13);  // RC (00 = round to nearest)
    mxcsr |= 1<<7;      // EM_INVALID
    _mm_setcsr(mxcsr);

    const __m128 k32767 = _mm_set1_ps(32767.0f);
    const __m128 k7FFFFFFF = CAST_M128(_mm_set1_epi32(0x7FFFFFFF));
    const __m128 k80000000 = CAST_M128(_mm_set1_epi32(0x80000000));

    if ((((uintptr_t)src | (uintptr_t)dest) & 15) == 0) {
        for (; count >= 8; src += 8, dest += 8, count -= 8) {
            const __m128 in0 = _mm_load_ps(src);
            const __m128 in1 = _mm_load_ps(src+4);
            const __m128 in0_scaled = _mm_mul_ps(in0, k32767);
            const __m128 in1_scaled = _mm_mul_ps(in1, k32767);
            const __m128 in0_abs = _mm_and_ps(in0_scaled, k7FFFFFFF);
            const __m128 in1_abs = _mm_and_ps(in1_scaled, k7FFFFFFF);
            const __m128 in0_sign = _mm_and_ps(in0_scaled, k80000000);
            const __m128 in1_sign = _mm_and_ps(in1_scaled, k80000000);
            const __m128 in0_sat = _mm_min_ps(in0_abs, k32767);
            const __m128 in1_sat = _mm_min_ps(in1_abs, k32767);
            const __m128 out0 = _mm_or_ps(in0_sat, in0_sign);
            const __m128 out1 = _mm_or_ps(in1_sat, in1_sign);
            const __m128i out0_32 = _mm_cvtps_epi32(out0);
            const __m128i out1_32 = _mm_cvtps_epi32(out1);
            const __m128i out_16 = _mm_packs_epi32(out0_32, out1_32);
            _mm_store_si128((void *)dest, out_16);
        }
    } else {
        for (; count >= 8; src += 8, dest += 8, count -= 8) {
            const __m128 in0 = _mm_loadu_ps(src);
            const __m128 in1 = _mm_loadu_ps(src+4);
            const __m128 in0_scaled = _mm_mul_ps(in0, k32767);
            const __m128 in1_scaled = _mm_mul_ps(in1, k32767);
            const __m128 in0_abs = _mm_and_ps(in0_scaled, k7FFFFFFF);
            const __m128 in1_abs = _mm_and_ps(in1_scaled, k7FFFFFFF);
            const __m128 in0_sign = _mm_and_ps(in0_scaled, k80000000);
            const __m128 in1_sign = _mm_and_ps(in1_scaled, k80000000);
            const __m128 in0_sat = _mm_min_ps(in0_abs, k32767);
            const __m128 in1_sat = _mm_min_ps(in1_abs, k32767);
            const __m128 out0 = _mm_or_ps(in0_sat, in0_sign);
            const __m128 out1 = _mm_or_ps(in1_sat, in1_sign);
            const __m128i out0_32 = _mm_cvtps_epi32(out0);
            const __m128i out1_32 = _mm_cvtps_epi32(out1);
            const __m128i out_16 = _mm_packs_epi32(out0_32, out1_32);
            _mm_storeu_si128((void *)dest, out_16);
        }
    }

    _mm_setcsr(saved_mxcsr);

#endif  // ENABLE_ASM_*

    for (int i = 0; i < count; i++) {
        const float sample = src[i];
        if (UNLIKELY(sample < -1.0f)) {
            dest[i] = -32767;
        } else if (LIKELY(sample <= 1.0f)) {
            dest[i] = (int16_t)roundf(sample * 32767.0f);
        } else {
            dest[i] = 32767;
        }
    }
}

/*-----------------------------------------------------------------------*/

void float_to_int16_interleave(int16_t *dest, float **src, int channels,
                               int count)
{
    for (int i = 0; i < count; i++) {
        for (int c = 0; c < channels; c++, dest++) {
            const float sample = src[c][i];
            if (UNLIKELY(sample < -1.0f)) {
                *dest = -32767;
            } else if (LIKELY(sample <= 1.0f)) {
                *dest = (int16_t)roundf(sample * 32767.0f);
            } else {
                *dest = 32767;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

void float_to_int16_interleave_2(
    int16_t *__restrict dest, const float *__restrict src0,
    const float *__restrict src1, int count)
{
#if defined(ENABLE_ASM_ARM_NEON)

    const float32x4_t k32767 = {32767, 32767, 32767, 32767};
    const float32x4_t k0_5 = {0.5, 0.5, 0.5, 0.5};
    const uint32x4_t k7FFFFFFF = {0x7FFFFFFF, 0x7FFFFFFF,
                                  0x7FFFFFFF, 0x7FFFFFFF};
    for (; count >= 4; src0 += 4, src1 += 4, dest += 8, count -= 4) {
        const float32x4_t in0 = vld1q_f32(src0);
        const float32x4_t in1 = vld1q_f32(src1);
        const float32x4_t in0_scaled = vmulq_f32(in0, k32767);
        const float32x4_t in1_scaled = vmulq_f32(in1, k32767);
        const uint32x4_t in0_abs = vandq_u32((uint32x4_t)in0_scaled,
                                             k7FFFFFFF);
        const uint32x4_t in1_abs = vandq_u32((uint32x4_t)in1_scaled,
                                             k7FFFFFFF);
        const uint32x4_t in0_sign = vbicq_u32((uint32x4_t)in0_scaled,
                                              k7FFFFFFF);
        const uint32x4_t in1_sign = vbicq_u32((uint32x4_t)in1_scaled,
                                              k7FFFFFFF);
        const uint32x4_t in0_sat = vminq_u32(in0_abs, (uint32x4_t)k32767);
        const uint32x4_t in1_sat = vminq_u32(in1_abs, (uint32x4_t)k32767);
        /* Note that we have to add 0.5 to the absolute values here because
         * vcvt always rounds toward zero. */
        const float32x4_t in0_adj = vaddq_f32((float32x4_t)in0_sat, k0_5);
        const float32x4_t in1_adj = vaddq_f32((float32x4_t)in1_sat, k0_5);
        const float32x4_t out0 = (float32x4_t)vorrq_u32((uint32x4_t)in0_adj,
                                                        in0_sign);
        const float32x4_t out1 = (float32x4_t)vorrq_u32((uint32x4_t)in1_adj,
                                                        in1_sign);
        const int32x4_t out0_32 = vcvtq_s32_f32(out0);
        const int32x4_t out1_32 = vcvtq_s32_f32(out1);
        int32x4x2_t out_32 = vzipq_s32(out0_32, out1_32);
#if defined(__GNUC__) && !defined(__clang__) && !defined(__aarch64__)
        int16x8_t out_16;
        __asm__(
            "vmovn.i32 %e0, %q1\n"
            "vmovn.i32 %f0, %q2\n"
            : "=&w" (out_16)
            : "w" (out_32.val[0]), "w" (out_32.val[1])
        );
#else
        const int16x4_t out0_16 = vmovn_s32(out_32.val[0]);
        const int16x4_t out1_16 = vmovn_s32(out_32.val[1]);
        const int16x8_t out_16 = vcombine_s16(out0_16, out1_16);
#endif
        vst1q_s16(dest, out_16);
    }

#elif defined(ENABLE_ASM_X86_AVX2)

    ASSERT((((uintptr_t)src0 | (uintptr_t)src1 | (uintptr_t)dest) & 31) == 0);

    const uint32_t saved_mxcsr = _mm_getcsr();
    uint32_t mxcsr = saved_mxcsr;
    mxcsr &= ~(3<<13);  // RC (00 = round to nearest)
    mxcsr |= 1<<7;      // EM_INVALID
    _mm_setcsr(mxcsr);

    const __m256 k32767 = _mm256_set1_ps(32767.0f);
    const __m256 k7FFFFFFF = _mm256_castsi256_ps(_mm256_set1_epi32(0x7FFFFFFF));
    const __m256 k80000000 = _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000));

    for (; count >= 8; src0 += 8, src1 += 8, dest += 16, count -= 8) {
        const __m256 in0 = _mm256_load_ps(src0);
        const __m256 in1 = _mm256_load_ps(src1);
        const __m256 in0_scaled = _mm256_mul_ps(in0, k32767);
        const __m256 in1_scaled = _mm256_mul_ps(in1, k32767);
        const __m256 in0_abs = _mm256_and_ps(in0_scaled, k7FFFFFFF);
        const __m256 in1_abs = _mm256_and_ps(in1_scaled, k7FFFFFFF);
        const __m256 in0_sign = _mm256_and_ps(in0_scaled, k80000000);
        const __m256 in1_sign = _mm256_and_ps(in1_scaled, k80000000);
        const __m256 in0_sat = _mm256_min_ps(in0_abs, k32767);
        const __m256 in1_sat = _mm256_min_ps(in1_abs, k32767);
        const __m256 out0 = _mm256_or_ps(in0_sat, in0_sign);
        const __m256 out1 = _mm256_or_ps(in1_sat, in1_sign);
        const __m256i out0_32 = _mm256_cvtps_epi32(out0);
        const __m256i out1_32 = _mm256_cvtps_epi32(out1);
        const __m256i out_32_lo = _mm256_unpacklo_epi32(out0_32, out1_32);
        const __m256i out_32_hi = _mm256_unpackhi_epi32(out0_32, out1_32);
        const __m256i out_16 = _mm256_packs_epi32(out_32_lo, out_32_hi);
        _mm256_store_si256((void *)dest, out_16);
    }

    _mm_setcsr(saved_mxcsr);

#elif defined(ENABLE_ASM_X86_SSE2)

    ASSERT((((uintptr_t)src0 | (uintptr_t)src1 | (uintptr_t)dest) & 15) == 0);

    const uint32_t saved_mxcsr = _mm_getcsr();
    uint32_t mxcsr = saved_mxcsr;
    mxcsr &= ~(3<<13);  // RC (00 = round to nearest)
    mxcsr |= 1<<7;      // EM_INVALID
    _mm_setcsr(mxcsr);

    const __m128 k32767 = _mm_set1_ps(32767.0f);
    const __m128 k7FFFFFFF = CAST_M128(_mm_set1_epi32(0x7FFFFFFF));
    const __m128 k80000000 = CAST_M128(_mm_set1_epi32(0x80000000));

    for (; count >= 4; src0 += 4, src1 += 4, dest += 8, count -= 4) {
        const __m128 in0 = _mm_load_ps(src0);
        const __m128 in1 = _mm_load_ps(src1);
        const __m128 in0_scaled = _mm_mul_ps(in0, k32767);
        const __m128 in1_scaled = _mm_mul_ps(in1, k32767);
        const __m128 in0_abs = _mm_and_ps(in0_scaled, k7FFFFFFF);
        const __m128 in1_abs = _mm_and_ps(in1_scaled, k7FFFFFFF);
        const __m128 in0_sign = _mm_and_ps(in0_scaled, k80000000);
        const __m128 in1_sign = _mm_and_ps(in1_scaled, k80000000);
        const __m128 in0_sat = _mm_min_ps(in0_abs, k32767);
        const __m128 in1_sat = _mm_min_ps(in1_abs, k32767);
        const __m128 out0 = _mm_or_ps(in0_sat, in0_sign);
        const __m128 out1 = _mm_or_ps(in1_sat, in1_sign);
        const __m128i out0_32 = _mm_cvtps_epi32(out0);
        const __m128i out1_32 = _mm_cvtps_epi32(out1);
        const __m128i out_32_lo = _mm_unpacklo_epi32(out0_32, out1_32);
        const __m128i out_32_hi = _mm_unpackhi_epi32(out0_32, out1_32);
        const __m128i out_16 = _mm_packs_epi32(out_32_lo, out_32_hi);
        _mm_store_si128((void *)dest, out_16);
    }

    _mm_setcsr(saved_mxcsr);

#endif  // ENABLE_ASM_*

    for (int i = 0; i < count; i++) {
        const float sample0 = src0[i];
        const float sample1 = src1[i];
        if (UNLIKELY(sample0 < -1.0f)) {
            dest[i*2+0] = -32767;
        } else if (LIKELY(sample0 <= 1.0f)) {
            dest[i*2+0] = (int16_t)roundf(sample0 * 32767.0f);
        } else {
            dest[i*2+0] = 32767;
        }
        if (UNLIKELY(sample1 < -1.0f)) {
            dest[i*2+1] = -32767;
        } else if (LIKELY(sample1 <= 1.0f)) {
            dest[i*2+1] = (int16_t)roundf(sample1 * 32767.0f);
        } else {
            dest[i*2+1] = 32767;
        }
    }
}

/*************************************************************************/
/*************************************************************************/
