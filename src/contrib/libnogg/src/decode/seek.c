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
#include "src/decode/decode.h"
#include "src/decode/inlines.h"
#include "src/decode/io.h"
#include "src/decode/packet.h"
#include "src/util/memory.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/**************************** Helper routines ****************************/
/*************************************************************************/

/**
 * set_file_offset:  Set the stream read position to the given offset from
 * the beginning of the stream.  If the stream is not seekable, this
 * function does nothing.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     offset: New stream read position, in bytes from the beginning of
 *         the stream.
 */
static void set_file_offset(stb_vorbis *handle, int64_t offset)
{
    handle->eof = false;
    (*handle->seek_callback)(handle->io_opaque, offset);
}

/*-----------------------------------------------------------------------*/

/**
 * get_file_offset:  Return the current stream read position.
 *
 * [Parameters]
 *     handle: Stream handle.
 * [Return value]
 *     Current stream read position, in bytes from the beginning of the
 *     stream, or 0 if the stream is not seekable.
 */
static int64_t get_file_offset(stb_vorbis *handle)
{
    return (*handle->tell_callback)(handle->io_opaque);
}

/*-----------------------------------------------------------------------*/

/**
 * find_page:  Locate the first page starting at or after the current read
 * position in the stream.  On success, the stream read position is set to
 * the start of the page.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     end_ret: Pointer to variable to receive the file offset of one byte
 *         past the end of the page.  May be NULL if not needed.
 *     last_ret: Pointer to variable to receive the "last page" flag of
 *         the page.  May be NULL if not needed.
 * [Return value]
 *     True if a page was found, false if not.
 */
static bool find_page(stb_vorbis *handle, int64_t *end_ret, bool *last_ret)
{
    uint8_t readbuf[4096];

    while (!handle->eof) {

        /* See if we have the first byte of an Ogg page. */
        const uint8_t byte = get8(handle);
        if (byte != 'O') {
            continue;
        }

        /* Read in the rest of the (possible) page header. */
        const int64_t page_start = get_file_offset(handle) - 1;
        uint8_t header[27];
        header[0] = byte;
        if (!getn(handle, &header[1], sizeof(header)-1)) {
            break;
        }

        /* See if this really is an Ogg page (as opposed to a random 'O'
         * byte in the middle of audio data). */
        if (header[1] == 'g' && header[2] == 'g' && header[3] == 'S'
         && header[4] == 0 /*Ogg version*/) {

            /* Check the page CRC to make the final determination of whether
             * this is a valid page. */
            const uint32_t expected_crc = extract_32(&header[22]);
            for (int i = 22; i < 26; i++) {
                header[i] = 0;
            }
            uint32_t crc = 0;
            for (int i = 0; i < 27; i++) {
                crc = crc32_update(crc, header[i]);
            }
            unsigned int len = 0;
            if (!getn(handle, readbuf, header[26])) {
                break;
            }
            for (int i = 0; i < header[26]; i++) {
                const unsigned int seglen = readbuf[i];
                crc = crc32_update(crc, seglen);
                len += seglen;
            }
            while (len > 0) {
                const unsigned int readcount = min(len, sizeof(readbuf));
                if (!getn(handle, readbuf, readcount)) {
                    ASSERT(handle->eof);
                    break;
                }
                for (unsigned int i = 0; i < readcount; i++) {
                    crc = crc32_update(crc, readbuf[i]);
                }
                len -= readcount;
            }
            if (handle->eof) {
                break;
            }

            if (crc == expected_crc) {
                /* We found a valid page! */
                if (end_ret) {
                    *end_ret = get_file_offset(handle);
                }
                if (last_ret) {
                    *last_ret = ((header[5] & 0x04) != 0);
                }
                set_file_offset(handle, page_start);
                return true;
            }
        }

        /* It wasn't a valid page, so seek back to the byte after the
         * first one we tested and try again. */
        set_file_offset(handle, page_start + 1);

   }  // while (!handle->eof)

    return false;
}

/*-----------------------------------------------------------------------*/

/**
 * test_page:  Check whether the given pointer points to the beginning of
 * a valid Ogg page, and return its length if so.  The data buffer is
 * assumed to start with the "OggS" capture pattern and a version byte of
 * zero.
 *
 * The data at ptr is destroyed whether the page is valid or not.
 *
 * [Parameters]
 *     ptr: Pointer to data to check.
 *     size: Amount of valid data at ptr, in bytes.
 * [Return value]
 *     Ogg page length in bytes if the page is valid, otherwise 0.
 */
