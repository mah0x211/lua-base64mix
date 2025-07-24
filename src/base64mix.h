/**
 *  base64mix.h
 *  Created by Masatoshi Teruya on 14/10/23.
 *
 *  Copyright 2014-present Masatoshi Fukunaga. All rights reserved.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */

#ifndef BASE64MIX_H
#define BASE64MIX_H

#include <errno.h>
#include <stdlib.h>

/* Get SIZE_MAX.  */
#ifdef __BIONIC__
# include <limits.h>
#else
# include <stdint.h>
#endif
// https://lists.gnu.org/archive/html/bug-gnulib/2013-01/msg00094.html
// fix include for SIZE_MAX with Bionic libc

/**
 * @name Encoding Tables
 * @{
 */

/** @brief Standard Base64 encoding table (RFC 4648)
 *  Uses '+' and '/' as the last two characters, with '=' padding */
static const unsigned char BASE64MIX_STDENC[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

/** @brief URL-safe Base64 encoding table (RFC 4648)
 *  Uses '-' and '_' as the last two characters, without padding */
static const unsigned char BASE64MIX_URLENC[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_'};

/** @} */

/**
 * @brief Calculate encoded length for base64 encoding
 *
 * @param len Length of input data to encode
 *
 * @return Required buffer size for encoded output
 *
 * @note Always returns size for padded base64 (standard format)
 * @note This ensures sufficient buffer for both standard and URL-safe formats
 * @note Returns SIZE_MAX on overflow
 */
static inline size_t b64m_encoded_len(size_t len)
{
    // Calculate number of 3-byte blocks
    size_t block = len / 3 + (len % 3 != 0);

    if (block > (SIZE_MAX / 4)) {
        return SIZE_MAX; // Indicate overflow error
    }

    // Each 3-byte block becomes 4 characters in base64
    return block * 4;
}

/**
 * @brief Encode binary data to base64 string using user-provided buffer
 *
 * @param src Input binary data to encode (must not be NULL)
 * @param srclen Length of input data
 * @param dst Output buffer for encoded string (must not be NULL)
 * @param dstlen Size of output buffer
 * @param enctbl Encoding table (BASE64MIX_STDENC or BASE64MIX_URLENC)
 *
 * @return Length of encoded string (excluding null terminator), or SIZE_MAX on
 * error
 *
 * @errno EINVAL - Invalid arguments (NULL pointers)
 * @errno ENOSPC - Output buffer too small
 *
 * @note Zero-allocation version: uses caller-provided buffer
 * @note Buffer size can be calculated with b64m_encoded_len()
 * @note Result is always null-terminated
 */
static inline size_t b64m_encode_to_buffer(const char *src, size_t srclen,
                                           char *dst, size_t dstlen,
                                           const unsigned char enctbl[])
{
    const uint8_t *cur = (const uint8_t *)src;
    unsigned char *ptr = (unsigned char *)dst;
    size_t remain      = 0;
    size_t i           = 0;

    // Validate input parameters
    if (!src || !dst || !enctbl) {
        errno = EINVAL;
        return SIZE_MAX;
    }

    // dstlen must include +1 space for null terminator
    if (dstlen < (b64m_encoded_len(srclen) + 1)) {
        errno = ENOSPC;
        return SIZE_MAX;
    }

    // Handle empty input
    if (srclen == 0) {
        *dst = '\0';
        return 0;
    }

    // Process complete 3-byte groups with 4-block unrolling optimization

#define ENCODE_BLOCK(v, out)                                                   \
    do {                                                                       \
        (out)[0] = enctbl[(v >> 18) & 0x3f];                                   \
        (out)[1] = enctbl[(v >> 12) & 0x3f];                                   \
        (out)[2] = enctbl[(v >> 6) & 0x3f];                                    \
        (out)[3] = enctbl[v & 0x3f];                                           \
    } while (0)

    i = 0;
    // Process 4 blocks (12 bytes -> 16 chars) at a time for better performance
    size_t blocks_4 =
        (srclen / 12) * 12; // Number of bytes in complete 4-block groups
    for (; i < blocks_4; i += 12) {
        // Process 4 blocks simultaneously
        uint32_t val0 =
            ((uint32_t)cur[0] << 16) | ((uint32_t)cur[1] << 8) | cur[2];
        uint32_t val1 =
            ((uint32_t)cur[3] << 16) | ((uint32_t)cur[4] << 8) | cur[5];
        uint32_t val2 =
            ((uint32_t)cur[6] << 16) | ((uint32_t)cur[7] << 8) | cur[8];
        uint32_t val3 =
            ((uint32_t)cur[9] << 16) | ((uint32_t)cur[10] << 8) | cur[11];

        ENCODE_BLOCK(val0, ptr);
        ENCODE_BLOCK(val1, ptr + 4);
        ENCODE_BLOCK(val2, ptr + 8);
        ENCODE_BLOCK(val3, ptr + 12);

        ptr += 16; // Move pointer forward by 16 bytes
        cur += 12; // Move input pointer forward by 12 bytes
    }

    // Process remaining single blocks (3 bytes -> 4 chars)
    for (size_t n = (srclen / 3) * 3; i < n; i += 3) {
        uint32_t val =
            ((uint32_t)cur[0] << 16) | ((uint32_t)cur[1] << 8) | cur[2];
        ENCODE_BLOCK(val, ptr);
        ptr += 4;
        cur += 3;
    }

#undef ENCODE_BLOCK

    // Handle remaining bytes
    remain = srclen - i;
    if (remain > 0) {
        // Add the remaining small block
        uint32_t val = (uint32_t)cur[0] << 16;
        if (remain == 2) {
            // If we have 2 bytes left, shift the second byte
            val |= (uint32_t)cur[1] << 8;
        }

        // Encode the remaining bytes
        *ptr++ = enctbl[(val >> 18) & 0x3fU];
        *ptr++ = enctbl[(val >> 12) & 0x3fU];

        // Add remaining characters and padding
        if (remain == 2) {
            *ptr++ = enctbl[(val >> 6) & 0x3fU];
            if (enctbl == BASE64MIX_STDENC) {
                *ptr++ = '=';
            }
        } else if (remain == 1 && enctbl == BASE64MIX_STDENC) {
            *ptr++ = '=';
            *ptr++ = '=';
        }
    }

    // Null terminate
    *ptr = '\0';

    return (size_t)(ptr - (unsigned char *)dst);
}

/**
 * @name Convenience Macros for Buffer Encoding
 * @{
 */

/** @brief Encode to user buffer using standard Base64 format
 *  @param src Input binary data
 *  @param srclen Input data length
 *  @param dst Output buffer
 *  @param dstlen Output buffer size
 *  @return Length of encoded string or SIZE_MAX on error */
#define b64m_encode_to_buffer_std(src, srclen, dst, dstlen)                    \
    b64m_encode_to_buffer(src, srclen, dst, dstlen, BASE64MIX_STDENC)

/** @brief Encode to user buffer using URL-safe Base64 format
 *  @param src Input binary data
 *  @param srclen Input data length
 *  @param dst Output buffer
 *  @param dstlen Output buffer size
 *  @return Length of encoded string or SIZE_MAX on error */
#define b64m_encode_to_buffer_url(src, srclen, dst, dstlen)                    \
    b64m_encode_to_buffer(src, srclen, dst, dstlen, BASE64MIX_URLENC)

/** @} */

/**
 * @brief Encode binary data to base64 string
 *
 * @param src Input binary data to encode (must not be NULL)
 * @param len Input/Output: input data length -> encoded string length
 * @param enctbl Encoding table (BASE64MIX_STDENC or BASE64MIX_URLENC)
 *
 * @return Allocated base64 string (caller must free), or NULL on error
 *
 * @errno EINVAL - Invalid arguments (NULL pointers)
 * @errno ERANGE - Input size too large (overflow protection)
 * @errno ENOMEM - Memory allocation failure
 *
 * @note Empty input (len=0) returns empty string, not NULL
 * @note Standard encoding adds padding, URL-safe encoding does not
 * @note Result is always null-terminated for safe string handling
 */
static inline char *b64m_encode(const char *src, size_t *len,
                                const unsigned char enctbl[])
{
    char *res     = NULL;
    size_t buflen = 0;

    // Reset errno before allocation
    errno = 0;

    // Validate input parameters
    if (!src || !len || !enctbl) {
        errno = EINVAL;
        return NULL;
    }

    // Calculate required buffer size using zero-allocation helper
    buflen = b64m_encoded_len(*len);
    // Check for overflow (buflen of 0 indicates overflow)
    if (buflen == SIZE_MAX) {
        errno = ERANGE;
        return NULL;
    }

    // Allocate buffer
    buflen += 1; // +1 for null terminator
    if ((res = malloc(buflen))) {
        // Use zero-allocation version to do the actual encoding
        // Update length with actual encoded length
        // NOTE: In this case, we do not need to check for SIZE_MAX because
        // b64m_encoded_len() already ensures that the buffer size is valid.
        *len = b64m_encode_to_buffer(src, *len, res, buflen, enctbl);
    }
    return res;
}
/**
 * @name Convenience Macros
 * @{
 */

/** @brief Encode using standard Base64 format (with padding)
 *  @param src Input binary data
 *  @param len Input/Output length pointer
 *  @return Encoded string or NULL on error */
#define b64m_encode_std(src, len) b64m_encode(src, len, BASE64MIX_STDENC)

/** @brief Encode using URL-safe Base64 format (without padding)
 *  @param src Input binary data
 *  @param len Input/Output length pointer
 *  @return Encoded string or NULL on error */
#define b64m_encode_url(src, len) b64m_encode(src, len, BASE64MIX_URLENC)

/** @} */

/**
 * @name Decoding Tables
 * @{
 */

/** @brief Standard Base64 decoding table
 *  @note Uses 0xFF for invalid characters. Valid Base64 values are 0-63, so 255
 * is safely distinguishable.
 */
static const unsigned char BASE64MIX_STDDEC[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // +            /
    62, 0xFF, 0xFF, 0xFF, 63,
    // 0-9
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF,
    // A-Z
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // a-z
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/** @brief URL-safe Base64 decoding table
 *  @note Uses 0xFF for invalid characters. Accepts '-' and '_' instead of '+'
 * and '/'. */
static const unsigned char BASE64MIX_URLDEC[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // -
    62, 0xFF, 0xFF,
    // 0-9
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    //   :   ;   <   =   >   ?   @
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // A-Z
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 0xFF, 0xFF, 0xFF, 0xFF,
    // _
    63, 0xFF,
    // a-z
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

/** @brief Mixed format Base64 decoding table (handles both standard and
 * URL-safe)
 *  @note Uses 0xFF for invalid characters. Accepts both '+' and '-' at position
 * 62, both '/' and '_' at position 63. */
static const unsigned char BASE64MIX_DEC[256] = {
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    // +     -      /
    62, 0xFF, 62, 0xFF, 63,
    // 0-9
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF,
    // A-Z
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
    21, 22, 23, 24, 25, 0xFF, 0xFF, 0xFF, 0xFF,
    // _
    63, 0xFF,
    // a-z
    26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44,
    45, 46, 47, 48, 49, 50, 51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#undef FF

/** @} */

/**
 * @brief Calculate decoded length for base64 decoding
 *
 * @param enclen Length of base64 encoded string
 *
 * @return Maximum buffer size needed for decoded output (excluding null
 * terminator)
 *
 * @note This calculates the maximum possible decoded length
 * @note Actual decoded length may be smaller due to padding
 * @note Use this for buffer allocation in zero-allocation decoding
 */
static inline size_t b64m_decoded_len(size_t enclen)
{
    // Base64 decoding: 4 input chars -> 3 output bytes (maximum)
    // For buffer allocation, we use a simple calculation that ensures we have
    // enough space. This may slightly overestimate but guarantees sufficient
    // buffer size.
    return (enclen * 3) / 4;
}

/**
 * @brief Decode base64 string to binary data using user-provided buffer
 *
 * @param src Input base64 string to decode (must not be NULL)
 * @param srclen Length of input base64 string
 * @param dst Output buffer for decoded binary data (must not be NULL)
 * @param dstlen Size of output buffer
 * @param dectbl Decoding table (BASE64MIX_STDDEC, BASE64MIX_URLDEC, or
 * BASE64MIX_DEC)
 *
 * @return Length of decoded data (excluding null terminator), or SIZE_MAX on
 * error
 *
 * @errno EINVAL - Invalid arguments (NULL pointers)
 * @errno EINVAL - Invalid base64 character encountered
 * @errno EINVAL - Invalid padding format (non-'=' after '=')
 * @errno ENOSPC - Output buffer too small
 * @errno EILSEQ - Illegal byte sequence (non-zero ignored bits in incomplete
 * groups)
 *
 * @note Zero-allocation version: uses caller-provided buffer
 * @note Buffer size can be calculated with b64m_decoded_len()
 * @note Result is always null-terminated for safety
 * @note Incomplete groups (1 char) are silently ignored as invalid
 */
static inline size_t b64m_decode_to_buffer(const char *src, size_t srclen,
                                           char *dst, size_t dstlen,
                                           const unsigned char dectbl[])
{
    uint8_t *ptr       = (uint8_t *)dst;
    const uint8_t *cur = (const uint8_t *)src;
    const uint8_t *end = NULL;
    uint8_t npad       = 0; // Number of padding characters at the end
    size_t len         = 0;

    // Validate input parameters
    // srclen % 4 must not be 1 (invalid base64)
    //  - 3 characters are valid (with padding)
    //  - 2 characters are valid (with padding)
    //  - 1 character is invalid (cannot decode to bytes)
    //  - 0 characters is valid (empty input)
    if (!src || !dst || !dectbl || (srclen % 4 == 1)) {
        errno = EINVAL;
        return SIZE_MAX;
    }
    end = cur + srclen;

    // Check if we have enough space (including null terminator)
    if (dstlen < b64m_decoded_len(srclen) + 1) {
        errno = ENOSPC;
        return SIZE_MAX;
    }

    // Handle empty input
    if (srclen == 0) {
        *dst = '\0';
        return 0;
    }

    // Optimized single-pass padding calculation and validation
    // Scan from end to beginning once, validating as we go
    // Single pass: count and validate padding characters from the end
    while (cur < end && end[-1] == '=') {
        end--;
        npad++;
        // Early exit on too many padding characters
        if ((srclen - (size_t)(end - cur)) > 2) {
            errno = EINVAL; // Too many padding characters
            return SIZE_MAX;
        }
    }
    // If src has padding but the length is not a multiple of 4, it's invalid
    if (npad && srclen % 4) {
        errno = EINVAL;
        return SIZE_MAX;
    }

    // At this point, characters from effective_len to srclen are all '='
    // No need for additional validation loop - single pass guaranteed
    // correctness

#define REMAINING_CHARS() (size_t)(end - cur)

    // Process 8-character blocks (8 chars -> 6 bytes in one operation)
    len = REMAINING_CHARS();
    for (const uint8_t *tail = cur + (len / 8) * 8; cur < tail; cur += 8) {
        // Decode all 8 characters at once
        uint8_t d0 = dectbl[cur[0]];
        uint8_t d1 = dectbl[cur[1]];
        uint8_t d2 = dectbl[cur[2]];
        uint8_t d3 = dectbl[cur[3]];
        uint8_t d4 = dectbl[cur[4]];
        uint8_t d5 = dectbl[cur[5]];
        uint8_t d6 = dectbl[cur[6]];
        uint8_t d7 = dectbl[cur[7]];

        // Check for invalid characters using OR operation
        if ((d0 | d1 | d2 | d3 | d4 | d5 | d6 | d7) > 63) {
            errno = EINVAL;
            return SIZE_MAX;
        }

        // Combine 8 characters into a 64-bit value
        uint64_t v = ((uint64_t)d0 << 58) | ((uint64_t)d1 << 52) |
                     ((uint64_t)d2 << 46) | ((uint64_t)d3 << 40) |
                     ((uint64_t)d4 << 34) | ((uint64_t)d5 << 28) |
                     ((uint64_t)d6 << 22) | ((uint64_t)d7 << 16);

        // Extract 6 bytes from the 64-bit value
        // Now we can use cleaner shift amounts
        ptr[0] = (uint8_t)(v >> 56);
        ptr[1] = (uint8_t)(v >> 48);
        ptr[2] = (uint8_t)(v >> 40);
        ptr[3] = (uint8_t)(v >> 32);
        ptr[4] = (uint8_t)(v >> 24);
        ptr[5] = (uint8_t)(v >> 16);
        // Move pointers forward
        ptr += 6;
    }

    // Process remaining 4-character blocks
    len = (size_t)(end - cur);
    for (const uint8_t *tail = cur + (len / 4) * 4; cur < tail; cur += 4) {
        // Decode 4 characters at once
        uint8_t d1 = dectbl[cur[0]];
        uint8_t d2 = dectbl[cur[1]];
        uint8_t d3 = dectbl[cur[2]];
        uint8_t d4 = dectbl[cur[3]];

        // Check if any character is invalid (single bitwise OR operation)
        if ((d1 | d2 | d3 | d4) > 63) {
            errno = EINVAL;
            return SIZE_MAX;
        }

        // Direct decode: 4 chars (24 bits) -> 3 bytes
        uint32_t v = ((uint32_t)d1 << 26) | ((uint32_t)d2 << 20) |
                     ((uint32_t)d3 << 14) | ((uint32_t)d4 << 8);
        // Extract 3 bytes from the 32-bit value
        ptr[0] = (uint8_t)(v >> 24);
        ptr[1] = (uint8_t)(v >> 16);
        ptr[2] = (uint8_t)(v >> 8);
        // Move pointers forward
        ptr += 3;
    }

    // Handle remaining characters (1-3 chars) directly without accumulator
    // This eliminates conditional branching in loops and reduces overhead
    len = REMAINING_CHARS();
    if (len >= 3) {
        // Process 3 characters directly -> 2 bytes output
        // Batch validate all 3 characters first
        uint8_t d0 = dectbl[cur[0]];
        uint8_t d1 = dectbl[cur[1]];
        uint8_t d2 = dectbl[cur[2]];

        // Check if any character is invalid (single bitwise OR operation)
        if ((d0 | d1 | d2) > 63) {
            errno = EINVAL;
            return SIZE_MAX;
        }

        // Check RFC 4648 compliance: last 2 bits must be 0 for 3-char groups
        if ((d2 & 0x03) != 0) {
            errno = EILSEQ; // Illegal byte sequence - data loss would occur
            return SIZE_MAX;
        }

        // Direct decode: 3 chars (18 bits) -> 2 bytes + 2 padding bits
        // [AAAAAA][BBBBBB][CCCCCC] -> [AAAAAA|BB][BBBB|CCCC|CC]
        uint32_t val = ((uint32_t)d0 << 12) | ((uint32_t)d1 << 6) | d2;
        ptr[0]       = (uint8_t)(val >> 10); // First byte: [AAAAAA|BB]
        ptr[1]       = (uint8_t)(val >> 2);  // Second byte: [BBBB|CCCC]
        ptr += 2;
        cur += 3;
        len = REMAINING_CHARS();
    }

    if (len >= 2) {
        // Process 2 characters directly -> 1 byte output
        // Batch validate both characters
        uint8_t d0 = dectbl[cur[0]];
        uint8_t d1 = dectbl[cur[1]];

        // Check if any character is invalid
        if ((d0 | d1) > 63) {
            errno = EINVAL;
            return SIZE_MAX;
        }

        // Check RFC 4648 compliance: last 4 bits must be 0 for 2-char groups
        if ((d1 & 0x0F) != 0) {
            errno = EILSEQ; // Illegal byte sequence - data loss would occur
            return SIZE_MAX;
        }

        // Direct decode: 2 chars (12 bits) -> 1 byte + 4 padding bits
        // [AAAAAA][BBBBBB] -> [AAAAAA|BB]
        uint32_t val = ((uint32_t)d0 << 6) | d1;
        ptr[0]       = (uint8_t)(val >> 4); // Single byte: [AAAAAA|BB]
        ptr += 1;
        cur += 2;
    }

    // If 1 character remains, ignore it (invalid incomplete group)

    // Null terminate for safety
    *ptr = 0;

    // Return decoded length
    return (size_t)((char *)ptr - dst);
}

/**
 * @name Convenience Macros for Buffer Decoding
 * @{
 */

/** @brief Decode to user buffer using standard Base64 format
 *  @param src Input base64 string
 *  @param srclen Input string length
 *  @param dst Output buffer
 *  @param dstlen Output buffer size
 *  @return Length of decoded data or SIZE_MAX on error */
#define b64m_decode_to_buffer_std(src, srclen, dst, dstlen)                    \
    b64m_decode_to_buffer(src, srclen, dst, dstlen, BASE64MIX_STDDEC)

/** @brief Decode to user buffer using URL-safe Base64 format
 *  @param src Input base64 string
 *  @param srclen Input string length
 *  @param dst Output buffer
 *  @param dstlen Output buffer size
 *  @return Length of decoded data or SIZE_MAX on error */
#define b64m_decode_to_buffer_url(src, srclen, dst, dstlen)                    \
    b64m_decode_to_buffer(src, srclen, dst, dstlen, BASE64MIX_URLDEC)

/** @brief Decode to user buffer using mixed Base64 format (handles both
 * standard and URL-safe)
 *  @param src Input base64 string
 *  @param srclen Input string length
 *  @param dst Output buffer
 *  @param dstlen Output buffer size
 *  @return Length of decoded data or SIZE_MAX on error */
#define b64m_decode_to_buffer_mix(src, srclen, dst, dstlen)                    \
    b64m_decode_to_buffer(src, srclen, dst, dstlen, BASE64MIX_DEC)

/** @} */

/**
 * @brief Decode base64 string to binary data
 *
 * @param src Input base64 string to decode (must not be NULL)
 * @param len Input/Output: input string length -> decoded data length
 * @param dectbl Decoding table (BASE64MIX_STDDEC, BASE64MIX_URLDEC, or
 * BASE64MIX_DEC)
 *
 * @return Allocated binary data (caller must free), or NULL on error
 *
 * @errno EINVAL - Invalid arguments (NULL pointers)
 * @errno EINVAL - Invalid base64 character encountered
 * @errno EINVAL - Invalid padding format (non-'=' after '=')
 * @errno ENOMEM - Memory allocation failure
 * @errno EILSEQ - Illegal byte sequence (non-zero ignored bits in incomplete
 * groups)
 *
 * @note Handles both standard (with padding) and URL-safe (without padding)
 * formats
 * @note Result buffer is null-terminated for safety (length excludes
 * terminator)
 * @note Incomplete groups (1 char) are silently ignored as invalid
 */
static inline char *b64m_decode(const char *src, size_t *len,
                                const unsigned char dectbl[])
{
    char *res     = NULL;
    size_t buflen = 0;

    // Reset errno before processing
    errno = 0;

    // Validate input parameters
    if (!src || !len || !dectbl) {
        errno = EINVAL;
        return NULL;
    }

    // Calculate required buffer size using zero-allocation helper
    buflen = b64m_decoded_len(*len) + 1; // +1 for null terminator
    if ((res = malloc(buflen))) {
        // Use zero-allocation version to do the actual decoding
        // Update length with actual decoded length
        size_t outlen = b64m_decode_to_buffer(src, *len, res, buflen, dectbl);
        if (outlen == SIZE_MAX) {
            // Error occurred in decoding
            free(res);
            return NULL;
        }
        *len = outlen;
    }
    return res;
}

/**
 * @name Decode Convenience Macros
 * @{
 */

/** @brief Decode standard Base64 format (expects padding)
 *  @param src Input base64 string
 *  @param len Input/Output length pointer
 *  @return Decoded binary data or NULL on error */
#define b64m_decode_std(src, len) b64m_decode(src, len, BASE64MIX_STDDEC)

/** @brief Decode URL-safe Base64 format (no padding expected)
 *  @param src Input base64 string
 *  @param len Input/Output length pointer
 *  @return Decoded binary data or NULL on error */
#define b64m_decode_url(src, len) b64m_decode(src, len, BASE64MIX_URLDEC)

/** @brief Decode mixed format (handles both standard and URL-safe)
 *  @param src Input base64 string
 *  @param len Input/Output length pointer
 *  @return Decoded binary data or NULL on error */
#define b64m_decode_mix(src, len) b64m_decode(src, len, BASE64MIX_DEC)

/** @} */

#endif /* BASE64MIX_H */
