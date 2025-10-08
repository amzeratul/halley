/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_COMMON_H
#define NOGG_SRC_DECODE_COMMON_H

#include <stdbool.h>
#include <stdint.h>

/*************************************************************************/
/*************************************************************************/

/* Pi as a single-precision floating-point value. */
#define M_PIf  3.14159265f

/* Code length value indicating that a symbol has no associated code. */
#define NO_CODE  127

/* Maximum number of floor-1 X list entries (defined by the Vorbis spec). */
#define FLOOR1_X_LIST_MAX  65

/* Internal helper which evaluates to "const" when precomputed lookup
 * tables are in use, to ensure that code does not try to overwrite
 * constant data. */
#ifdef USE_LOOKUP_TABLES
    #define TABLE_CONST  const
#else
    #define TABLE_CONST  /*nothing*/
#endif

/*-----------------------------------------------------------------------*/

/*
 * Note:  Arrays marked "// varies" are actually of variable length but are
 * small enough that it makes more sense to store them as fixed-size arrays
 * than to incur the extra overhead of dynamic allocation.
 */

/* Data for a codebook. */
typedef struct Codebook {
    /* Codebook configuration. */
    int32_t dimensions;
    int32_t entries;
    bool sparse;
    int8_t lookup_type;
    int8_t value_bits;
    bool sequence_p;
    float minimum_value;
    float delta_value;
    int32_t lookup_values;
    /* List of codewords for each symbol.  Only used for non-sparse
     * codebooks; indexed by symbol value. */
    uint32_t *codewords;
    /* List of codeword lengths for each symbol.  For non-sparse codebooks,
     * the array is indexed by symbol value; for sparse codebooks, the
     * order matches the order of codewords in sorted_codewords[]. */
    int8_t *codeword_lengths;
    /* List of multiplicands (vector components).  These are pre-expanded
     * from the 16-bit integer multiplicands read from the stream into the
     * corresponding "minimum + delta * multiplicand" values. */
    float *multiplicands;
    /* Lookup table for O(1) decoding of short codewords. */
    int16_t *fast_huffman;
    /* Sorted lookup table for binary search of longer codewords. */
    uint32_t *sorted_codewords;
    /* Symbol corresponding to each codeword in sorted_codewords[]. */
    int32_t *sorted_values;
    /* Number of entries in the sorted tables. */
    int32_t sorted_entries;
} Codebook;

/* Data for a type 0 floor curve. */
typedef struct Floor0 {
    /* Floor configuration. */
    uint8_t order;
    uint16_t rate;
    uint16_t bark_map_size;
    int8_t amplitude_bits;
    uint8_t amplitude_offset;
    int8_t number_of_books;
    int8_t book_bits;
    uint8_t book_list[16];  // varies
    /* Lookup table for the map function, for short and long windows. */
    int16_t *map[2];
} Floor0;

/* Structure holding neighbor indices for Floor1.X_list. */
typedef struct Floor1Neighbors {
    uint8_t low;  // Index of next smallest preceding value.
    uint8_t high;  // Index of next largest preceding value.
} Floor1Neighbors;

/* Data for a type 1 floor curve. */
typedef struct Floor1 {
    /* Floor configuration. */
    int8_t partitions;
    int8_t partition_class_list[31];  // varies
    int8_t class_dimensions[16];  // varies
    int8_t class_subclasses[16];  // varies
    uint8_t class_masterbooks[16];  // varies
    int16_t subclass_books[16][8];  // varies
    int8_t floor1_multiplier;
    int8_t rangebits;
    int8_t values;
    uint16_t X_list[FLOOR1_X_LIST_MAX];  // varies
    /* Indices of X_list[] values when sorted, such that
     * X_list[sorted_order[0]] < X_list[sorted_order[1]] < ... */
    int8_t sorted_order[FLOOR1_X_LIST_MAX];  // varies
    /* Low and high neighbors (as defined by the Vorbis spec) for each
     * element of X_list. */
    Floor1Neighbors neighbors[FLOOR1_X_LIST_MAX];  // varies
} Floor1;

/* Union holding data for all possible floor types. */
typedef union Floor {
    Floor0 floor0;
    Floor1 floor1;
} Floor;

/* Data for a residue configuration. */
typedef struct Residue {
    /* Residue configuration data. */
    int32_t begin, end;
    int32_t part_size;
    int8_t classifications;
    uint8_t classbook;
    int16_t (*residue_books)[8];
    /* Precomputed classifications[][] array, used to avoid div/mod in the
     * decode loop.  The Vorbis spec calls this "classifications", but we
     * already have a field by that name (from "residue_classification"),
     * so we use "classdata" instead. */
    uint8_t **classdata;
} Residue;

/* Channel coupling data for a mapping. */
typedef struct CouplingStep {
    uint8_t magnitude;
    uint8_t angle;
} CouplingStep;

/* Data for a mapping configuration. */
typedef struct Mapping {
    /* Data for channel coupling. */
    int16_t coupling_steps;
    CouplingStep *coupling;
    /* Channel multiplex settings. */
    int8_t *mux;
    /* Submap data. */
    int8_t submaps;
    uint8_t submap_floor[15];  // varies
    uint8_t submap_residue[15];  // varies
} Mapping;

/* Data for an encoding mode. */
typedef struct Mode {
   bool blockflag;
   uint8_t mapping;
   uint16_t windowtype;
   uint16_t transformtype;
} Mode;