static unsigned int test_page(uint8_t *ptr, unsigned int size)
{
    ASSERT(ptr != NULL);
    ASSERT(size >= 27);
    ASSERT(ptr[0] == 'O');
    ASSERT(ptr[1] == 'g');
    ASSERT(ptr[2] == 'g');
    ASSERT(ptr[3] == 'S');
    ASSERT(ptr[4] == 0);

    unsigned int len = 27 + ptr[26];
    if (size < len) {
        return 0;  // Buffer too small for segment length list.
    }
    for (int i = 0; i < ptr[26]; i++) {
        len += ptr[27+i];
    }
    if (size < len) {
        return 0;  // Buffer too small for page data.
    }

    const uint32_t expected_crc = extract_32(&ptr[22]);
    for (int i = 22; i < 26; i++) {
        ptr[i] = 0;
    }
    uint32_t crc = 0;
    for (unsigned int i = 0; i < len; i++) {
        crc = crc32_update(crc, ptr[i]);
    }
    if (crc == expected_crc) {
        return len;
    } else {
        return 0;
    }
}

/*-----------------------------------------------------------------------*/

/**
 * analyze_page:  Scan an Ogg page starting at the current read position
 * to determine the page's file and sample offset bounds.  The page header
 * is assumed to be valid.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     page_ret: Pointer to variable to receive the page data.
 * [Return value]
 *     True on success, false on error.
 */
static int analyze_page(stb_vorbis *handle, ProbedPage *page_ret)
{
    /* The page start position is easy. */
    page_ret->page_start = get_file_offset(handle);

    /* Parse the header to determine the page length. */
    uint8_t header[27], lacing[255];
    if (!getn(handle, header, 27)) {
        goto bail;
    }
    const int num_segments = header[26];
    if (!getn(handle, lacing, num_segments)) {
        goto bail;
    }
    unsigned int payload_len = 0;
    for (int i = 0; i < num_segments; i++) {
        payload_len += lacing[i];
    }
    page_ret->page_end = page_ret->page_start + 27 + num_segments + payload_len;

    /* The header contains the sample offset of the end of the page.
     * If this value is -1, there are no complete packets (i.e., frames)
     * on the page.  (The end sample offset points to the middle of the
     * window, but if the last frame of the page is a long block and the
     * first one on the next page is a short one, the frame contains data
     * beyond that point.  We don't worry about that here because we may
     * not be able to do anything about it, such as if the long block is
     * the only frame on the page.) */
    page_ret->last_decoded_sample = extract_64(&header[6]);
    if (page_ret->last_decoded_sample == (uint64_t)-1) {
        page_ret->first_decoded_sample = -1;
        goto done;
    }

    /* The header does _not_ contain the sample offset of the beginning of
     * the page, so we have to go through ridiculous contortions to figure
     * it out ourselves, if it's even possible for the page in question.
     * Presumably the Ogg format developers thought it so important to save
     * an extra 64 bits per page that they didn't care about the impact on
     * decoder implementations. */

    /* On the last page of a stream, the end sample position is overloaded
     * to also declare the true length of the final frame (to allow for a
     * stream length of an arbitrary number of samples).  This means we
     * have no way to work out the first sample on the page.  Sigh deeply
     * and give up. */
    if (header[5] & 4) {
        page_ret->first_decoded_sample = page_ret->last_decoded_sample;
        goto done;
    }

    /* Scan the page to find the number of (complete) frames and the type
     * of each one. */
    bool frame_long[255];
    int num_frames = 0;
    bool last_packet_was_audio = false;
    bool packet_start = !(header[5] & PAGEFLAG_continued_packet);
    const int mode_bits = handle->mode_bits;
    for (int i = 0; i < num_segments; i++) {
        if (packet_start) {
            last_packet_was_audio = false;
            if (UNLIKELY(lacing[i] == 0)) {
                continue;
            }
            const uint8_t packet_header = get8(handle);
            const int mode = (packet_header >> 1) & ((1 << mode_bits) - 1);
            if (UNLIKELY(packet_header & 0x01)  // Not an audio packet.
             || UNLIKELY(mode >= handle->mode_count)) {
                skip(handle, lacing[i] - 1);
            } else {
                last_packet_was_audio = true;
                frame_long[num_frames] = handle->mode_config[mode].blockflag;
                skip(handle, lacing[i] - 1);
                num_frames++;
            }
        } else {
            skip(handle, lacing[i]);
        }
        packet_start = (lacing[i] < 255);
    }
    if (UNLIKELY(num_frames == 0)) {
        /* The page claimed to have an end sample position, but there were
         * no frames that completed on the page.  This can never occur for
         * a well-formed stream. */
        page_ret->first_decoded_sample = -1;
        page_ret->last_decoded_sample = -1;
        goto done;
    }
    if (!packet_start && last_packet_was_audio) {
        /* The last packet is incomplete, so ignore that frame. */
        num_frames--;
    }
    if (num_frames == 0) {
        /* The page did not contain any complete frames, so we can't use
         * it as a seek anchor.  Unlike the case above, this case can occur
         * in a legal stream (though it's probably unlikely in real-life
         * streams, since the reference encoder uses a target page size of
         * 4k while Vorbis packets seem to be rarely longer than 1k). */
        page_ret->first_decoded_sample = -1;
        page_ret->last_decoded_sample = -1;
        goto done;
    }

    /* Count backwards from the end of the page to find the beginning
     * sample offset of the first fully-decoded sample on this page (this
     * is the beginning of the _second_ frame, since the first frame will
     * overlap with the last frame of the previous page). */
    uint64_t sample_pos = page_ret->last_decoded_sample;
    for (int i = num_frames-1; i >= 1; i--) {
        sample_pos -= (handle->blocksize[frame_long[i]] / 4
                       + handle->blocksize[frame_long[i-1]] / 4);
    }
    page_ret->first_decoded_sample = sample_pos;

  done:
    set_file_offset(handle, page_ret->page_start);
    return true;

   /* Error conditions come down here. */
  bail:
    set_file_offset(handle, page_ret->page_start);
    return false;
}

