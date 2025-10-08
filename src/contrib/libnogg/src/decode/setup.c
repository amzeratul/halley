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
#include "src/decode/common.h"
#include "src/decode/crc32.h"
#include "src/decode/inlines.h"
#include "src/decode/io.h"
#include "src/decode/packet.h"
#include "src/decode/setup.h"
#include "src/decode/tables.h"
#include "src/util/memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Older versions of GCC warn about shadowing functions with variables, so
 * work around those warnings. */
#if defined(__GNUC__) && !defined(__clang__) && __GNUC__==4 && __GNUC_MINOR__<7
# define floor floor_
# define index index_
#endif

/*************************************************************************/
/****************************** Local data *******************************/
/*************************************************************************/

/* Byte memory alignment for buffer allocation.  Some optimized decoding
 * routines require specific alignments when loading data into vector
 * registers for SIMD operations. */
#ifdef ENABLE_ASM_X86_AVX2
# define BUFFER_ALIGN  32
#else
# define BUFFER_ALIGN  16
#endif

/* Vorbis header packet IDs. */
enum {
    VORBIS_packet_ident = 1,
    VORBIS_packet_comment = 3,
    VORBIS_packet_setup = 5,
};

/* Data structure mapping array values to array indices.  Used in
 * parse_floors() for sorting data. */
typedef struct ArrayElement {
    uint16_t value;
    int8_t index;
} ArrayElement;

/*************************************************************************/
/**************************** Helper routines ****************************/
/*************************************************************************/

/**
 * float32_unpack:  Convert a 32-bit floating-point bit pattern read from
 * the bitstream to a native floating-point value.
 *
 * [Parameters]
 *     bits: Raw data read from the bitstream (32 bits).
 * [Return value]
 *     Corresponding floating-point value.
 */
static CONST_FUNCTION float float32_unpack(uint32_t bits)
{
    const float mantissa = (float)(bits & UINT32_C(0x001FFFFF));
    const int   exponent =   (int)(bits & UINT32_C(0x7FE00000)) >> 21;
    const bool  sign     =        (bits & UINT32_C(0x80000000)) != 0;
    return ldexpf(sign ? -mantissa : mantissa, exponent-788);
}

/*-----------------------------------------------------------------------*/

/**
 * ilog:  Return the Vorbis-style base-2 log of the argument, with any
 * fractional part of the result truncated.  The Vorbis specification
 * defines ilog2(1) = 1, ilog2(2) = 2, ilog2(4) = 3, etc., such that the
 * return value is the number of bits required to express the argument
 * (with 0 and negative values being defined to return 0).
 */
