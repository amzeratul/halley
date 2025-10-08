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
#include "src/util/open.h"

#include <stdlib.h>
#include <string.h>

/*************************************************************************/
/*************************** Interface routine ***************************/
/*************************************************************************/

vorbis_t *vorbis_open_packet(
    const void *id_packet, int32_t id_packet_len,
    const void *setup_packet, int32_t setup_packet_len,
    vorbis_callbacks_t callbacks, void *opaque,
    unsigned int options, vorbis_error_t *error_ret)
{
    return open_common(
        &(open_params_t){.callbacks = &callbacks,
                         .callback_data = opaque,
                         .options = options,
                         .packet_mode = true,
                         .id_packet = id_packet,
                         .id_packet_len = id_packet_len,
                         .setup_packet = setup_packet,
                         .setup_packet_len = setup_packet_len},
        error_ret);
}

/*************************************************************************/
/*************************************************************************/
