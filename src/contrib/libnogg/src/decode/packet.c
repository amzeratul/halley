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
#include "src/decode/inlines.h"
#include "src/decode/io.h"
#include "src/decode/packet.h"

#include <string.h>

/*************************************************************************/
/**************************** Helper routines ****************************/
/*************************************************************************/

/**
 * getn_packet_raw:  Read a sequence of bytes from the current packet.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     True on success, false on end of packet or error.
 */
static bool getn_packet_raw(stb_vorbis *handle, char *buf, int len)
{
    while (len > 0) {
        if (handle->segment_pos >= handle->segment_size) {
            if (handle->last_seg || !next_segment(handle)) {
                return false;
            }
        }
        int to_copy = len;
        /* Note that currently, this comparison always evaluates to false
         * because this function is only called for data which is known to
         * fully reside within the first segment of a packet. */
        if (to_copy > handle->segment_size - handle->segment_pos) {
            to_copy = handle->segment_size - handle->segment_pos;
        }
        memcpy(buf, &handle->segment_data[handle->segment_pos], to_copy);
        handle->segment_pos += to_copy;
        buf += to_copy;
        len -= to_copy;
    }
    return true;
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

bool next_segment(stb_vorbis *handle)
{
    ASSERT(!handle->last_seg);

    if (handle->packet_mode) {
        /* If we get here, there must be data remaining in the packet, since
         * otherwise last_seg would have been set on the previous call. */
        ASSERT(handle->packet_len > 0);
        const int segment_size = min(handle->packet_len, 255);
        memcpy(handle->segment_data, handle->packet_data, segment_size);
        handle->segment_size = segment_size;
        handle->segment_pos = 0;
        handle->packet_data += segment_size;
        handle->packet_len -= segment_size;
        handle->last_seg = (handle->packet_len == 0);
        return true;
    }

    if (handle->next_seg == -1) {
        if (!start_page(handle, true)) {
            handle->last_seg = true;
            handle->last_seg_index = handle->segment_count - 1;
            return false;
        }
        if (!(handle->page_flag & PAGEFLAG_continued_packet)) {
            return error(handle, VORBIS_continued_packet_flag_invalid);
        }
    }
    const uint8_t len = handle->segments[handle->next_seg++];
    if (len < 255) {
        handle->last_seg = true;
        handle->last_seg_index = handle->next_seg - 1;
    }
    if (handle->next_seg >= handle->segment_count) {
        handle->next_seg = -1;
    }
    if (len > 0 && !getn(handle, handle->segment_data, len)) {
        return error(handle, VORBIS_unexpected_eof);
    }
    handle->segment_size = len;
    handle->segment_pos = 0;
    return len > 0;
}

/*-----------------------------------------------------------------------*/

void reset_page(stb_vorbis *handle)
{
    handle->next_seg = -1;
}

/*-----------------------------------------------------------------------*/

bool start_page(stb_vorbis *handle, bool check_page_number)
{
    uint8_t page_header[27];

    if (handle->scan_for_next_page) {
        /* Find the beginning of a page. */
        static const uint8_t capture_pattern[4] = "OggS";
        int capture_index = 0;
        do {
            page_header[capture_index] = get8(handle);
            if (UNLIKELY(handle->eof)) {
                return error(handle, VORBIS_reached_eof);
            }
            if (page_header[capture_index] == capture_pattern[capture_index]) {
                capture_index++;
            } else if (page_header[capture_index] == capture_pattern[0]) {
                capture_index = 1;
            } else {
                capture_index = 0;
            }
        } while (capture_index < 4);
        if (!getn(handle, page_header+4, sizeof(page_header)-4)) {
            return error(handle, VORBIS_reached_eof);
        }
    } else {
        /* Check that a new page can be read from the stream. */
        if (!getn(handle, page_header, sizeof(page_header))) {
            return error(handle, VORBIS_reached_eof);
        }
        if (memcmp(page_header, "OggS", 4) != 0) {
            return error(handle, VORBIS_missing_capture_pattern);
        }
    }

    /* Check the Ogg bitstream version. */
    if (page_header[4] != 0) {
        return error(handle, VORBIS_invalid_stream_structure_version);
    }

    /* Parse the page header. */
    handle->page_flag = page_header[5];
    const uint64_t sample_pos = extract_64(&page_header[6]);
    const uint32_t bitstream_id = extract_32(&page_header[14]);
    const uint32_t page_number = extract_32(&page_header[18]);
    UNUSED const uint32_t crc = extract_32(&page_header[22]);
    handle->segment_count = page_header[26];

    /* Read the list of segment lengths. */
    if (!getn(handle, handle->segments, handle->segment_count)) {
        return error(handle, VORBIS_unexpected_eof);
    }

    /* Skip over pages belonging to other bitstreams. */
    if (handle->bitstream_id_set) {
        if (bitstream_id != handle->bitstream_id) {
            unsigned int page_size = 0;
            for (int i = 0; i < handle->segment_count; i++) {
                page_size += handle->segments[i];
            }
            skip(handle, page_size);
            return start_page(handle, check_page_number);
        }
    } else {
        handle->bitstream_id = bitstream_id;
        handle->bitstream_id_set = true;
    }

    /* If this page has a sample position, find the packet it belongs to,
     * which will be the complete packet in this page. */
    handle->end_seg_with_known_loc = -2;  // Assume no packet with sample pos.
    if (sample_pos != ~UINT64_C(0)) {
        for (int i = (int)handle->segment_count - 1; i >= 0; i--) {
            /* An Ogg segment with size < 255 indicates the end of a packet. */
            if (handle->segments[i] < 255) {
                handle->end_seg_with_known_loc = i;
                handle->known_loc_for_packet = sample_pos;
                break;
            }
        }
    }

    /* If we haven't started decoding yet, record this page as the first
     * page containing audio.  We'll keep updating the data until we
     * actually start decoding, at which point first_decode will be cleared. */
    if (handle->first_decode) {
        int len = 27 + handle->segment_count;
        for (int i = 0; i < handle->segment_count; i++) {
            len += handle->segments[i];
        }
        /* p_first.page_start is set by start_decoder(). */
        handle->p_first.page_end = handle->p_first.page_start + len;
        handle->p_first.after_previous_page_start = handle->p_first.page_start;
        handle->p_first.first_decoded_sample = 0;
        handle->p_first.last_decoded_sample = sample_pos;
    }

    if (handle->segment_count > 0) {
        handle->next_seg = 0;
    }

    /* If this page is out of sequence, return an error.  We wait until
     * the end of the function to do this so that the page is set up for
     * reading after the error is handled. */
    const uint32_t last_page = handle->page_number;
    handle->page_number = page_number;
    if (check_page_number && page_number != last_page + 1) {
        return error(handle, VORBIS_wrong_page_number);
    }

    return true;
}

/*-----------------------------------------------------------------------*/

bool start_packet(stb_vorbis *handle)
{
    ASSERT(!handle->packet_mode);

    handle->last_seg = false;
    handle->valid_bits = 0;
    handle->segment_size = 0;
    while (handle->next_seg == -1) {
        if (!start_page(handle, true)) {
            return false;
        }
        if (handle->page_flag & PAGEFLAG_continued_packet) {
            return error(handle, VORBIS_continued_packet_flag_invalid);
        }
    }
    return true;
}

/*-----------------------------------------------------------------------*/

void start_packet_direct(stb_vorbis *handle, const void *packet,
                         int32_t packet_len)
{
    ASSERT(handle->packet_mode);
    ASSERT(packet != NULL);
    ASSERT(packet_len > 0);

    handle->packet_data = packet;
    handle->packet_len = packet_len;
    handle->last_seg = false;
    ASSERT(next_segment(handle));
}

/*-----------------------------------------------------------------------*/

int get8_packet(stb_vorbis *handle)
{
    handle->valid_bits = 0;
    return get8_packet_raw(handle);
}

/*-----------------------------------------------------------------------*/

int32_t get32_packet(stb_vorbis *handle)
{
    handle->valid_bits = 0;
    uint8_t value_buf[4];
    getn_packet_raw(handle, (char *)value_buf, sizeof(value_buf));
    return (int32_t)extract_32(value_buf);
}

/*-----------------------------------------------------------------------*/

bool getn_packet(stb_vorbis *handle, void *buf, int len)
{
    handle->valid_bits = 0;
    return getn_packet_raw(handle, buf, len);
}

/*-----------------------------------------------------------------------*/

uint32_t get_bits(stb_vorbis *handle, int count)
{
    if (handle->valid_bits < 0) {
        return 0;
    }
    if (handle->valid_bits < count) {
        if (count > 24) {
            /* We have to handle this case specially to avoid overflow of
             * the bit accumulator. */
            uint32_t value = get_bits(handle, 24);
            value |= get_bits(handle, count-24) << 24;
            return value;
        }
        if (handle->valid_bits == 0) {
            handle->acc = 0;
        }
        while (handle->valid_bits < count) {
            int32_t byte = get8_packet_raw(handle);
            if (UNLIKELY(byte == EOP)) {
                handle->valid_bits = -1;
                return 0;
            }
            handle->acc |= byte << handle->valid_bits;
            handle->valid_bits += 8;
        }
    }
    const uint32_t value = handle->acc & ((UINT32_C(1) << count) - 1);
    handle->acc >>= count;
    handle->valid_bits -= count;
    return value;
}

/*-----------------------------------------------------------------------*/

bool flush_packet(stb_vorbis *handle)
{
    while (!handle->last_seg) {
        if (!next_segment(handle)) {  // Only fails at EOF.
            break;
        }
    }
    if (handle->eof) {
        return error(handle, VORBIS_unexpected_eof);
    }
    handle->segment_size = 0;
    handle->valid_bits = 0;
    return true;
}

/*************************************************************************/
/*************************************************************************/
