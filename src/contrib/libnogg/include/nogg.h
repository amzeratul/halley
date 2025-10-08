/*
 * libnogg: a decoder library for Ogg Vorbis streams
 * Copyright (c) 2014-2024 Andrew Church <achurch@achurch.org>
 *
 * This software may be copied and redistributed under certain conditions;
 * see the file "COPYING" in the source code distribution for details.
 * NO WARRANTY is provided with this software.
 */

#ifndef NOGG_H
#define NOGG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************/
/*********************** Data types and constants ************************/
/*************************************************************************/

/**
 * vorbis_t:  Type of a Vorbis decoder handle.  This is the object through
 * which all operations on a particular stream are performed.
 *
 * Note that API functions do _not_ check for a null handle pointer except
 * where explicitly documented.  Passing a null handle pointer to a
 * function which does not check for one will generally result in a
 * program crash due to dereferencing the null pointer.
 */
typedef struct vorbis_t vorbis_t;


/**
 * vorbis_callbacks_t:  Structure containing callbacks for reading from a
 * stream and managing dynamically-allocated memory, used with
 * vorbis_open_callbacks().  The "opaque" parameter to each of these
 * functions receives the argument passed to the "opaque" parameter to
 * vorbis_open_callbacks(), and it may thus be used to point to a file
 * handle, state block, or similar data structure for the stream data.
 *
 * For stream data access callbacks, the "current byte offset" is the
 * offset in bytes from the beginning of the stream of the next byte which
 * will be returned by a read() call.  An offset of zero indicates the
 * beginning of the stream, and an offset equal to length() indicates the
 * end of the stream (at which point read() will read no data and return 0).
 */
typedef struct vorbis_callbacks_t {

    /*------------------ Stream data access callbacks -------------------*/

    /* Return the length of the stream in bytes, or -1 if the stream is
     * not seekable.  This value is assumed to be constant for any given
     * stream.  If this function pointer is NULL, the stream is assumed to
     * be unseekable, and the tell() and seek() function pointers will be
     * ignored. */
    int64_t (*length)(void *opaque);

    /* Return the stream's current byte offset.  This function must be
     * provided if a length() function is provided, but it will only be
     * called on seekable streams. */
    int64_t (*tell)(void *opaque);

    /* Set the stream's current byte offset to the given value.  This
     * function must be provided if a length() function is provided, but it
     * will only be called on seekable streams, and the value of the offset
     * argument will always satisfy 0 <= offset <= length().  The operation
     * is assumed to succeed, though this does not imply that a subsequent
     * read operation must also succeed (for example, an error from the
     * physical storage layer may cause the next read() call to return no
     * data, and the library will correctly handle this case).  For streams
     * on which the seek operation itself could fail, the stream must be
     * reported as unseekable. */
    void (*seek)(void *opaque, int64_t offset);

    /* Read data from the stream, returning the number of bytes
     * successfully read.  The caller guarantees that length is nonnegative
     * (though it may be zero) and buffer points to a buffer with at least
     * length bytes of space.  A return value less than the requested
     * length is interpreted as an end-of-stream indication; libnogg does
     * not differentiate between end-of-stream and data access errors (such
     * as a read error from the physical storage layer), and the callback
     * must not return a negative number as an error indication.  For
     * seekable streams, libnogg uses the value returned by the length()
     * callback as the true stream length even if a short read occurs, so
     * if a transient error condition is subsequently resolved, the
     * remainder of the stream will still be readable (though a
     * vorbis_seek() may be required to reinitialize decoder state). */
    int32_t (*read)(void *opaque, void *buffer, int32_t length);

    /* Close the stream.  This function will be called exactly once for a
     * successfully opened stream, and no other functions will be called on
     * the stream once it has been closed.  If the open operation fails,
     * this function will not be called at all.  This function pointer may
     * be NULL if no close operation is required. */
    void (*close)(void *opaque);

    /*------------------- Memory management callbacks -------------------*/

    /* Allocate a block of memory, optionally (if align > 0) aligned to a
     * power-of-two address alignment.  This function, along with the
     * corresponding free() function below, can be used to implement a
     * custom memory allocator for a specific stream handle.  On success,
     * the function must return a block of memory suitably aligned for any
     * data type (like malloc()), or aligned as specified by the align
     * parameter.  Normally, these functions should be left at NULL, which
     * will cause libnogg to use the standard library's malloc() and free()
     * functions for this purpose. */
    void *(*malloc)(void *opaque, int32_t size, int32_t align);

    /* Free a block of memory allocated with the malloc() function above.
     * If the malloc() function pointer is non-NULL, this field must also
     * be non-NULL. */
    void (*free)(void *opaque, void *ptr);

} vorbis_callbacks_t;


