/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_DECODE_H
#define NOGG_SRC_DECODE_DECODE_H

/*************************************************************************/
/*************************************************************************/

/**
 * vorbis_decode_initial:  Perform initial decoding steps for a packet.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     left_start_ret, left_end_ret, right_start_ret, right_end_ret: Pointers
 *         to variables to receive the frame's window parameters.
 *     mode_ret: Pointer to variable to receive the frame's mode index.
 * [Return value]
 *     True on success, false on error.
 */
#define vorbis_decode_initial INTERNAL(vorbis_decode_initial)
extern bool vorbis_decode_initial(stb_vorbis *handle, int *left_start_ret,
                                  int *left_end_ret, int *right_start_ret,
                                  int *right_end_ret, int *mode_ret);

/**
 * vorbis_decode_packet:  Decode a Vorbis packet into the internal PCM
 * buffers.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     len_ret: Pointer to variable to receive the length of the decoded
 *         frame.  Not modified on error.  May be NULL if the value is
 *         not needed.
 * [Return value]
 *     True on success, false on error.
 */
#define vorbis_decode_packet INTERNAL(vorbis_decode_packet)
extern bool vorbis_decode_packet(stb_vorbis *handle, int *len_ret);

/**
 * vorbis_decode_packet_direct:  Decode a Vorbis packet supplied by the
 * caller into the internal PCM buffers.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     packet: Pointer to packet data.
 *     packet_len: Length of packet, in bytes.
 *     len_ret: Pointer to variable to receive the length of the decoded
 *         frame.  Not modified on error.  May be NULL if the value is
 *         not needed.
 * [Return value]
 *     True on success, false on error.
 */
#define vorbis_decode_packet_direct INTERNAL(vorbis_decode_packet_direct)
extern bool vorbis_decode_packet_direct(
    stb_vorbis *handle, const void *packet, int32_t packet_len, int *len_ret);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_DECODE_H