/*-----------------------------------------------------------------------*/

/**
 * scan_frame:  Find the next valid frame and return its window parameters.
 * Helper function for seek_frame_from_page().
 *
 * [Parameters]
 *     handle: Stream handle.
 *     left_start_ret, left_end_ret, right_start_ret, mode_index_ret:
 *         Pointers to variables to receive the frame's window parameters.
 * [Return value]
 *     True on success, false on error.
 */
static bool scan_frame(stb_vorbis *handle, int *left_start_ret,
                       int *left_end_ret, int *right_start_ret,
                       int *mode_index_ret)
{
    bool decode_ok;
    do {
        int right_end;
        decode_ok = vorbis_decode_initial(handle, left_start_ret, left_end_ret,
                                          right_start_ret, &right_end,
                                          mode_index_ret);
        if (!flush_packet(handle)
            || (!decode_ok
                && handle->error != VORBIS_invalid_packet
                && handle->error != VORBIS_continued_packet_flag_invalid
                && handle->error != VORBIS_wrong_page_number)) {
            return false;
        }
    } while (!decode_ok);
    return true;
}

/*-----------------------------------------------------------------------*/

/**
 * seek_frame_from_page:  Seek to the frame containing the given target
 * sample by scanning forward from the page beginning at the given offset.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     page_start: File offset of the beginning of the page.
 *     first_sample: Sample offset of the middle of the first complete
 *         frame on the page.
 *     target_sample: Sample to seek to.  Must be no less than first_sample.
 * [Return value]
 *     Number of samples that must be discarded from the beginning of the
 *     frame to reach the target sample.
 */