/**
 * vorbis_error_t:  Type of error codes returned from vorbis_*() functions.
 */
typedef enum vorbis_error_t {
    /* No error has occurred.  This constant is guaranteed to have the
     * value zero in all future library versions. */
    VORBIS_NO_ERROR = 0,

    /*---- Error codes for API usage or initialization errors ----*/

    /* An invalid argument was passed to a function. */
    VORBIS_ERROR_INVALID_ARGUMENT = 1,
    /* The requested function is not supported in this build of the library. */
    VORBIS_ERROR_DISABLED_FUNCTION = 2,
    /* Insufficient system resources were available for the operation. */
    VORBIS_ERROR_INSUFFICIENT_RESOURCES = 3,
    /* vorbis_open_file() failed to open the requested file.  The global
     * variable errno indicates the specific error that occurred. */
    VORBIS_ERROR_FILE_OPEN_FAILED = 4,
    /* An invalid operation was attempted.*/
    VORBIS_ERROR_INVALID_OPERATION = 5,
    /* The runtime environment does not support the CPU instruction set
     * (including extensions) for which the library was compiled. */
    VORBIS_ERROR_NO_CPU_SUPPORT = 6,

    /*---- Error codes for stream input errors ----*/

    /* The stream is not a Vorbis stream or is corrupt. */
    VORBIS_ERROR_STREAM_INVALID = 101,
    /* A seek operation was attempted on an unseekable stream. */
    VORBIS_ERROR_STREAM_NOT_SEEKABLE = 102,
    /* A read operation attempted to read past the end of the stream
     * (for a packet-submission decoder, past the end of the packet). */
    VORBIS_ERROR_STREAM_END = 103,

    /*---- Error codes for decode-time errors ----*/

    /* An error occurred while initializing the Vorbis decoder. */
    VORBIS_ERROR_DECODE_SETUP_FAILED = 201,
    /* An unrecoverable error occurred while decoding audio data. */
    VORBIS_ERROR_DECODE_FAILED = 202,
    /* An error was detected in the stream, but the decoder was able to
     * recover and subsequent read operations may be attempted. */
    VORBIS_ERROR_DECODE_RECOVERED = 203,

} vorbis_error_t;


/**
 * VORBIS_OPTION_*:  Option flags for vorbis_open_*().
 */

/* Use the given number of bits (0-24) for direct lookup of Huffman codes.
 * More bits increases the number of codes which can be looked up without
 * a full search, but also requires more memory (2^n entries, where one
 * entry is 2 bytes).  A value of zero disables the accelerated lookup;
 * invalid values (greater than 24) are treated as the default.  The
 * default is 10. */
#define VORBIS_OPTION_FAST_HUFFMAN_LENGTH(n)    (1U << 5 | ((n) & 31))

/* Disable binary search of Huffman codes not found in the direct lookup
 * table, using a simple linear search instead.  This is a size/speed
 * tradeoff, reducing performance in exchange for not needing to store an
 * extra sorted copy of the Huffman table. */
#define VORBIS_OPTION_NO_HUFFMAN_BINARY_SEARCH  (1U << 6)

/* Disable precomputation of the result of scalar residue decoding.  This
 * is a size/speed tradeoff, reducing performance in exchange for not
 * needing to store the precomputed data. */
#define VORBIS_OPTION_DIVIDES_IN_RESIDUE        (1U << 7)

/* Disable conversion of lookup-format (lookup type 1) VQ codebooks to
 * literal format (lookup type 2).  This is a size/speed tradeoff, reducing
 * performance in exchange for a smaller memory footprint for lookup-format
 * codebooks. */
#define VORBIS_OPTION_DIVIDES_IN_CODEBOOK       (1U << 8)

/* Allow junk data between Ogg pages.  Without this option, the decoder
 * will report an error if a completed Ogg page is not immediately followed
 * by another page.  Note that even if this option is set, the decoder does
 * not perform CRC checks while decoding, so if the Ogg capture pattern
 * ("OggS") appears in the junk data, the decoder will become confused. */
#define VORBIS_OPTION_SCAN_FOR_NEXT_PAGE        (1U << 9)

