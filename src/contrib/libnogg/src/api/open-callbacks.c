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


vorbis_t *vorbis_open_callbacks(
    vorbis_callbacks_t callbacks, void *opaque, unsigned int options,
    vorbis_error_t *error_ret)
{
    return open_common(
        &(open_params_t){.callbacks = &callbacks,
                         .callback_data = opaque,
                         .options = options,
                         .packet_mode = false},
        error_ret);
}