static int seek_frame_from_page(stb_vorbis *handle, int64_t page_start,
                                uint64_t first_sample, uint64_t target_sample)
{
    ASSERT(target_sample >= first_sample);

    /* Reset the read position to the beginning of the page's first
     * complete frame. */
    set_file_offset(handle, page_start);
    if (UNLIKELY(!start_page(handle, false))) {
        return error(handle, VORBIS_seek_failed);
    }
    if (handle->page_flag & PAGEFLAG_continued_packet) {
        ASSERT(start_packet(handle));
        if (UNLIKELY(!flush_packet(handle))) {
            return error(handle, VORBIS_seek_failed);
        }
    }

    int left_start, left_end, right_start;

    /* Scan the first frame to get its window parameters, and shift
     * first_sample so it points at the first sample returned from that
     * frame. */
    {
        int mode_index;
        if (!scan_frame(handle, &left_start, &left_end, &right_start,
                        &mode_index)) {
            return error(handle, VORBIS_seek_failed);
        }
        const Mode *mode = &handle->mode_config[mode_index];
        /* The frame we just scanned will give fully decoded samples in
         * the window range [left_start,right_start).  However, if it's the
         * very first frame in the file, we need to skip the left half of
         * the window. */
        if (first_sample == 0) {
            left_start = left_end = handle->blocksize[mode->blockflag] / 2;
        }
        first_sample -= (handle->blocksize[mode->blockflag] / 2) - left_start;
    }

    /* Find the frame which, when decoded, will generate the target sample.
     * (This will not necessarily be in the same page, but we only use the
     * passed-in page data as an anchor, so that's not a problem.) */
    int frame = 0;  // Offset from the first complete frame.
    /* frame_start is the position of the first fully decoded sample in
     * frame number 'frame'. */
    uint64_t frame_start = first_sample;
    while (target_sample >= frame_start + (right_start - left_start)) {
        frame++;
        frame_start += right_start - left_start;
        int mode_index;
        if (!scan_frame(handle, &left_start, &left_end, &right_start,
                        &mode_index)) {
            return error(handle, VORBIS_seek_failed);
        }
    }

    /* Determine where we need to start decoding in order to get the
     * desired sample. */
    int frames_to_skip;
    bool decode_one_frame;
    if (target_sample >= frame_start + (left_end - left_start)) {
        /* In this case, the sample is outside the overlapped window
         * segments and doesn't require the previous frame to decode.
         * This can only happen in a long frame preceded or followed by a
         * short frame. */
        frames_to_skip = frame;
        decode_one_frame = false;
    } else {
        /* The sample is in the overlapped portion of the window, so we'll
         * need to decode the previous frame first. */
        ASSERT(frame > 0);  // Guaranteed by function precondition.
        frames_to_skip = frame - 1;
        decode_one_frame = true;
    }

    /* Set up the stream state so the next decode call returns the frame
     * containing the target sample, and return the offset of that sample
     * within the frame. */
    set_file_offset(handle, page_start);
    if (UNLIKELY(!start_page(handle, false))) {
        return error(handle, VORBIS_seek_failed);
    }
    if (handle->page_flag & PAGEFLAG_continued_packet) {
        ASSERT(start_packet(handle));
        if (!flush_packet(handle)) {
            return error(handle, VORBIS_seek_failed);
        }
    }
    for (int i = 0; i < frames_to_skip; i++) {
        if (UNLIKELY(!start_packet(handle))
         || UNLIKELY(!flush_packet(handle))) {
            return error(handle, VORBIS_seek_failed);
        }
    }
    handle->previous_length = 0;
    handle->first_decode = (first_sample == 0 && frames_to_skip == 0);
    if (decode_one_frame) {
        if (UNLIKELY(!vorbis_decode_packet(handle, NULL))) {
            return error(handle, VORBIS_seek_failed);
        }
    }
    handle->current_loc = frame_start;
    handle->current_loc_valid = true;
    handle->error = VORBIS__no_error;
    return (int)(target_sample - frame_start);
}

/*************************************************************************/
/************************** Interface routines ***************************/
/*************************************************************************/