/* Indicate that the caller will only request samples in 16-bit integer
 * format.  This improves performance and slightly reduces memory usage,
 * but calling vorbis_read_float() on a handle created with this option set
 * will fail. */
#define VORBIS_OPTION_READ_INT16_ONLY           (1U << 10)

/*************************************************************************/
/**************** Interface: Library version information *****************/
/*************************************************************************/

/**
 * nogg_version:  Return the version number of the library as a string
 * (for example, "1.2.3").
 *
 * [Return value]
 *     Library version number.
 */
extern const char *nogg_version(void);

/*************************************************************************/
/**************** Interface: Opening and closing streams *****************/
/*************************************************************************/

/**
 * vorbis_open_buffer:  Create a new stream handle for a stream whose
 * contents are stored in memory.
 *
 * [Parameters]
 *     buffer: Pointer to the buffer containing the stream data.
 *     length: Length of the stream data, in bytes.
 *     options: Decoder options (bitwise OR of VORBIS_OPTION_* flags).
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (always VORBIS_NO_ERROR on success).  May be NULL if
 *         the error code is not needed.
 * [Return value]
 *     Newly-created handle, or NULL on error.
 */
extern vorbis_t *vorbis_open_buffer(
    const void *buffer, int64_t length, unsigned int options,
    vorbis_error_t *error_ret);

/**
 * vorbis_open_callbacks:  Create a new stream handle for a stream whose
 * contents are accessed through a set of callbacks.
 *
 * This function will fail with VORBIS_ERROR_INVALID_ARGUMENT if the
 * callback set is incorrectly specified (no read function, or a length
 * function but no tell or seek function).
 *
 * [Parameters]
 *     callbacks: Set of callbacks to be used to access the stream data.
 *     opaque: Opaque pointer value passed through to the callbacks.
 *     options: Decoder options (bitwise OR of VORBIS_OPTION_* flags).
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (always VORBIS_NO_ERROR on success).  May be NULL if
 *         the error code is not needed.
 * [Return value]
 *     Newly-created handle, or NULL on error.
 */
extern vorbis_t *vorbis_open_callbacks(
    vorbis_callbacks_t callbacks, void *opaque, unsigned int options,
    vorbis_error_t *error_ret);

/**
 * vorbis_open_file:  Create a new stream handle for a stream whose
 * contents will be read from a file on the filesystem.
 *
 * If stdio support was disabled when the library was built, this function
 * will always fail with VORBIS_ERROR_DISABLED_FUNCTION.
 *
 * [Parameters]
 *     path: Pathname of the file from which the stream is to be read.
 *     options: Decoder options (bitwise OR of VORBIS_OPTION_* flags).
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (always VORBIS_NO_ERROR on success).  May be NULL if
 *         the error code is not needed.
 * [Return value]
 *     Newly-created handle, or NULL on error.
 */
extern vorbis_t *vorbis_open_file(
    const char *path, unsigned int options, vorbis_error_t *error_ret);

/**
 * vorbis_open_packet:  Create a new stream handle for a stream which
 * will be submitted packet-by-packet directly to the decoder.
 *
 * Decoders created using this function will not attempt to parse an Ogg
 * stream; instead, the caller is expected to submit sequential Vorbis
 * packets through the vorbis_submit_packet() function.  Audio data can be
 * read through the usual vorbis_read_int16() and vorbis_read_float()
 * functions, which will return 0 with the error VORBIS_ERROR_STREAM_END
 * once all data has been read from the packet.  The caller must read all
 * audio data from one packet before submitting the next packet.
 *
 * The Vorbis header packets should not be submitted via
 * vorbis_submit_packet(); instead, the identification and setup header
 * packets should be passed directly to this function.  (The comment
 * header packet is not used by libnogg and may be ignored by the caller.)
 *
 * The following functions will not work or have limited functionality
 * with packet-submission decoders:
 *    - vorbis_length() (always returns -1)
 *    - vorbis_bitrate() (returns 0 if no bitrate found in the ID packet)
 *    - vorbis_seek() (always returns false)
 *
 * This interface is useful when decoding Vorbis data stored in something
 * other than an Ogg container, such as audio from a WebM file.
 *
 * [Parameters]
 *     id_packet: Pointer to the Vorbis identification packet data.
 *     id_packet_len: Length of the Vorbis identification packet, in bytes.
 *     setup_packet: Pointer to the Vorbis setup packet data.
 *     setup_packet_len: Length of the Vorbis setup packet, in bytes.
 *     callbacks: Set of callbacks to be used for memory allocation.
 *         Only the "malloc" and "free" fields are used.
 *     opaque: Opaque pointer value passed through to the callbacks.
 *     options: Decoder options (bitwise OR of VORBIS_OPTION_* flags).
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (always VORBIS_NO_ERROR on success).  May be NULL if
 *         the error code is not needed.
 * [Return value]
 *     Newly-created handle, or NULL on error.
 */
