/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_PACKET_H
#define NOGG_SRC_DECODE_PACKET_H

/*************************************************************************/
/*************************************************************************/

/* Constant returned from get8_packet_raw() when trying to read past the
 * end of a packet. */
#define EOP  (-1)

/* Flag bits set in the page_flag field of the decoder handle based on the
 * status of the current page. */
#define PAGEFLAG_continued_packet   1
#define PAGEFLAG_first_page         2
#define PAGEFLAG_last_page          4

/*-----------------------------------------------------------------------*/

/**
 * next_segment:  Advance to the next segment in the current packet.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on error or end of packet.
 */
#define next_segment INTERNAL(next_segment)
extern bool next_segment(stb_vorbis *handle);

/**
 * reset_page:  Reset the current stream state to start reading from a new
 * page.  A call to this function should be followed by a call to start_page()
 * or start_packet().
 *
 * [Parameters]
 *     handle: Stream handle.
 */
#define reset_page INTERNAL(reset_page)
extern void reset_page(stb_vorbis *handle);

/**
 * start_page:  Verify that the stream read position is at the beginning
 * of a valid Ogg page, and start reading from the page.
 *
 * On error, some bytes may have been consumed from the stream.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     check_page_number: True to check the page number for sequentiality,
 *         false to skip the check.
 * [Return value]
 *     True on success, false on error.
 */
#define start_page INTERNAL(start_page)
extern bool start_page(stb_vorbis *handle, bool check_page_number);

/**
 * start_packet:  Start reading a new packet at the current stream read
 * position, advancing to a new page if necessary.  Invalid for packet
 * mode decoders.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on error.
 */
#define start_packet INTERNAL(start_packet)
extern bool start_packet(stb_vorbis *handle);

/**
 * start_packet_direct:  Start reading a new packet from the given data
 * buffer.  Only valid for packet mode decoders.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on error.
 */
#define start_packet_direct INTERNAL(start_packet_direct)
extern void start_packet_direct(stb_vorbis *handle, const void *packet,
                                int32_t packet_len);

/**
 * get8_packet:  Read one byte from the current packet.  The bit accumulator
 * for bitstream reads is cleared.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Byte read, or EOP on end of packet or error.
 */
#define get8_packet INTERNAL(get8_packet)
extern int get8_packet(stb_vorbis *handle);

/**
 * get32_packet:  Read a byte-aligned 32-bit signed integer from the
 * current packet.  The bit accumulator for bitstream reads is cleared.
 *
 * This function does not check for end-of-packet.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Value read.
 */
#define get32_packet INTERNAL(get32_packet)
extern int32_t get32_packet(stb_vorbis *handle);

/**
 * getn_packet:  Read a sequence of bytes from the current packet.  The bit
 * accumulator for bitstream reads is cleared.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on end of packet or error.
 */
#define getn_packet INTERNAL(getn_packet)
extern bool getn_packet(stb_vorbis *handle, void *buf, int len);

/**
 * get_bits:  Read a value of arbitrary bit length from the stream.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Value read, or 0 on end of packet or error.
 */
#define get_bits INTERNAL(get_bits)
extern uint32_t get_bits(stb_vorbis *handle, int count);

/**
 * flush_packet:  Advance the stream read position to the end of the
 * current packet.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     False on unexpected end-of-file, true otherwise.
 */
#define flush_packet INTERNAL(flush_packet)
extern bool flush_packet(stb_vorbis *handle);

/*-----------------------------------------------------------------------*/

/**
 * get8_packet_raw:  Read one byte from the current packet.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Byte read, or EOP on end of packet or error.
 */
static inline UNUSED int get8_packet_raw(stb_vorbis *handle)
{
    if (handle->segment_pos >= handle->segment_size) {
        if (handle->last_seg || !next_segment(handle)) {
            return EOP;
        }
    }
    return handle->segment_data[handle->segment_pos++];
}

/**
 * fill_bits:  Fill the bit accumulator with as much data as possible.
 *
 * [Parameters]
 *     handle: Stream handle.
 */
static inline UNUSED void fill_bits(stb_vorbis *handle)
{
    if (handle->valid_bits == 0) {
        handle->acc = 0;
    }
    while (handle->valid_bits <= ((int)sizeof(long) * 8 - 8)) {
        const int32_t byte = get8_packet_raw(handle);
        if (UNLIKELY(byte == EOP)) {
            break;
        }
        handle->acc |= (unsigned long)byte << handle->valid_bits;
        handle->valid_bits += 8;
    }
}

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_PACKET_H