int stb_vorbis_seek(stb_vorbis *handle, uint64_t sample_number)
{
    /* Fail early for unseekable streams. */
    if (handle->stream_len < 0) {
        return error(handle, VORBIS_cant_find_last_page);
    }

    /* Find the first and last pages if they have not yet been looked up. */
    if (handle->p_first.page_end == 0) {
        ASSERT(handle->first_decode);
        if (UNLIKELY(!start_packet(handle))) {
            return error(handle, VORBIS_cant_find_last_page);
        }
        flush_packet(handle);
        handle->first_decode = false;
    }
    if (handle->p_last.page_start == 0) {
        (void) stb_vorbis_stream_length_in_samples(handle);
        if (handle->total_samples == (uint64_t)-1) {
            return 0;
        }
    }

    /* If we're seeking to/past the end of the stream, just do that. */
    if (sample_number >= handle->total_samples) {
        set_file_offset(handle, handle->stream_len);
        reset_page(handle);
        handle->current_loc = handle->total_samples;
        handle->current_loc_valid = true;
        return 0;
    }

    /* If the sample is known to be on the first page, we don't need to
     * search for the correct page. */
    if (sample_number < handle->p_first.last_decoded_sample) {
        return seek_frame_from_page(handle, handle->p_first.page_start,
                                    0, sample_number);
    }

    /* Otherwise, perform an interpolated binary search (biased toward
     * where we expect the page to be located) for the page containing
     * the target sample. */
    ProbedPage low = handle->p_first;
    ProbedPage high = handle->p_last;
    /* Conceptually, we iterate until low.page_end and high.page_start
     * are equal, indicating that they represent two consecutive pages.
     * However, if there's junk data between the two pages, the two
     * positions will never converge, so instead we use
     * high.after_previous_page_start, which is guaranteed to (eventually)
     * fall somewhere in the low page if the two pages are consecutive. */
    while (low.page_end < high.after_previous_page_start) {
        /* Bounds for the search.  We use low.page_start+1 instead of
         * low.page_end as the lower bound to bias the search toward the
         * beginning of the stream, since find_page() scans forward. */
        const int64_t low_offset = low.page_start + 1;
        const int64_t high_offset = high.after_previous_page_start - 1;
        const uint64_t low_sample = low.last_decoded_sample;
        const uint64_t high_sample = high.first_decoded_sample;
        /* These assertions express the search loop invariant that the
         * target sample is located strictly between the low and high
         * bound pages. */
        ASSERT(low_sample <= sample_number);
        ASSERT(sample_number < high_sample);

        /* Pick a probe point for the next iteration.  As we get closer to
         * the target, we reduce the amount of bias, since there may be up
         * to a page's worth of variance in the relative positions of the
         * low and high bounds within their respective pages. */
        float lerp_frac = ((float)(sample_number - low_sample)
                           / (float)(high_sample - low_sample));
        if (high_offset - low_offset < 8192) {
            lerp_frac = 0.5f;
        } else if (high_offset - low_offset < 65536) {
            lerp_frac = 0.25f + lerp_frac*0.5f;
        }
        const int64_t probe = low_offset
            + (int64_t)floorf(lerp_frac * (float)(high_offset - low_offset));

        /* Look for the next page starting after the probe point.  If it's
         * a page without a known sample position, continue scanning forward
         * and use the sample position from the next page that has one. */
        set_file_offset(handle, probe);
        ProbedPage page;
        /* These calls can only fail on a read error. */
        if (UNLIKELY(!find_page(handle, NULL, NULL))
         || UNLIKELY(!analyze_page(handle, &page))) {
            return error(handle, VORBIS_seek_failed);
        }
        while (page.first_decoded_sample == (uint64_t)-1) {
            set_file_offset(handle, page.page_end);
            if (UNLIKELY(!find_page(handle, NULL, NULL))
             || UNLIKELY(!analyze_page(handle, &page))) {
                return error(handle, VORBIS_seek_failed);
            }
        }
        page.after_previous_page_start = probe;

        /* Choose one or the other side of the range and iterate (unless
         * we happened to find the right page, in which case we just stop). */
        if (sample_number >= page.first_decoded_sample) {
            if (sample_number < page.last_decoded_sample) {
                return seek_frame_from_page(handle, page.page_start,
                                            page.first_decoded_sample,
                                            sample_number);
            } else {
                low = page;
            }
        } else {
            high = page;
        }
    }

    return seek_frame_from_page(handle, low.page_start,
                                low.first_decoded_sample, sample_number);
}

/*-----------------------------------------------------------------------*/