extern vorbis_t *vorbis_open_packet(
    const void *id_packet, int32_t id_packet_len,
    const void *setup_packet, int32_t setup_packet_len,
    vorbis_callbacks_t callbacks, void *opaque,
    unsigned int options, vorbis_error_t *error_ret);

/**
 * vorbis_close:  Close a handle, freeing all associated resources.
 * After calling this function, the handle is no longer valid.
 *
 * [Parameters]
 *     handle: Handle to close.  If NULL, this function does nothing.
 */
extern void vorbis_close(vorbis_t *handle);

/*************************************************************************/
/***************** Interface: Getting stream information *****************/
/*************************************************************************/

/**
 * vorbis_channels:  Return the number of channels in the given stream.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 * [Return value]
 *     Number of audio channels (always a positive value).
 */
extern int vorbis_channels(const vorbis_t *handle);

/**
 * vorbis_rate:  Return the sampling rate of the given stream.
 *
 * Note that the returned value is unsigned, to match the Vorbis
 * specification.  Sampling rates of 2^31 or greater are not likely to
 * occur in real-world streams but are permitted by the specification,
 * so callers should take care to handle such values correctly.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 * [Return value]
 *     Audio sampling rate (always a positive value).
 */
extern uint32_t vorbis_rate(const vorbis_t *handle);

/**
 * vorbis_length:  Return the number of samples in the given stream, if
 * known.
 *
 * When first called for a given stream (if no seek operations have
 * previously been performed on the stream), this function must read from
 * the stream and therefore has a moderate amount of overhead.  The value
 * is cached so that subsequent calls on the same stream will return
 * immediately.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 * [Return value]
 *     Length of stream in samples, or -1 if the length is not known.
 */
extern int64_t vorbis_length(const vorbis_t *handle);

/**
 * vorbis_bitrate:  Return the average data rate of the given stream, if
 * known.
 *
 * If the stream is seekable and has nonzero duration, this function
 * returns the average data rate over the entire stream, specifically
 * (data length in bits / duration in seconds), rounded to the nearest
 * integer.  This incurs the same overhead as vorbis_length().  If the
 * returned value would be greater than 2^31-1, it is clamped to 2^31-1;
 * if (stream length in bits * sampling rate + half of stream duration in
 * samples) exceeds 2^63-1, 2^31-1 is returned regardless of the stream
 * duration.
 *
 * If the stream is not seekable or has zero duration, then:
 *    - If the stream header indicates a positive nominal bitrate for the
 *      stream, that value is returned.
 *    - Otherwise, if the stream header indicates positive minimum and
 *      maximum bitrates for the stream, the average of those two values
 *      is returned, rounded up to the next integer if non-integral.
 *    - Otherwise, zero is returned.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 * [Return value]
 *     Stream data rate in bits per second, or 0 if the data rate is not
 *     known.
 */
extern int32_t vorbis_bitrate(const vorbis_t *handle);

/*************************************************************************/
/********** Interface: Setting and getting the decode position ***********/
/*************************************************************************/

/**
 * vorbis_seek:  Seek to the given sample index in the stream.  An index
 * of 0 indicates the first sample in the stream.
 *
 * Seeking to an arbitrary position in an Ogg Vorbis stream requires
 * knowing the stream length.  Consequently, this function implicitly
 * calls vorbis_length(), and the caveat about the first call to that
 * function for a given stream applies here as well.
 *
 * This function is only guaranteed to work correctly for a compliant Ogg
 * Vorbis stream.  The function attempts to locate the correct position
 * (i.e., the same position that would be reached by reading the given
 * number of samples from the beginning of the stream) even in broken or
 * corrupt streams, but depending on how the stream is broken, this
 * function may seek to an incorrect position or fail completely.
 *
 * On failure (except when called on an unseekable stream), the state of
 * the decoder is undefined until a subsequent seek operation succeeds.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     position: Position to seek to, in samples.
 * [Return value]
 *     True (nonzero) on success, false (zero) on failure.
 */
extern int vorbis_seek(vorbis_t *handle, int64_t position);

