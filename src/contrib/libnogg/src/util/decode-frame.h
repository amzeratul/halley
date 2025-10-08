/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_UTIL_DECODE_FRAME_H
#define NOGG_SRC_UTIL_DECODE_FRAME_H

/*************************************************************************/
/*************************************************************************/

/**
 * decode_frame:  Decode the next frame from the stream (or the given
 * packet, for a packet-mode decoder) and store the decoded data in
 * decode_buf.
 *
 * On return from this function, the handle's decode_buf_pos field will
 * always be set to zero.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     packet: Pointer to packet data.  Required for packet-mode decoders;
 *         ignored otherwise.
 *     packet_len: Length of packet, in bytes.  Required for packet-mode
 *         decoders; ignored otherwise.
 * [Return value]
 *     Result of the operation (VORBIS_NO_ERROR or a VORBIS_ERROR_* code).
 */
#define decode_frame INTERNAL(decode_frame)
extern vorbis_error_t decode_frame(vorbis_t *handle, const void *packet,
                                   int32_t packet_len);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_UTIL_DECODE_FRAME_H