uint64_t stb_vorbis_stream_length_in_samples(stb_vorbis *handle)
{
    if (!handle->total_samples) {
        /* Fail early for unseekable streams. */
        if (handle->stream_len < 0) {
            handle->total_samples = -1;
            return error(handle, VORBIS_cant_find_last_page);
        }

        /* Save the current file position so we can restore it when done. */
        const int64_t restore_offset = get_file_offset(handle);

        const int64_t first_page_start = handle->p_first.page_start;

        /*
         * We need to find the last Ogg page in the file.  An Ogg page can
         * have up to 255*255 bytes of data, but typical sizes are in the
         * 4k-8k range; we don't want to waste time scanning through 64k
         * of data if we don't have to.  So we make the assumptions that
         * (A) the last page of the stream is less than 8k long and (B) the
         * capture pattern ("OggS") is unlikely to occur within the last
         * page of the stream, and we use the following algorithm:
         *
         * 1) Read the last 8k of the stream (or the entire stream if
         * it is less than 8k long).
         *
         * 2a) Starting from a position 4k from the end of the stream,
         * scan backward until we find an occurrence of the capture
         * pattern or reach the beginning of the data read in step 1.
         *
         * 2b) If no capture pattern was found in step 2a, start from
         * the same position and search forward instead.
         *
         * 2c) If no capture pattern was found in step 2b, fall back to
         * a linear search of the last 64k of the stream.
         *
         * 3) Check whether the capture pattern starts a new page.
         *
         * 4a) If it does, continue scanning pages after the end of that
         * page until the end of the stream is reached.
         *
         * 4b) Otherwise, fall back to a linear search of the last 64k of
         * the stream.
         */
        const int64_t stream_len = handle->stream_len - first_page_start;
        int64_t in_prev_page;
        if (stream_len >= 65536) {
            in_prev_page = handle->stream_len - 65536;
        } else {
            in_prev_page = first_page_start;
        }
        int64_t last_page_loc = -1;
        int64_t page_end = -1;
        bool last;

        uint8_t search_buf[8192];
        const int search_bufsize = sizeof(search_buf);
        if (stream_len > search_bufsize) {
            const int64_t search_start = handle->stream_len - search_bufsize;
            set_file_offset(handle, search_start);
            if (getn(handle, search_buf, search_bufsize)) {
                int capture_offset = -1;
                for (int i = search_bufsize/2-1; i >= 0; i--) {
                    if (search_buf[i  ]=='O' && search_buf[i+1]=='g'
                     && search_buf[i+2]=='g' && search_buf[i+3]=='S'
                     && search_buf[i+4]==0) {
                        capture_offset = i;
                        break;
                    }
                }
                if (capture_offset < 0) {
                    for (int i = search_bufsize/2; i < search_bufsize-27; i++) {
                        if (search_buf[i  ]=='O' && search_buf[i+1]=='g'
                         && search_buf[i+2]=='g' && search_buf[i+3]=='S'
                         && search_buf[i+4]==0) {
                            capture_offset = i;
                            break;
                        }
                    }
                }
                if (capture_offset >= 0) {
                    int page_len =
                        test_page(search_buf + capture_offset,
                                  sizeof(search_buf) - capture_offset);
                    if (page_len > 0) {
                        last_page_loc = search_start + capture_offset;
                        in_prev_page = last_page_loc+1;
                        page_end = last_page_loc + page_len;
                        last = ((search_buf[capture_offset+5] & 0x04) != 0);
                    }
                }
            }
        }

        if (last_page_loc < 0) {
            /* The optimistic search failed or was skipped.  Fall back to
             * a linear search of the last 64k of the stream. */
            set_file_offset(handle, in_prev_page);
            /* Check that we can actually find a page there. */
            if (!find_page(handle, &page_end, &last)) {
                handle->total_samples = -1;
                goto done;
            }
            last_page_loc = get_file_offset(handle);
        }

        /* Look for subsequent pages. */
        if (in_prev_page == last_page_loc
         && in_prev_page > first_page_start) {
            /* We happened to start exactly at a page boundary, so back up
             * one byte for the "in previous page" location. */
            in_prev_page--;
        }
        while (!last) {
            set_file_offset(handle, page_end);
            if (!find_page(handle, &page_end, &last)) {
                /* The last page didn't have the "last page" flag set.
                 * This probably means the file is truncated, but we go
                 * on anyway. */
                break;
            }
            in_prev_page = last_page_loc+1;
            last_page_loc = get_file_offset(handle);
        }

        /* Get the sample offset at the end of the last page. */
        set_file_offset(handle, last_page_loc+6);
        uint8_t buf[8];
        if (UNLIKELY(!getn(handle, buf, 8))) {
            goto done;
        }
        handle->total_samples = extract_64(buf);
        if (handle->total_samples == (uint64_t)-1) {
            /* Oops, the last page didn't have a sample offset! */
            goto done;
        }

        handle->p_last.page_start = last_page_loc;
        handle->p_last.page_end = page_end;
        handle->p_last.first_decoded_sample = handle->total_samples;
        handle->p_last.last_decoded_sample = handle->total_samples;
        handle->p_last.after_previous_page_start = in_prev_page;

      done:
        set_file_offset(handle, restore_offset);
    }  // if (!handle->total_samples)

    if (handle->total_samples == (uint64_t)-1) {
        return error(handle, VORBIS_cant_find_last_page);
    } else {
        return handle->total_samples;
    }
}

/*************************************************************************/
/*************************************************************************/