/**
 * vorbis_tell:  Return the current decode position, which is the index of
 * the next sample to be returned by one of the vorbis_read_*() functions.
 * An index of 0 indicates the first sample in the stream.
 *
 * The value returned by this function tracks the sample position encoded
 * in the stream.  Normally this is identical to the total number of
 * samples returned from read operations (if no seeks have been performed),
 * but if some audio data is dropped due to corruption, the return value
 * past that point will be the offset that would have been returned for
 * the uncorrupted stream.  Seek operations also follow the encoded sample
 * position, so seeking to the value returned from this function will
 * restore the decode position to the same place even if an earlier part
 * of the stream was dropped.  (However, seek and tell operations may be
 * inaccurate very close to a dropped packet.)
 *
 * [Parameters]
 *     handle: Handle to operate on.
 * [Return value]
 *     Current decode position, in samples.
 */
extern int64_t vorbis_tell(const vorbis_t *handle);

/*************************************************************************/
/*********************** Interface: Reading frames ***********************/
/*************************************************************************/

/**
 * vorbis_submit_packet:  Submit the next Vorbis packet in the stream to a
 * packet-submission decoder.  The packet will be immediately decoded.
 *
 * This function may only be called when all audio data from the preceding
 * frame (if any) has been read with vorbis_read_int16() or
 * vorbis_read_float().  Calling this function while unread audio data is
 * pending will result in a VORBIS_ERROR_INVALID_OPERATION error, and the
 * submitted packet will be ignored.
 *
 * This function has no effect (and always returns false with a
 * VORBIS_ERROR_INVALID_OPERATION error) when called on a decoder which
 * was not created with vorbis_open_packet().
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     packet: Pointer to packet data.
 *     packet_len: Length of packet, in bytes.
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (always VORBIS_NO_ERROR on success).  May be NULL if
 *         the error code is not needed.
 * [Return value]
 *     True on success, false on error.
 */
extern int vorbis_submit_packet(vorbis_t *handle, const void *packet,
                                int32_t packet_len, vorbis_error_t *error_ret);

/**
 * vorbis_read_int16:  Decode and return up to the given number of PCM
 * samples as 16-bit signed integers in the range [-32767,+32767].
 * Multichannel audio data is stored with channels interleaved.
 *
 * If an error occurs, the error code will be stored in *error_ret if that
 * pointer is not NULL.  In the case of VORBIS_ERROR_DECODE_RECOVERED, the
 * return value indicates the number of samples decoded before the error
 * was encountered (which may be zero), and the next read call will return
 * samples starting from the point at which the decoder recovered from the
 * error.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     buf: Buffer into which to store decoded audio data.
 *     len: Number of samples (per channel) to read.
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (or VORBIS_NO_ERROR if no error was encountered).
 *         May be NULL if the error code is not needed.
 * [Return value]
 *     Number of samples successfully read.
 */
extern int32_t vorbis_read_int16(
    vorbis_t *handle, int16_t *buf, int32_t len, vorbis_error_t *error_ret);

/**
 * vorbis_read_float:  Decode and return up to the given number of PCM
 * samples as single-precision floating point values.  Multichannel audio
 * data is stored with channels interleaved.
 *
 * Sample values are normally in the range [-1.0,+1.0], but this function
 * does not clamp the returned data, so callers should be prepared to
 * handle sample values outside that range.
 *
 * If an error occurs, the error code will be stored in *error_ret if that
 * pointer is not NULL.  In the case of VORBIS_ERROR_DECODE_RECOVERED, the
 * return value indicates the number of samples decoded before the error
 * was encountered (which may be zero), and the next read call will return
 * samples starting from the point at which the decoder recovered from the
 * error.
 *
 * If the decoder was created with the VORBIS_OPTION_READ_INT16_ONLY option
 * set, this function will fail with VORBIS_ERROR_INVALID_ARGUMENT.
 *
 * [Parameters]
 *     handle: Handle to operate on.
 *     buf: Buffer into which to store decoded audio data.
 *     len: Number of samples (per channel) to read.
 *     error_ret: Pointer to variable to receive the error code from the
 *         operation (or VORBIS_NO_ERROR if no error was encountered).
 *         May be NULL if the error code is not needed.
 * [Return value]
 *     Number of samples successfully read.
 */
extern int32_t vorbis_read_float(
    vorbis_t *handle, float *buf, int32_t len, vorbis_error_t *error_ret);

/*************************************************************************/
/*************************************************************************/

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // NOGG_H
