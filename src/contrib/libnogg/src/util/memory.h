/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_SRC_UTIL_MEMORY_H
#define NOGG_SRC_UTIL_MEMORY_H

/*************************************************************************/
/*************************************************************************/

/**
 * mem_alloc:  Allocate a block of memory using the stream's allocator.
 *
 * [Parameters]
 *     handle: Stream handle for which memory is being allocated.
 *     size: Number of bytes to allocate.
 *     align: Required address alignment in bytes (must be a power of two),
 *         or zero for the default alignment.
 * [Return value]
 *     Pointer to the allocated block, or NULL on allocation failure.
 */
#define mem_alloc INTERNAL(mem_alloc)
extern void *mem_alloc(vorbis_t *handle, int32_t size, int32_t align);

/**
 * mem_free:  Free a block of memory allocated with mem_alloc().
 *
 * [Parameters]
 *     handle: Stream handle.
 *     ptr: Memory block to free.
 */
#define mem_free INTERNAL(mem_free)
extern void mem_free(vorbis_t *handle, void *ptr);

/**
 * alloc_channel_array:  Allocate an array of "channels" sub-arrays, with
 * each sub-array having "size" bytes of storage.  The entire set of arrays
 * can be freed by simply calling mem_free() on the returned pointer.
 *
 * The return value is conceptually "<T> **", but the function is typed
 * as "void *" so the return value does not need an explicit cast to the
 * target data type.
 *
 * [Parameters]
 *     handle: Stream handle.
 *     channels: Number of channels (sub-arrays) required.
 *     size: Number of bytes of storage to allocate for each sub-array.
 *     align: Required alignment for each array, as for mem_alloc().
 * [Return value]
 *     Pointer to the top-level array, or NULL on allocation failure.
 */
#define alloc_channel_array INTERNAL(alloc_channel_array)
extern void *alloc_channel_array(vorbis_t *handle, int channels, int32_t size,
                                 int32_t align);

/*************************************************************************/
/*************************************************************************/

#endif  // NOGG_SRC_UTIL_MEMORY_H