static CONST_FUNCTION int ilog(uint32_t n)
{
#ifdef __GNUC__
    if (LIKELY((int32_t)n > 0)) {
        return 32 - __builtin_clz(n);
    } else {
        return 0;
    }
#else
    static const signed char log2_4[16] = {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
    const uint32_t u1 = UINT32_C(1);

    /* 2 compares if n < 1<<14, 3 compares otherwise (4 if n >= 1<<29) */
    if (n < (u1 << 14))
         if (n < (u1 <<  4))           return  0 + log2_4[n      ];
         else if (n < (u1 <<  9))      return  5 + log2_4[n >>  5];
              else                     return 10 + log2_4[n >> 10];
    else if (n < (u1 << 24))
              if (n < (u1 << 19))      return 15 + log2_4[n >> 15];
              else                     return 20 + log2_4[n >> 20];
         else if (n < (u1 << 29))      return 25 + log2_4[n >> 25];
              else if (n < (u1 << 31)) return 30 + log2_4[n >> 30];
                   else /*negative*/   return 0;
#endif
}

/*-----------------------------------------------------------------------*/

/**
 * uint32_compare:  Compare two 32-bit unsigned integer values.  Comparison
 * function for qsort().
 */
static int uint32_compare(const void *p, const void *q)
{
    const uint32_t x = *(uint32_t *)p;
    const uint32_t y = *(uint32_t *)q;
    return x < y ? -1 : x > y;
}

/*-----------------------------------------------------------------------*/

/**
 * array_element_compare:  Compare two ArrayElement values.  Comparison
 * function for qsort().
 */
static int array_element_compare(const void *p, const void *q)
{
    ArrayElement *a = (ArrayElement *)p;
    ArrayElement *b = (ArrayElement *)q;
    return a->value < b->value ? -1 : a->value > b->value;
}

/*-----------------------------------------------------------------------*/

/**
 * lookup1_values:  Return the length of the value index for a codebook
 * vector lookup table of lookup type 1.  This is defined in the Vorbis
 * specification as "the greatest integer value for which [return_value]
 * to the power of [codebook_dimensions] is less than or equal to
 * [codebook_entries]".
 *
 * [Parameters]
 *     entries: Number of codebook entries.
 *     dimensions: Number of codebook dimensions (must be positive; must be
 *         no greater than 30, to prevent integer overflow).
 * [Return value]
 *     Value index length.
 */
static CONST_FUNCTION int32_t lookup1_values(int32_t entries, int dimensions)
{
    ASSERT(dimensions > 0);
    ASSERT(dimensions <= 30);

    /* Conceptually, we want to calculate floor(entries ^ (1/dimensions)).
     * Generic exponentiation is a slow operation on some systems, so we
     * use log() and exp() instead: a^(1/b) == e^(log(a) * 1/b) */
    int32_t retval =
        (int32_t)floorf(expf(logf((float)entries) / (float)dimensions));

    /* Rounding could conceivably cause us to end up with the wrong value,
     * so check retval+1 just in case. */
    int32_t test = retval + 1;
    int32_t product = test;
    for (int power = 2; power <= dimensions; power++) {
        ASSERT((int64_t)product * test <= 0x7FFFFFFF);
        product *= test;
    }
    if (product <= entries) {
        retval++;
        ASSERT((int32_t)powf((float)(retval+1), (float)dimensions) > entries);
    } else {
        ASSERT((int32_t)powf((float)retval, (float)dimensions) <= entries);
    }
    return retval;
}

/*-----------------------------------------------------------------------*/

/**
 * bark:  Return the value of bark(x) for the given x, as defined by
 * section 6.2.3 of the Vorbis specification (including errata 20150227,
 * which corrects a typo in the formula).
 */
static CONST_FUNCTION float bark(float x)
{
    return 13.1f * atanf(0.00074f*x)
         + 2.24f * atanf(0.0000000185f*(x*x))
         + 0.0001f*x;
}

/*-----------------------------------------------------------------------*/

/**
 * floor0_map:  Return the value of map[i] for the given type 0 floor
 * configuration, as defined by section 6.2.3 of the Vorbis specification.
 *
 * [Parameters]
 *     floor: Floor configuration.
 *     n: Half of window size.
 *     i: Function argument, assumed to be in the range [0,n-1].
 * [Return value]
 *     map[i]
 */
static PURE_FUNCTION int floor0_map(const Floor0 *floor, int n, int i)
{
    const int foobar =  // Yes, the spec uses the variable name "foobar".
        (int)floorf(bark((float)(floor->rate*i) / (2.0f*(float)n))
                    * (floor->bark_map_size / bark(0.5f*floor->rate)));
    return min(foobar, floor->bark_map_size - 1);
}

/*-----------------------------------------------------------------------*/

/**
 * validate_header_packet:  Validate the Vorbis header packet at the
 * current stream read position and return its type.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Packet type (nonzero), or zero on error or invalid packet.
 */
static int validate_header_packet(stb_vorbis *handle)
{
    uint8_t buf[7];
    if (!getn_packet(handle, buf, sizeof(buf))) {
        return 0;
    }
    if (!(buf[0] & 1) || memcmp(buf+1, "vorbis", 6) != 0) {
        return 0;
    }
    return buf[0];
}

/*-----------------------------------------------------------------------*/

/**
 * compute_codewords:  Generate the Huffman code tables for the given codebook.
 *
 * [Parameters]
 *     book: Codebook to operate on.
 *     lengths: Array of codeword lengths per symbol.
 *     values: Array into which the symbol for each code will be written.
 *         Used only for sparse codebooks.
 * [Return value]
 *     True on success, false on error (underspecified or overspecified tree).
 */
static bool compute_codewords(Codebook *book, const int8_t *lengths,
                              int32_t *values)
{
    /* Number of symbols (for convenience). */
    const int32_t count = book->entries;
    /* Current index in the codeword list. */
    int32_t index = 0;
    /* Next code available at each codeword length ([0] is unused). */
    uint32_t available[33];
    memset(available, 0, sizeof(available));

    for (int32_t symbol = 0; symbol < count; symbol++) {
        if (lengths[symbol] == NO_CODE) {
            continue;
        }

        /* Determine the code for this value, which will always be the
         * lowest available leaf.  (stb_vorbis note: "this property, and
         * the fact we can never have more than one free leaf at a given
         * level, isn't totally trivial to prove, but it seems true and
         * the assert never fires, so!") */
        uint32_t code;
        int bitpos;  // Position of the bit toggled for this code, plus one.
        if (index == 0) {
            /* This is the first value, so it always gets an all-zero code. */
            code = 0;
            bitpos = 0;
        } else {
            bitpos = lengths[symbol];
            while (bitpos > 0 && !available[bitpos]) {
                bitpos--;
            }
            if (bitpos == 0) {
                return false;  // Overspecified tree.
            }
            code = available[bitpos];
            available[bitpos] = 0;
        }

        /* Add the code and corresponding symbol to the tree. */
        if (book->sparse) {
            book->codewords       [index] = bit_reverse(code);
            book->codeword_lengths[index] = lengths[symbol];
            values                [index] = symbol;
        } else {
            book->codewords      [symbol] = bit_reverse(code);
        }
        index++;

        /* If the code's final bit isn't a 1, propagate availability down
         * the tree. */
        for (int i = bitpos+1; i <= lengths[symbol]; i++) {
            ASSERT(!available[i]);
            available[i] = code | (UINT32_C(1) << (32-i));
        }
    }

    /* Handle the case of a tree with a single symbol.  From errata
     * 20150226 to the Vorbis specification, single-symbol codebooks are
     * allowed but must have a codeword length of one bit, and decoders
     * should treat both a 0 bit and a 1 bit as the single symbol when
     * decoding. */
    if (index == 1) {
        /* Find the symbol and make sure it has length 1. */
        int32_t symbol;
        for (symbol = 0; ; symbol++) {
            ASSERT(symbol < count);
            if (lengths[symbol] != NO_CODE) {
                if (lengths[symbol] != 1) {
                    return false;  // Wrong code length.
                }
                break;
            }
        }
        /* Add a second copy of the same symbol to the tree so a 1 bit
         * also decodes to that symbol. */
        ASSERT(available[1] == UINT32_C(1) << 31);
        available[1] = 0;
        /* We force (in parse_codebooks()) single-symbol tables to be
         * sparse so we can properly handle reading both 0 bits and 1 bits. */
        ASSERT(book->sparse);
        ASSERT(book->sorted_entries == 2);
        book->codewords       [1] = 1;
        book->codeword_lengths[1] = 1;
        values                [1] = symbol;
    }

    for (int i = 1; i <= 32; i++) {
        if (available[i]) {
            return false;  // Underspecified tree.
        }
    }
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * compute_sorted_huffman:  Generate the sorted Huffman table used for
 * binary search of Huffman codes which are too long for the O(1) lookup
 * table.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     book: Codebook to operate on.
 *     lengths: List of lengths for each symbol.  For non-sparse codebooks,
 *         the array is indexed by symbol; for sparse codebooks, each entry
 *         corresponds to an entry of values[].
 *     values: List of values (symbols) for sparse codebooks.
 */
static void compute_sorted_huffman(const stb_vorbis *handle, Codebook *book,
                                   const int8_t *lengths,
                                   const int32_t *values)
{
    /* Build the list of entries to be included in the binary search table.
     * For non-sparse books, we skip over entries which are handled by the
     * accelerated table to avoid wasting space on them in the table.
     * (For sparse books, codeword_lengths[] is linked to sorted_codewords[]
     * and we need lengths for values in the accelerated table, so we have
     * to include everything in this table.) */
    if (book->sparse) {
        for (int32_t i = 0; i < book->sorted_entries; i++) {
            book->sorted_codewords[i] = bit_reverse(book->codewords[i]);
        }
    } else {
        int32_t count = 0;
        for (int32_t i = 0; i < book->entries; i++) {
            if (lengths[i] > handle->fast_huffman_length
             && lengths[i] != NO_CODE) {
                book->sorted_codewords[count++] =
                    bit_reverse(book->codewords[i]);
            }
        }
        ASSERT(count == book->sorted_entries);
    }

    /* Sort the codeword list. */
    qsort(book->sorted_codewords, (size_t)book->sorted_entries,
          sizeof(*book->sorted_codewords), uint32_compare);

    /* Map symbols to the sorted codeword list.  We iterate over the
     * original symbol list since we can use binary search on the sorted
     * list to find the matching entry. */
    const int32_t entries =
        book->sparse ? book->sorted_entries : book->entries;
    for (int32_t i = 0; i < entries; i++) {
        const int len = book->sparse ? lengths[values[i]] : lengths[i];
        if (book->sparse
         || (len > handle->fast_huffman_length && len != NO_CODE)) {
            const uint32_t code = bit_reverse(book->codewords[i]);
            int32_t low = 0, high = book->sorted_entries;
            /* sorted_codewords[low] <= code < sorted_codewords[high] */
            while (low+1 < high) {
                const int32_t mid = (low + high) / 2;
                if (book->sorted_codewords[mid] <= code) {
                    low = mid;
                } else {
                    high = mid;
                }
            }
            ASSERT(book->sorted_codewords[low] == code);
            if (book->sparse) {
                book->sorted_values[low] = values[i];
                book->codeword_lengths[low] = (int8_t)len;
            } else {
                book->sorted_values[low] = i;
            }
        }
    }
}

/*-----------------------------------------------------------------------*/

/**
 * compute_accelerated_huffman:  Create the O(1) lookup table for short
 * Huffman codes for the given codebook.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     book: Codebook to operate on.
 */
static void compute_accelerated_huffman(const stb_vorbis *handle,
                                        Codebook *book)
{
    int16_t *fast_huffman = book->fast_huffman;
    const unsigned int fast_huffman_mask = handle->fast_huffman_mask;
    memset(fast_huffman, -1, sizeof(*fast_huffman) * (fast_huffman_mask + 1));

    int32_t len = book->sparse ? book->sorted_entries : book->entries;
    if (len > 32767) {
        len = 32767;
    }
    for (int i = 0; i < (int)len; i++) {
        if (book->codeword_lengths[i] <= handle->fast_huffman_length) {
            uint32_t code = (book->sparse
                             ? bit_reverse(book->sorted_codewords[i])
                             : book->codewords[i]);
            /* Set table entries for all entries with this code in the
             * low-end bits. */
            const uint32_t code_inc = UINT32_C(1) << book->codeword_lengths[i];
            for (; code <= fast_huffman_mask; code += code_inc) {
                book->fast_huffman[code] = (int16_t)i;
            }
        }
    }
}

/*************************************************************************/
/************** Setup data parsing/initialization routines ***************/
/*************************************************************************/

/**
 * init_blocksize:  Allocate and initialize lookup tables used for each
 * audio block size.  handle->blocksize[] is assumed to have been initialized.
 *
 * On error, handle->error will be set appropriately.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     index: Block size index (0 or 1).
 * [Return value]
 *     True on success, false on error.
 */
static bool init_blocksize(stb_vorbis *handle, const int index)
{
#ifdef USE_LOOKUP_TABLES

    ASSERT(handle->blocksize_bits[index] >= 6);
    ASSERT(handle->blocksize_bits[index] <= 13);
    const int bits_index = handle->blocksize_bits[index] - 6;

    handle->A[index] = table_A[bits_index];
    handle->B[index] = table_B[bits_index];
    handle->C[index] = table_C[bits_index];
    handle->bit_reverse[index] = table_bitrev[bits_index];
    handle->window_weights[index] = table_weights[bits_index];

#else  // !USE_LOOKUP_TABLES

    const int blocksize = handle->blocksize[index];

    /* 16-byte alignment to help out vectorized loops. */
    handle->A[index] = mem_alloc(
        handle->mem_opaque, sizeof(*handle->A[index]) * (blocksize/2),
        BUFFER_ALIGN);
    handle->B[index] = mem_alloc(
        handle->mem_opaque, sizeof(*handle->B[index]) * (blocksize/2),
        BUFFER_ALIGN);
    handle->C[index] = mem_alloc(
        handle->mem_opaque, sizeof(*handle->C[index]) * (blocksize/4),
        BUFFER_ALIGN);
    if (!handle->A[index] || !handle->B[index] || !handle->C[index]) {
        return error(handle, VORBIS_outofmem);
    }
    float *__restrict A = handle->A[index];
    float *__restrict B = handle->B[index];
    float *__restrict C = handle->C[index];
    const float pi_blocksize = M_PIf / blocksize;
    const float two_pi_blocksize = 2 * pi_blocksize;
    const float half_pi_blocksize = 0.5f * pi_blocksize;
    for (int i = 0; i < blocksize/2; i += 2) {
        A[i  ] =  cosf(i*two_pi_blocksize);
        A[i+1] = -sinf(i*two_pi_blocksize);
        B[i  ] =  cosf((i+1)*half_pi_blocksize) * 0.5f;
        B[i+1] =  sinf((i+1)*half_pi_blocksize) * 0.5f;
    }
    for (int i = 0; i < blocksize/4; i += 2) {
        C[i  ] =  cosf((i+1)*two_pi_blocksize);
        C[i+1] = -sinf((i+1)*two_pi_blocksize);
    }

    handle->bit_reverse[index] = mem_alloc(
        handle->mem_opaque, sizeof(*handle->bit_reverse[index]) * (blocksize/8),
        32);
    if (!handle->bit_reverse[index]) {
        return error(handle, VORBIS_outofmem);
    }
    uint16_t *__restrict bitrev = handle->bit_reverse[index];
    const int bits = handle->blocksize_bits[index];
    for (int i = 0; i < blocksize/8; i++) {
        bitrev[i] = (bit_reverse(i) >> (32-bits+3)) << 2;
    }

    handle->window_weights[index] = mem_alloc(
        handle->mem_opaque,
        sizeof(*handle->window_weights[index]) * (blocksize/2), 0);
    if (!handle->window_weights[index]) {
        return error(handle, VORBIS_outofmem);
    }
    float *__restrict weights = handle->window_weights[index];
    for (int i = 0; i < blocksize/2; i++) {
        const float x = sinf((i+0.5f)*pi_blocksize);
        weights[i] = sinf(0.5f * M_PIf * (x*x));
    }

#endif  // USE_LOOKUP_TABLES

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_codebook_lookup:  Parse vector lookup data for a codebook from the
 * setup header.  The lookup type is assumed to be either 1 or 2.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     book: Codebook into which to store parsed data.
 * [Return value]
 *     True on success, false on error.
 */
static bool parse_codebook_lookup(stb_vorbis *handle, Codebook *book)
{
    book->minimum_value = float32_unpack(get_bits(handle, 32));
    book->delta_value = float32_unpack(get_bits(handle, 32));
    book->value_bits = get_bits(handle, 4) + 1;
    book->sequence_p = get_bits(handle, 1);
    if (book->lookup_type == 1) {
        if (book->dimensions == 0 || book->dimensions > 30) {
            /* Malformed input. */
            return error(handle, VORBIS_invalid_setup);
        }
        book->lookup_values = lookup1_values(book->entries, book->dimensions);
    } else {
        book->lookup_values = book->entries * book->dimensions;
    }

    uint16_t *mults = mem_alloc(
        handle->mem_opaque, sizeof(*mults) * book->lookup_values, 0);
    if (!mults) {
        return error(handle, VORBIS_outofmem);
    }
    for (int32_t j = 0; j < book->lookup_values; j++) {
        mults[j] = get_bits(handle, book->value_bits);
    }

    /* Precompute and/or pre-expand multiplicands depending on decoder
     * options. */

    enum {EXPAND_LOOKUP1, COPY, NONE} precompute_type = COPY;
    if (!handle->divides_in_codebook && book->lookup_type == 1) {
        if (book->sparse && book->sorted_entries == 0) {
            precompute_type = NONE;  // Empty codebook!
        } else {
            precompute_type = EXPAND_LOOKUP1;
        }
    }

    if (precompute_type == EXPAND_LOOKUP1) {
        /* Pre-expand multiplicands to avoid a divide in an inner decode
         * loop. */
        const int32_t len =
            book->sparse ? book->sorted_entries : book->entries;
        book->multiplicands = mem_alloc(
            handle->mem_opaque,
            (sizeof(*book->multiplicands) * len * book->dimensions), 0);
        if (!book->multiplicands) {
            mem_free(handle->mem_opaque, mults);
            return error(handle, VORBIS_outofmem);
        }
        for (int32_t j = 0; j < len; j++) {
            const int32_t index = book->sparse ? book->sorted_values[j] : j;
            int divisor = 1;
            float last = book->minimum_value;
            for (int k = 0; k < book->dimensions; k++) {
                const int offset = (index / divisor) % book->lookup_values;
                const float value = mults[offset]*book->delta_value + last;
                book->multiplicands[j*book->dimensions + k] = value;
                if (book->sequence_p) {
                    last = value + book->minimum_value;
                }
                /* lookup1_values() guarantees that this multiplication
                 * can never overflow. */
                ASSERT((int64_t)divisor * book->lookup_values <= 0x7FFFFFFF);
                divisor *= book->lookup_values;
            }
        }
        book->lookup_type = 2;
        book->sequence_p = false;

    } else if (precompute_type == COPY) {
        book->multiplicands = mem_alloc(
            handle->mem_opaque,
            sizeof(*book->multiplicands) * book->lookup_values, 0);
        if (!book->multiplicands) {
            mem_free(handle->mem_opaque, mults);
            return error(handle, VORBIS_outofmem);
        }
        if (book->lookup_type == 2 && book->sequence_p) {
            /* NOTE: Not tested because I can't find an example.  (It
             * seems that historically, the reference encoder only ever
             * used sequence_p with floor 0 codebooks, which were lookup
             * type 1.) */
            float last = 0;
            int dim_count = 0;
            for (int32_t j = 0; j < book->lookup_values; j++) {
                last = book->multiplicands[j] =
                    mults[j] * book->delta_value + book->minimum_value + last;
                dim_count++;
                if (dim_count == book->dimensions) {
                    last = 0;
                    dim_count = 0;
                }
            }
            book->sequence_p = false;
        } else {
            for (int32_t j = 0; j < book->lookup_values; j++) {
                book->multiplicands[j] =
                    mults[j] * book->delta_value + book->minimum_value;
            }
        }

    } else {  // precompute_type == NONE, so an empty type-1 codebook.
        ASSERT(book->lookup_type == 1);
        book->lookup_type = 2;
        book->sequence_p = false;
    }

    mem_free(handle->mem_opaque, mults);

    /* All type-2 lookups (including converted type-1s) should have
     * sequence_p unset, since we bake it into the array. */
    ASSERT(!(book->lookup_type == 2 && book->sequence_p));
    /* All lookup tables should be type 2 for !divides_in_codebook. */
    ASSERT(!(!handle->divides_in_codebook && book->lookup_type != 2));

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_codebook:  Parse data for a single codebook from the setup header.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     book: Codebook into which to store parsed data.
 * [Return value]
 *     True on success, false on error.
 */
static bool parse_codebook(stb_vorbis *handle, Codebook *book)
{
    /* Verify the codebook sync pattern and read basic parameters. */
    if (get_bits(handle, 24) != UINT32_C(0x564342)) {
        return error(handle, VORBIS_invalid_setup);
    }
    book->dimensions = get_bits(handle, 16);
    book->entries = get_bits(handle, 24);
    const bool ordered = get_bits(handle, 1);

    /* Abort early on an abnormally large "dimensions * entries" product
     * to avoid multiplication overflow later on. */
    if ((uint64_t)book->dimensions * book->entries >= UINT64_C(1)<<28) {
        return error(handle, VORBIS_invalid_setup);
    }

    /* Read in the code lengths for each entry. */
    int8_t *lengths = mem_alloc(handle->mem_opaque, book->entries, 0);
    if (!lengths) {
        return error(handle, VORBIS_outofmem);
    }
    int32_t code_count = 0;  // Only used for non-ordered codebooks.
    if (ordered) {
        book->sparse = false;
        int32_t current_entry = 0;
        int current_length = get_bits(handle, 5) + 1;
        while (current_entry < book->entries) {
            if (current_length > 32) {
                mem_free(handle->mem_opaque, lengths);
                return error(handle, VORBIS_invalid_setup);
            }
            const int32_t limit = book->entries - current_entry;
            const int32_t count = get_bits(handle, ilog(limit));
            if (current_entry + count > book->entries) {
                mem_free(handle->mem_opaque, lengths);
                return error(handle, VORBIS_invalid_setup);
            }
            memset(lengths + current_entry, current_length, count);
            current_entry += count;
            current_length++;
        }
    } else {
        book->sparse = get_bits(handle, 1);
        for (int32_t j = 0; j < book->entries; j++) {
            const bool present = book->sparse ? get_bits(handle,1) : true;
            if (present) {
                lengths[j] = get_bits(handle, 5) + 1;
                code_count++;
            } else {
                lengths[j] = NO_CODE;
            }
        }
        /* If the codebook is marked as sparse but is more than 25% full,
         * converting it to non-sparse will generally give us a better
         * space/time tradeoff. */
        if (book->sparse && code_count >= book->entries/4) {
            book->sparse = false;
        }
    }

    /* Check for single-symbol trees and force them to be sparse with
     * two entries in the sorted table (see compute_codewords()). */
    if (book->sparse) {
        if (code_count == 1) {
            code_count = 2;
        }
    } else {
        int num_symbols = 0;
        for (int32_t j = 0; j < book->entries && num_symbols <= 1; j++) {
            if (lengths[j] != NO_CODE) {
                num_symbols++;
            }
        }
        if (num_symbols == 1) {
            book->sparse = true;
            if (ordered) {
                ASSERT(book->entries == 1);
            } else {
                ASSERT(code_count == 1);
            }
            code_count = 2;
        }
    }

    /* Count the number of entries to be included in the sorted Huffman
     * tables (we omit codes in the accelerated table to save space). */
    if (book->sparse) {
        book->sorted_entries = code_count;
    } else {
        book->sorted_entries = 0;
        /* If the Huffman binary search is disabled, we just leave
         * sorted_entries set to zero, which will prevent the table
         * from being created below. */
        if (handle->huffman_binary_search) {
            for (int32_t j = 0; j < book->entries; j++) {
                if (lengths[j] > handle->fast_huffman_length
                 && lengths[j] != NO_CODE) {
                    book->sorted_entries++;
                }
            }
        }
    }

    /* Allocate and generate the codeword tables. */
    int32_t *values = NULL;  // Used only for sparse codebooks.
    if (book->sparse) {
        if (book->sorted_entries > 0) {
            book->codeword_lengths = mem_alloc(
                handle->mem_opaque, book->sorted_entries, 0);
            book->codewords = mem_alloc(
                handle->mem_opaque,
                sizeof(*book->codewords) * book->sorted_entries, 0);
            values = mem_alloc(
                handle->mem_opaque, sizeof(*values) * book->sorted_entries, 0);
            if (!book->codeword_lengths || !book->codewords || !values) {
                mem_free(handle->mem_opaque, lengths);
                mem_free(handle->mem_opaque, values);
                return error(handle, VORBIS_outofmem);
            }
        }
    } else {
        book->codeword_lengths = lengths;
        book->codewords = mem_alloc(
            handle->mem_opaque, sizeof(*book->codewords) * book->entries, 0);
        if (!book->codewords) {
            return error(handle, VORBIS_outofmem);
        }
    }
    if (!compute_codewords(book, lengths, values)) {
        if (book->sparse) {
            mem_free(handle->mem_opaque, values);
            mem_free(handle->mem_opaque, lengths);
        }
        return error(handle, VORBIS_invalid_setup);
    }

    /* Build the code lookup tables used in decoding. */
    if (book->sorted_entries > 0) {
        /* Include an extra slot at the end for a sentinel. */
        book->sorted_codewords = mem_alloc(
            handle->mem_opaque,
            sizeof(*book->sorted_codewords) * (book->sorted_entries+1), 0);
        /* Include an extra slot before the beginning so we can access
         * sorted_values[-1] instead of needing a guard on the index. */
        book->sorted_values = mem_alloc(
            handle->mem_opaque,
            sizeof(*book->sorted_values) * (book->sorted_entries+1), 0);
        if (!book->sorted_codewords || !book->sorted_values) {
            if (book->sparse) {
                mem_free(handle->mem_opaque, values);
                mem_free(handle->mem_opaque, lengths);
            }
            return error(handle, VORBIS_outofmem);
        }
        book->sorted_codewords[book->sorted_entries] = ~UINT32_C(0);
        book->sorted_values++;
        book->sorted_values[-1] = -1;
        compute_sorted_huffman(handle, book, lengths, values);
    }
    book->fast_huffman = mem_alloc(
        handle->mem_opaque, ((handle->fast_huffman_mask + 1)
                         * sizeof(*book->fast_huffman)), 0);
    if (!book->fast_huffman) {
        if (book->sparse) {
            mem_free(handle->mem_opaque, values);
            mem_free(handle->mem_opaque, lengths);
        }
        return error(handle, VORBIS_outofmem);
    }
    if (handle->fast_huffman_length > 0) {
        compute_accelerated_huffman(handle, book);
    } else {
        book->fast_huffman[0] = -1;
    }

    /* For sparse codebooks, we've now compressed the data into our
     * target arrays so we no longer need the original buffers. */
    if (book->sparse) {
        mem_free(handle->mem_opaque, lengths);
        mem_free(handle->mem_opaque, values);
        mem_free(handle->mem_opaque, book->codewords);
        book->codewords = NULL;
    }

    /* Read the vector lookup table. */
    book->lookup_type = get_bits(handle, 4);
    if (book->lookup_type == 0) {
        /* No lookup data to be read. */
    } else if (book->lookup_type <= 2) {
        if (!parse_codebook_lookup(handle, book)) {
            return false;
        }
    } else {  // book->lookup_type > 2
        return error(handle, VORBIS_invalid_setup);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_codebooks:  Parse codebook data from the setup header.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on error.
 */
static NOINLINE bool parse_codebooks(stb_vorbis *handle)
{
    handle->codebook_count = get_bits(handle, 8) + 1;
    handle->codebooks = mem_alloc(
        handle->mem_opaque, sizeof(*handle->codebooks) * handle->codebook_count, 0);
    if (!handle->codebooks) {
        return error(handle, VORBIS_outofmem);
    }
    memset(handle->codebooks, 0,
           sizeof(*handle->codebooks) * handle->codebook_count);

    for (int i = 0; i < handle->codebook_count; i++) {
        if (!parse_codebook(handle, &handle->codebooks[i])) {
            return false;
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_time_domain_transforms:  Parse time domain transform data from the
 * setup header.  The spec declares that these are all placeholders in the
 * current Vorbis version, so we don't actually store any data.
 *
 * [Parameters]
 *     handle: Stream handle.
 */
static NOINLINE bool parse_time_domain_transforms(stb_vorbis *handle)
{
    const int vorbis_time_count = get_bits(handle, 6) + 1;
    for (int i = 0; i < vorbis_time_count; i++) {
        const uint16_t value = get_bits(handle, 16);
        if (value != 0) {
            return error(handle, VORBIS_invalid_setup);
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_floors:  Parse floor data from the setup header.
 *
 * [Parameters]
 *     handle: Stream handle.
 */
static NOINLINE bool parse_floors(stb_vorbis *handle)
{
    int largest_floor0_order = 0;
    int longest_floor1_list = 0;

    handle->floor_count = get_bits(handle, 6) + 1;
    handle->floor_config = mem_alloc(
        handle->mem_opaque, handle->floor_count * sizeof(*handle->floor_config), 0);
    if (!handle->floor_config) {
        return error(handle, VORBIS_outofmem);
    }
    memset(handle->floor_config, 0,
           handle->floor_count * sizeof(*handle->floor_config));

    for (int i = 0; i < handle->floor_count; i++) {
        handle->floor_types[i] = get_bits(handle, 16);

        if (handle->floor_types[i] == 0) {
            Floor0 *floor = &handle->floor_config[i].floor0;
            floor->order = get_bits(handle, 8);
            floor->rate = get_bits(handle, 16);
            floor->bark_map_size = get_bits(handle, 16);
            floor->amplitude_bits = get_bits(handle, 6);
            floor->amplitude_offset = get_bits(handle, 8);
            floor->number_of_books = get_bits(handle, 4) + 1;
            /* The missing "-1" on the ilog() argument is deliberate,
             * according to the specification.  (Presumably, it's an error
             * in the original encoder/decoder which wasn't detected until
             * after the spec was published.) */
            floor->book_bits = ilog(floor->number_of_books);
            for (int j = 0; j < floor->number_of_books; j++) {
                floor->book_list[j] = get_bits(handle, 8);
                if (floor->book_list[j] >= handle->codebook_count) {
                    return error(handle, VORBIS_invalid_setup);
                }
            }

            floor->map[0] = mem_alloc(
                handle->mem_opaque,
                (handle->blocksize[0] + handle->blocksize[1] + 2)
                    * sizeof(*floor->map[0]),
                0);
            if (!floor->map[0]) {
                return error(handle, VORBIS_outofmem);
            }
            floor->map[1] = floor->map[0] + (handle->blocksize[0] + 1);
            for (int blocktype = 0; blocktype < 2; blocktype++) {
                const int n = handle->blocksize[blocktype] / 2;
                for (int j = 0; j < n; j++) {
                    floor->map[blocktype][j] = floor0_map(floor, n, j);
                }
                floor->map[blocktype][n] = -1;
            }

            /* Make sure the array is allocated so the decoder doesn't
             * have to worry about checking for NULL, even if the floor
             * definition improperly specifies order zero. */
            if (largest_floor0_order < 1) {
                largest_floor0_order = 1;
            }
            if (floor->order > largest_floor0_order) {
                largest_floor0_order = floor->order;
            }

        } else if (handle->floor_types[i] == 1) {
            Floor1 *floor = &handle->floor_config[i].floor1;
            floor->partitions = get_bits(handle, 5);
            int max_class = -1;
            for (int j = 0; j < floor->partitions; j++) {
                floor->partition_class_list[j] = get_bits(handle, 4);
                if (floor->partition_class_list[j] > max_class) {
                    max_class = floor->partition_class_list[j];
                }
            }
            for (int j = 0; j <= max_class; j++) {
                floor->class_dimensions[j] = get_bits(handle, 3) + 1;
                floor->class_subclasses[j] = get_bits(handle, 2);
                if (floor->class_subclasses[j]) {
                    floor->class_masterbooks[j] = get_bits(handle, 8);
                    if (floor->class_masterbooks[j] >= handle->codebook_count) {
                        return error(handle, VORBIS_invalid_setup);
                    }
                }
                for (int k = 0; k < (1 << floor->class_subclasses[j]); k++) {
                    floor->subclass_books[j][k] = get_bits(handle, 8) - 1;
                    if (floor->subclass_books[j][k] >= handle->codebook_count) {
                        return error(handle, VORBIS_invalid_setup);
                    }
                }
            }
            floor->floor1_multiplier = get_bits(handle, 2) + 1;
            floor->rangebits = get_bits(handle, 4);
            floor->X_list[0] = 0;
            floor->X_list[1] = 1U << floor->rangebits;
            floor->values = 2;
            for (int j = 0; j < floor->partitions; j++) {
                int c = floor->partition_class_list[j];
                for (int k = 0; k < floor->class_dimensions[c]; k++) {
                    if (floor->values >= FLOOR1_X_LIST_MAX) {
                        return error(handle, VORBIS_invalid_setup);
                    }
                    floor->X_list[floor->values++] =
                        get_bits(handle, floor->rangebits);
                }
            }

            /* We'll need to process the floor curve in X value order, so
             * precompute a sorted list. */
            ArrayElement elements[FLOOR1_X_LIST_MAX];
            for (int j = 0; j < floor->values; j++) {
                elements[j].value = floor->X_list[j];
                elements[j].index = j;
            }
            qsort(elements, floor->values, sizeof(*elements),
                  array_element_compare);
            for (int j = 0; j < floor->values; j++) {
                if (j > 0 && elements[j].value == elements[j-1].value) {
                    /* 7.2.2: All vector [floor1_x_list] element values
                     * must be unique within the vector; a non-unique
                     * value renders the stream undecodable. */
                    return error(handle, VORBIS_invalid_setup);
                }
                floor->sorted_order[j] = elements[j].index;
            }

            /* Find the low and high neighbors for each element of X_list,
             * following the Vorbis definition (which only looks at the
             * preceding elements of the list). */
            for (int j = 2; j < floor->values; j++) {
                const unsigned int X_j = floor->X_list[j];
                int low_index = 0, high_index = 1;
                unsigned int low = floor->X_list[0], high = floor->X_list[1];
                for (int k = 2; k < j; k++) {
                    if (floor->X_list[k] > low && floor->X_list[k] < X_j) {
                        low = floor->X_list[k];
                        low_index = k;
                    }
                    if (floor->X_list[k] < high && floor->X_list[k] > X_j) {
                        high = floor->X_list[k];
                        high_index = k;
                    }
                }
                floor->neighbors[j].low = low_index;
                floor->neighbors[j].high = high_index;
            }

            /* Remember the longest X_list length we've seen for allocating
             * the final_Y buffer later.  Unlike floor 0, we don't have to
             * worry about forcing the array to be allocated because
             * floor->values will always be at least 2. */
            ASSERT(floor->values > 0);
            if (floor->values > longest_floor1_list) {
                longest_floor1_list = floor->values;
            }

        } else {  // handle->floor_types[i] > 1
            return error(handle, VORBIS_invalid_setup);
        }
    }

    if (largest_floor0_order > 0) {
        handle->coefficients = alloc_channel_array(
            handle->mem_opaque, handle->channels,
            sizeof(**handle->coefficients) * largest_floor0_order, 0);
        if (!handle->coefficients) {
            return error(handle, VORBIS_outofmem);
        }
    }
    if (longest_floor1_list > 0) {
        handle->final_Y = alloc_channel_array(
            handle->mem_opaque, handle->channels,
            sizeof(**handle->final_Y) * longest_floor1_list, 0);
        if (!handle->final_Y) {
            return error(handle, VORBIS_outofmem);
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_residues:  Parse residue data from the setup header.
 *
 * [Parameters]
 *     handle: Stream handle.
 */
static NOINLINE bool parse_residues(stb_vorbis *handle)
{
    /* Maximum number of temporary array entries needed by any residue
     * configuration. */
    int residue_max_temp = 0;

    handle->residue_count = get_bits(handle, 6) + 1;
    handle->residue_config = mem_alloc(
        handle->mem_opaque,
        handle->residue_count * sizeof(*handle->residue_config), 0);
    if (!handle->residue_config) {
        return error(handle, VORBIS_outofmem);
    }
    memset(handle->residue_config, 0,
           handle->residue_count * sizeof(*handle->residue_config));

    for (int i = 0; i < handle->residue_count; i++) {
        Residue *r = &handle->residue_config[i];
        handle->residue_types[i] = get_bits(handle, 16);
        if (handle->residue_types[i] > 2) {
            return error(handle, VORBIS_invalid_setup);
        }

        r->begin = get_bits(handle, 24);
        r->end = get_bits(handle, 24);
        r->part_size = get_bits(handle, 24) + 1;
        r->classifications = get_bits(handle, 6) + 1;
        r->classbook = get_bits(handle, 8);
        if (r->classbook >= handle->codebook_count) {
            return error(handle, VORBIS_invalid_setup);
        }
        if (handle->codebooks[r->classbook].dimensions == 0) {
            return error(handle, VORBIS_invalid_setup);
        }

        uint8_t residue_cascade[64];
        for (int j = 0; j < r->classifications; j++) {
            const uint8_t low_bits = get_bits(handle,3);
            const uint8_t high_bits = (get_bits(handle, 1)
                                       ? get_bits(handle, 5) : 0);
            residue_cascade[j] = high_bits<<3 | low_bits;
        }

        r->residue_books = mem_alloc(
            handle->mem_opaque, sizeof(*(r->residue_books)) * r->classifications,
            0);
        if (!r->residue_books) {
            return error(handle, VORBIS_outofmem);
        }
        for (int j = 0; j < r->classifications; j++) {
            for (int k = 0; k < 8; k++) {
                if (residue_cascade[j] & (1 << k)) {
                    r->residue_books[j][k] = get_bits(handle, 8);
                    if (r->residue_books[j][k] >= handle->codebook_count) {
                        return error(handle, VORBIS_invalid_setup);
                    }
                } else {
                    r->residue_books[j][k] = -1;
                }
            }
        }

        const int classwords = handle->codebooks[r->classbook].dimensions;
        r->classdata = mem_alloc(
            handle->mem_opaque,
            sizeof(*r->classdata) * handle->codebooks[r->classbook].entries, 0);
        if (!r->classdata) {
            return error(handle, VORBIS_outofmem);
        }
        r->classdata[0] = mem_alloc(
            handle->mem_opaque, (handle->codebooks[r->classbook].entries
                             * sizeof(**r->classdata) * classwords), 0);
        if (!r->classdata[0]) {
            return error(handle, VORBIS_outofmem);
        }
        for (int j = 0; j < handle->codebooks[r->classbook].entries; j++) {
            r->classdata[j] = r->classdata[0] + (j * classwords);
            int temp = j;
            for (int k = classwords-1; k >= 0; k--) {
                r->classdata[j][k] = temp % r->classifications;
                temp /= r->classifications;
            }
        }

        /* Calculate how many temporary array entries we need, and update
         * the maximum across all residue configurations. */
        const int temp_required = (r->end - r->begin) / r->part_size;
        if (temp_required > residue_max_temp) {
            residue_max_temp = temp_required;
        }
    }

    if (handle->divides_in_residue) {
        handle->classifications = alloc_channel_array(
            handle->mem_opaque, handle->channels, residue_max_temp * sizeof(int),
            0);
    } else {
        handle->classifications = alloc_channel_array(
            handle->mem_opaque, handle->channels,
            residue_max_temp * sizeof(uint8_t *), 0);
    }
    if (!handle->classifications) {
        return error(handle, VORBIS_outofmem);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_mappings:  Parse mapping data from the setup header.
 *
 * [Parameters]
 *     handle: Stream handle.
 */
static NOINLINE bool parse_mappings(stb_vorbis *handle)
{
    handle->mapping_count = get_bits(handle, 6) + 1;
    handle->mapping = mem_alloc(
        handle->mem_opaque, handle->mapping_count * sizeof(*handle->mapping), 0);
    if (!handle->mapping) {
        return error(handle, VORBIS_outofmem);
    }
    memset(handle->mapping, 0,
           handle->mapping_count * sizeof(*handle->mapping));
    handle->mapping[0].mux = mem_alloc(
        handle->mem_opaque, (handle->mapping_count * handle->channels
                         * sizeof(*(handle->mapping[0].mux))), 0);
    if (!handle->mapping[0].mux) {
        return error(handle, VORBIS_outofmem);
    }
    for (int i = 1; i < handle->mapping_count; i++) {
        handle->mapping[i].mux =
            handle->mapping[0].mux + (i * handle->channels);
    }

    for (int i = 0; i < handle->mapping_count; i++) {
        Mapping *m = &handle->mapping[i];
        int mapping_type = get_bits(handle, 16);
        if (mapping_type != 0) {
            return error(handle, VORBIS_invalid_setup);
        }
        if (get_bits(handle, 1)) {
            m->submaps = get_bits(handle, 4) + 1;
        } else {
            m->submaps = 1;
        }

        if (get_bits(handle, 1)) {
            m->coupling_steps = get_bits(handle, 8) + 1;
            m->coupling = mem_alloc(
                handle->mem_opaque, m->coupling_steps * sizeof(*m->coupling), 0);
            if (!m->coupling) {
                return error(handle, VORBIS_outofmem);
            }
            for (int j = 0; j < m->coupling_steps; j++) {
                m->coupling[j].magnitude =
                    get_bits(handle, ilog(handle->channels - 1));
                m->coupling[j].angle =
                    get_bits(handle, ilog(handle->channels - 1));
                if (m->coupling[j].magnitude >= handle->channels
                 || m->coupling[j].angle >= handle->channels
                 || m->coupling[j].magnitude == m->coupling[j].angle) {
                    return error(handle, VORBIS_invalid_setup);
                }
            }
        } else {
            m->coupling_steps = 0;
            m->coupling = NULL;
        }

        if (get_bits(handle, 2)) {  // Reserved field, must be zero.
            return error(handle, VORBIS_invalid_setup);
        }

        if (m->submaps > 1) {
            for (int j = 0; j < handle->channels; j++) {
                m->mux[j] = get_bits(handle, 4);
                if (m->mux[j] >= m->submaps) {
                    return error(handle, VORBIS_invalid_setup);
                }
            }
        } else {
            /* The specification doesn't explicitly say what to store in
             * the mux array for this case, but since the values are used
             * to index the submap list, it should be safe to assume that
             * all values are set to zero (the only valid submap index). */
            for (int j = 0; j < handle->channels; j++) {
                m->mux[j] = 0;
            }
        }

        for (int j = 0; j < m->submaps; j++) {
            get_bits(handle, 8);  // Unused time configuration placeholder.
            m->submap_floor[j] = get_bits(handle, 8);
            m->submap_residue[j] = get_bits(handle, 8);
            if (m->submap_floor[j] >= handle->floor_count
             || m->submap_residue[j] >= handle->residue_count) {
                return error(handle, VORBIS_invalid_setup);
            }
        }
    }

    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_modes:  Parse mode data from the setup header.
 *
 * [Parameters]
 *     handle: Stream handle.
 */
static NOINLINE bool parse_modes(stb_vorbis *handle)
{
    handle->mode_count = get_bits(handle, 6) + 1;
    handle->mode_bits = ilog(handle->mode_count - 1);
    for (int i = 0; i < handle->mode_count; i++) {
        Mode *m = &handle->mode_config[i];
        m->blockflag = get_bits(handle, 1);
        m->windowtype = get_bits(handle, 16);
        m->transformtype = get_bits(handle, 16);
        m->mapping = get_bits(handle, 8);
        if (m->windowtype != 0
         || m->transformtype != 0
         || m->mapping >= handle->mapping_count) {
            return error(handle, VORBIS_invalid_setup);
        }
    }

    return true;
}

/*************************************************************************/
/*********************** Top-level packet parsers ************************/
/*************************************************************************/

/**
 * parse_ident_header:  Parse the Vorbis identification header packet.
 * The packet length is assumed to have been validated as 30 bytes, and
 * the 7-byte packet header is assumed to have already been read.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on error.
 */
static NOINLINE bool parse_ident_header(stb_vorbis *handle)
{
    const uint32_t vorbis_version    = get32_packet(handle);
    const int      audio_channels    = get8_packet(handle);
    const uint32_t audio_sample_rate = get32_packet(handle);
    const int32_t  bitrate_maximum   = get32_packet(handle);
    const int32_t  bitrate_nominal   = get32_packet(handle);
    const int32_t  bitrate_minimum   = get32_packet(handle);
    const int      log2_blocksize    = get8_packet(handle);
    const int      framing_flag      = get8_packet(handle);
    ASSERT(framing_flag != EOP);
    ASSERT(get8_packet(handle) == EOP);

    const int log2_blocksize_0 = (log2_blocksize >> 0) & 15;
    const int log2_blocksize_1 = (log2_blocksize >> 4) & 15;

    if (vorbis_version != 0
     || audio_channels == 0
     || audio_sample_rate == 0
     || !(framing_flag & 1)) {
        return error(handle, VORBIS_invalid_first_page);
    }
    if (log2_blocksize_0 < 6
     || log2_blocksize_0 > 13
     || log2_blocksize_1 < log2_blocksize_0
     || log2_blocksize_1 > 13) {
        return error(handle, VORBIS_invalid_setup);
    }

    handle->channels = audio_channels;
    handle->sample_rate = audio_sample_rate;
    handle->nominal_bitrate = bitrate_nominal;
    handle->min_bitrate = bitrate_minimum;
    handle->max_bitrate = bitrate_maximum;
    handle->blocksize[0] = 1 << log2_blocksize_0;
    handle->blocksize[1] = 1 << log2_blocksize_1;
    handle->blocksize_bits[0] = log2_blocksize_0;
    handle->blocksize_bits[1] = log2_blocksize_1;
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * parse_setup_header:  Parse the Vorbis setup header packet.  The 7-byte
 * packet header is assumed to have already been read.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on error.
 */
static NOINLINE bool parse_setup_header(stb_vorbis *handle)
{
    if (!parse_codebooks(handle)) {
        return false;
    }
    if (!parse_time_domain_transforms(handle)) {
        return false;
    }
    if (!parse_floors(handle)) {
        return false;
    }
    if (!parse_residues(handle)) {
        return false;
    }
    if (!parse_mappings(handle)) {
        return false;
    }
    if (!parse_modes(handle)) {
        return false;
    }

    if (get_bits(handle, 1) != 1) {  // Also catches EOP.
        return error(handle, VORBIS_invalid_setup);
    }
    flush_packet(handle);

    return true;
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

bool start_decoder(
    stb_vorbis *handle, const void *id_packet, int32_t id_packet_len,
    const void *setup_packet, int32_t setup_packet_len)
{
    /* Initialize the CRC lookup table, if necessary.  This function
     * modifies global state but is multithread-safe, so we just call it
     * unconditionally. */
    crc32_init();

    if (handle->packet_mode) {
        ASSERT(id_packet);
        ASSERT(id_packet_len > 0);
        start_packet_direct(handle, id_packet, id_packet_len);
    } else {
        /* Check for and parse the identification header packet.  The spec
         * requires that this packet is the only packet in the first Ogg
         * page of the stream.  We follow this requirement to ensure that
         * we're looking at a valid Ogg Vorbis stream. */
        if (!start_page(handle, false)) {
            return false;
        }
        if (handle->page_flag != PAGEFLAG_first_page
         || handle->segment_count != 1
         || handle->segments[0] != 30) {
            return error(handle, VORBIS_invalid_first_page);
        }
        ASSERT(start_packet(handle));
    }
    if (validate_header_packet(handle) != VORBIS_packet_ident) {
        return error(handle, VORBIS_invalid_first_page);
    }
    if (!parse_ident_header(handle)) {
        return false;
    }

    /* Set up stream parameters and allocate buffers based on the stream
     * format. */
    for (int i = 0; i < 2; i++) {
        if (!init_blocksize(handle, i)) {
            return false;
        }
    }
    /* 16-byte alignment to help out vectorized loops. */
    handle->channel_buffers[0] = alloc_channel_array(
        handle->mem_opaque, handle->channels*2,
        sizeof(float) * handle->blocksize[1], BUFFER_ALIGN);
    handle->channel_buffers[1] = handle->channel_buffers[0] + handle->channels;
    handle->outputs = mem_alloc(
        handle->mem_opaque, handle->channels * sizeof(float *), BUFFER_ALIGN);
    handle->previous_window = mem_alloc(
        handle->mem_opaque, handle->channels * sizeof(float *), BUFFER_ALIGN);
    handle->imdct_temp_buf = mem_alloc(
        handle->mem_opaque,
        (handle->blocksize[1] / 2) * sizeof(*handle->imdct_temp_buf),
        BUFFER_ALIGN);
    if (!handle->channel_buffers[0]
     || !handle->outputs
     || !handle->previous_window
     || !handle->imdct_temp_buf) {
        return error(handle, VORBIS_outofmem);
    }
    for (int i = 0; i < handle->channels; i++) {
        memset(handle->channel_buffers[0][i], 0,
               sizeof(float) * handle->blocksize[1]);
        memset(handle->channel_buffers[1][i], 0,
               sizeof(float) * handle->blocksize[1]);
    }

    if (handle->packet_mode) {
        ASSERT(setup_packet);
        ASSERT(setup_packet_len > 0);
        start_packet_direct(handle, setup_packet, setup_packet_len);
        if (validate_header_packet(handle) != VORBIS_packet_setup) {
            return error(handle, VORBIS_invalid_stream);
        }
    } else {
        /* The second Ogg page (and possibly subsequent pages) should
         * contain the comment and setup headers.  The spec requires both
         * headers to exist in that order, but in the spirit of being
         * liberal with what we accept, we also allow the comment header
         * to be missing. */
        bool got_setup_header = false;
        while (!got_setup_header) {
            if (!start_packet(handle)) {
                return false;
            }
            const int type = validate_header_packet(handle);
            switch (type) {
              case VORBIS_packet_comment:
                /* Currently, we don't attempt to parse anything out of the
                 * comment header. */
                flush_packet(handle);
                break;
              case VORBIS_packet_setup:
                got_setup_header = true;
                break;
              default:
                return error(handle, VORBIS_invalid_stream);
            }
        }
    }
    if (!parse_setup_header(handle)) {
        return false;
    }

    /* Set up remaining state parameters for decoding. */
    handle->previous_length = 0;
    handle->first_decode = true;
    if (handle->stream_len >= 0) {
        handle->p_first.page_start =
            (*handle->tell_callback)(handle->io_opaque);
    }

    return true;
}

/*************************************************************************/
/*************************************************************************/
