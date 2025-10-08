/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_DECODE_SETUP_H
#define NOGG_SRC_DECODE_SETUP_H

/*************************************************************************/
/*************************************************************************/

/**
 * start_decoder:  Read the header for a stream and perform all setup
 * necessary for decoding.
 *
 * The identification and setup header packets are required if
 * handle->packet_mode is true, and ignored otherwise.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     id_packet: Pointer to the Vorbis identification packet data.
 *     id_packet_len: Length of the Vorbis identification packet, in bytes.
 *     setup_packet: Pointer to the Vorbis setup packet data.
 *     setup_packet_len: Length of the Vorbis setup packet, in bytes.
 * [Return value]
 *     True on success, false on error.
 */
#define start_decoder INTERNAL(start_decoder)
extern bool start_decoder(
    stb_vorbis *handle, const void *id_packet, int32_t id_packet_len,
    const void *setup_packet, int32_t setup_packet_len);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_DECODE_SETUP_H