/* Position information for an Ogg page, used in seeking. */
typedef struct ProbedPage {
    /* The start and end file offsets of this page. */
    int64_t page_start, page_end;
    /* A file offset known to be within the previous page. */
    int64_t after_previous_page_start;
    /* The first and last sample offsets in this page. */
    uint64_t first_decoded_sample;
    uint64_t last_decoded_sample;
} ProbedPage;

/* The top-level decoder handle structure. */
struct stb_vorbis {
    /* Basic stream information. */
    uint32_t sample_rate;
    int32_t nominal_bitrate;
    int32_t min_bitrate;
    int32_t max_bitrate;
    int channels;
    uint64_t total_samples;
    int64_t stream_len;  // From open().
    uint32_t bitstream_id;
    bool bitstream_id_set;  // False until bitstream_id has been set.

    /* Packet mode flag.  If true, the caller is expected to send packets
     * directly to vorbis_decode_packet_direct() rather than implicitly
     * read them via vorbis_decode_packet(). */
    bool packet_mode;

    /* Current packet's data pointer and length, used if packet_mode is
     * true. */
    const uint8_t *packet_data;
    int32_t packet_len;

    /* Callbacks for stream reading, used if packet_mode is false.  The
     * seek and tell callbacks are only used if stream_len >= 0. */
    int32_t (*read_callback)(void *opaque, void *buf, int32_t len);
    void (*seek_callback)(void *opaque, int64_t offset);
    int64_t (*tell_callback)(void *opaque);

    /* Opaque pointer for stream reading callbacks. */
    void *io_opaque;

    /* Opaque pointer for memory allocation calls.  This is always a
     * vorbis_t pointer from the libnogg API functions. */
    void *mem_opaque;

    /* Decoder configuration. */
    uint32_t fast_huffman_mask;
    int8_t fast_huffman_length;
    bool huffman_binary_search;
    bool divides_in_residue;
    bool divides_in_codebook;
    bool scan_for_next_page;

    /* Operation results. */
    bool eof;
    STBVorbisError error;

    /* Have we started decoding yet?  (This flag is set when the handle
     * is created and cleared when the first frmae is decoded.) */
    bool first_decode;

    /* Stream configuration. */
    int16_t blocksize[2];
    int8_t blocksize_bits[2];
    int16_t codebook_count;
    Codebook *codebooks;
    int8_t floor_count;
    uint16_t floor_types[64]; // varies
    Floor *floor_config;
    int8_t residue_count;
    uint16_t residue_types[64]; // varies
    Residue *residue_config;
    int8_t mapping_count;
    Mapping *mapping;
    int8_t mode_count;
    int8_t mode_bits;  // ilog(mode_count - 1)
    Mode mode_config[64];  // varies

    /* IMDCT twiddle factors for each blocksize. */
    TABLE_CONST float *A[2],*B[2],*C[2];
    TABLE_CONST uint16_t *bit_reverse[2];
    /* Window sample weights for each blocksize. */
    TABLE_CONST float *window_weights[2];

    /* Buffers for decoded data, two per channel. */
    float **channel_buffers[2];
    /* Channel buffer index containing this frame's data (0 or 1). */
    int8_t cur_channel_buffer;
    /* Per-channel pointers within channel_buffers to the decode buffer for
     * the current frame. */
    float **outputs;
    /* Per-channel pointers within channel_buffers for data from the
     * previous frame's right-side overlap window. */
    float **previous_window;
    /* Length of the previous window. */
    int previous_length;

    /* Temporary buffers used in floor curve computation. */
    float **coefficients;
    int16_t **final_Y;

    /* Temporary buffer used in residue decoding.  This is either "int **"
     * if the DIVIDES_IN_RESIDUE option is set, or "uint8_t ***" if not. */
    void *classifications;

    /* Temporary buffer for inverse MDCT computation. */
    float *imdct_temp_buf;

    /* Data for the current Ogg page. */
    uint32_t page_number;
    uint8_t segment_count;
    uint8_t page_flag;
    uint8_t segments[255];
    uint8_t segment_data[255];
    uint8_t segment_size;  // Size of current segment's data.
    uint8_t segment_pos;  // Current read position in segment data.
    /* Index of the segment corresponding to the page's sample position, or
     * negative if unknown or unavailable. */
    int end_seg_with_known_loc;
    /* Sample position for that segment's packet. */
    uint64_t known_loc_for_packet;

    /* Index of the next segment to read, or -1 if the current segment is
     * the last one on the page. */
    int next_seg;
    /* Flag indicating whether we've hit the last segment in the page. */
    bool last_seg;
    /* Segment index of the last segment.  Only valid if last_seg is true. */
    int last_seg_index;

    /* Information about the first and last pages containing audio data.
     * Used in seeking. */
    ProbedPage p_first, p_last;

    /* Accumulator for bits read from the stream. */
    unsigned long acc;
    /* Number of valid bits in the accumulator, or -1 if end-of-packet
     * has been reached. */
    int valid_bits;

    /* Sample position of next decoded sample to return. */
    uint64_t current_loc;
    /* True if current_loc has ever been set for this stream. */
    bool current_loc_valid;
};


static inline UNUSED COLD_FUNCTION int error(
   stb_vorbis *f, enum STBVorbisError e)
{
   f->error = e;
   return 0;
}

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_COMMON_H
