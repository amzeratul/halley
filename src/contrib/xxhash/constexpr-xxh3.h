/*
BSD 2-Clause License

constexpr-xxh3 - C++20 constexpr implementation of the XXH3 64-bit variant of xxHash
Copyright (c) 2021-2023, chys <admin@chys.info> <chys87@github>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
This file uses code from Yann Collet's xxHash implementation.
Original xxHash copyright notice:

xxHash - Extremely Fast Hash algorithm
Header File
Copyright (C) 2012-2020 Yann Collet
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>  // for std::data, std::size
#include <type_traits>
#include <utility>

namespace constexpr_xxh3 {

template <typename T>
concept ByteType = (std::is_integral_v<T> && sizeof(T) == 1)
#if defined __cpp_lib_byte && __cpp_lib_byte >= 201603
                   || std::is_same_v<T, std::byte>
#endif
    ;

template <typename T>
concept BytePtrType = requires (T ptr) {
  requires std::is_pointer_v<T>;
  requires ByteType<std::remove_cvref_t<decltype(*ptr)>>;
};

template <typename T>
concept BytesType = requires (const T& bytes) {
  { std::data(bytes) };
  requires BytePtrType<decltype(std::data(bytes))>;
  // -> std::convertible_to is not supported widely enough
  { static_cast<size_t>(std::size(bytes)) };
};

inline constexpr uint32_t swap32(uint32_t x) noexcept {
  return ((x << 24) & 0xff000000) | ((x << 8) & 0x00ff0000) |
         ((x >> 8) & 0x0000ff00) | ((x >> 24) & 0x000000ff);
}

template <typename T>
inline constexpr uint32_t readLE32(const T* ptr) noexcept {
  return uint8_t(ptr[0]) | uint32_t(uint8_t(ptr[1])) << 8 |
         uint32_t(uint8_t(ptr[2])) << 16 | uint32_t(uint8_t(ptr[3])) << 24;
}

inline constexpr uint64_t swap64(uint64_t x) noexcept {
  return ((x << 56) & 0xff00000000000000ULL) |
         ((x << 40) & 0x00ff000000000000ULL) |
         ((x << 24) & 0x0000ff0000000000ULL) |
         ((x << 8) & 0x000000ff00000000ULL) |
         ((x >> 8) & 0x00000000ff000000ULL) |
         ((x >> 24) & 0x0000000000ff0000ULL) |
         ((x >> 40) & 0x000000000000ff00ULL) |
         ((x >> 56) & 0x00000000000000ffULL);
}

template <typename T>
inline constexpr uint64_t readLE64(const T* ptr) noexcept {
  return readLE32(ptr) | uint64_t(readLE32(ptr + 4)) << 32;
}

inline constexpr void writeLE64(uint8_t* dst, uint64_t v) noexcept {
  for (int i = 0; i < 8; ++i) dst[i] = uint8_t(v >> (i * 8));
}

inline constexpr uint32_t PRIME32_1 = 0x9E3779B1U;
inline constexpr uint32_t PRIME32_2 = 0x85EBCA77U;
inline constexpr uint32_t PRIME32_3 = 0xC2B2AE3DU;

inline constexpr uint64_t PRIME64_1 = 0x9E3779B185EBCA87ULL;
inline constexpr uint64_t PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;
inline constexpr uint64_t PRIME64_3 = 0x165667B19E3779F9ULL;
inline constexpr uint64_t PRIME64_4 = 0x85EBCA77C2B2AE63ULL;
inline constexpr uint64_t PRIME64_5 = 0x27D4EB2F165667C5ULL;

inline constexpr size_t SECRET_DEFAULT_SIZE = 192;
inline constexpr size_t SECRET_SIZE_MIN = 136;

inline constexpr uint8_t kSecret[SECRET_DEFAULT_SIZE]{
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c,
    0xf7, 0x21, 0xad, 0x1c, 0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb,
    0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f, 0xcb, 0x79, 0xe6, 0x4e,
    0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6,
    0x81, 0x3a, 0x26, 0x4c, 0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb,
    0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3, 0x71, 0x64, 0x48, 0x97,
    0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7,
    0xc7, 0x0b, 0x4f, 0x1d, 0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31,
    0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64, 0xea, 0xc5, 0xac, 0x83,
    0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26,
    0x29, 0xd4, 0x68, 0x9e, 0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc,
    0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce, 0x45, 0xcb, 0x3a, 0x8f,
    0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};

inline constexpr std::pair<uint64_t, uint64_t> mult64to128(
    uint64_t lhs, uint64_t rhs) noexcept {
  uint64_t lo_lo = uint64_t(uint32_t(lhs)) * uint32_t(rhs);
  uint64_t hi_lo = (lhs >> 32) * uint32_t(rhs);
  uint64_t lo_hi = uint32_t(lhs) * (rhs >> 32);
  uint64_t hi_hi = (lhs >> 32) * (rhs >> 32);
  uint64_t cross = (lo_lo >> 32) + uint32_t(hi_lo) + lo_hi;
  uint64_t upper = (hi_lo >> 32) + (cross >> 32) + hi_hi;
  uint64_t lower = (cross << 32) | uint32_t(lo_lo);
  return {lower, upper};
}

inline constexpr uint64_t mul128_fold64(uint64_t lhs, uint64_t rhs) noexcept {
#if defined __GNUC__ && __WORDSIZE >= 64
  // It appears both GCC and Clang support evaluating __int128 as constexpr
  auto product = static_cast<unsigned __int128>(lhs) * rhs;
  return uint64_t(product >> 64) ^ uint64_t(product);
#else
  auto product = mult64to128(lhs, rhs);
  return product.first ^ product.second;
#endif
}

inline constexpr uint64_t XXH64_avalanche(uint64_t h) noexcept {
  h = (h ^ (h >> 33)) * PRIME64_2;
  h = (h ^ (h >> 29)) * PRIME64_3;
  return h ^ (h >> 32);
}

inline constexpr uint64_t XXH3_avalanche(uint64_t h) noexcept {
  h = (h ^ (h >> 37)) * 0x165667919E3779F9ULL;
  return h ^ (h >> 32);
}

inline constexpr uint64_t rrmxmx(uint64_t h, uint64_t len) noexcept {
  h ^= ((h << 49) | (h >> 15)) ^ ((h << 24) | (h >> 40));
  h *= 0x9FB21C651E98DF25ULL;
  h ^= (h >> 35) + len;
  h *= 0x9FB21C651E98DF25ULL;
  return h ^ (h >> 28);
}

template <typename T, typename S>
constexpr uint64_t mix16B(const T* input, const S* secret,
                          uint64_t seed) noexcept {
  return mul128_fold64(readLE64(input) ^ (readLE64(secret) + seed),
                       readLE64(input + 8) ^ (readLE64(secret + 8) - seed));
}

inline constexpr size_t STRIPE_LEN = 64;
inline constexpr size_t SECRET_CONSUME_RATE = 8;
inline constexpr size_t ACC_NB = STRIPE_LEN / sizeof(uint64_t);

template <typename T, typename S>
constexpr void accumulate_512(uint64_t* acc, const T* input,
                              const S* secret) noexcept {
  for (size_t i = 0; i < ACC_NB; i++) {
    uint64_t data_val = readLE64(input + 8 * i);
    uint64_t data_key = data_val ^ readLE64(secret + i * 8);
    acc[i ^ 1] += data_val;
    acc[i] += uint32_t(data_key) * (data_key >> 32);
  }
}

template <typename T, typename S>
constexpr uint64_t hashLong_64b_internal(const T* input, size_t len,
                                         const S* secret,
                                         size_t secretSize) noexcept {
  uint64_t acc[ACC_NB]{PRIME32_3, PRIME64_1, PRIME64_2, PRIME64_3,
                       PRIME64_4, PRIME32_2, PRIME64_5, PRIME32_1};
  size_t nbStripesPerBlock = (secretSize - STRIPE_LEN) / SECRET_CONSUME_RATE;
  size_t block_len = STRIPE_LEN * nbStripesPerBlock;
  size_t nb_blocks = (len - 1) / block_len;

  for (size_t n = 0; n < nb_blocks; n++) {
    for (size_t i = 0; i < nbStripesPerBlock; i++)
      accumulate_512(acc, input + n * block_len + i * STRIPE_LEN,
                     secret + i * SECRET_CONSUME_RATE);
    for (size_t i = 0; i < ACC_NB; i++)
      acc[i] = (acc[i] ^ (acc[i] >> 47) ^
                readLE64(secret + secretSize - STRIPE_LEN + 8 * i)) *
               PRIME32_1;
  }

  size_t nbStripes = ((len - 1) - (block_len * nb_blocks)) / STRIPE_LEN;
  for (size_t i = 0; i < nbStripes; i++)
    accumulate_512(acc, input + nb_blocks * block_len + i * STRIPE_LEN,
                   secret + i * SECRET_CONSUME_RATE);
  accumulate_512(acc, input + len - STRIPE_LEN,
                 secret + secretSize - STRIPE_LEN - 7);
  uint64_t result = len * PRIME64_1;
  for (size_t i = 0; i < 4; i++)
    result +=
        mul128_fold64(acc[2 * i] ^ readLE64(secret + 11 + 16 * i),
                      acc[2 * i + 1] ^ readLE64(secret + 11 + 16 * i + 8));
  return XXH3_avalanche(result);
}

template <typename T, typename S, typename HashLong>
constexpr uint64_t XXH3_64bits_internal(const T* input, size_t len,
                                        uint64_t seed, const S* secret,
                                        size_t secretLen,
                                        HashLong f_hashLong) noexcept {
  if (len == 0) {
    return XXH64_avalanche(seed ^
                           (readLE64(secret + 56) ^ readLE64(secret + 64)));
  } else if (len < 4) {
    uint64_t keyed = ((uint32_t(uint8_t(input[0])) << 16) |
                      (uint32_t(uint8_t(input[len >> 1])) << 24) |
                      uint8_t(input[len - 1]) | (uint32_t(len) << 8)) ^
                     ((readLE32(secret) ^ readLE32(secret + 4)) + seed);
    return XXH64_avalanche(keyed);
  } else if (len <= 8) {
    uint64_t keyed =
        (readLE32(input + len - 4) + (uint64_t(readLE32(input)) << 32)) ^
        ((readLE64(secret + 8) ^ readLE64(secret + 16)) -
         (seed ^ (uint64_t(swap32(uint32_t(seed))) << 32)));
    return rrmxmx(keyed, len);
  } else if (len <= 16) {
    uint64_t input_lo =
        readLE64(input) ^
        ((readLE64(secret + 24) ^ readLE64(secret + 32)) + seed);
    uint64_t input_hi =
        readLE64(input + len - 8) ^
        ((readLE64(secret + 40) ^ readLE64(secret + 48)) - seed);
    uint64_t acc =
        len + swap64(input_lo) + input_hi + mul128_fold64(input_lo, input_hi);
    return XXH3_avalanche(acc);
  } else if (len <= 128) {
    uint64_t acc = len * PRIME64_1;
    size_t secret_off = 0;
    for (size_t i = 0, j = len; j > i; i += 16, j -= 16) {
      acc += mix16B(input + i, secret + secret_off, seed);
      acc += mix16B(input + j - 16, secret + secret_off + 16, seed);
      secret_off += 32;
    }
    return XXH3_avalanche(acc);
  } else if (len <= 240) {
    uint64_t acc = len * PRIME64_1;
    for (size_t i = 0; i < 128; i += 16)
      acc += mix16B(input + i, secret + i, seed);
    acc = XXH3_avalanche(acc);
    for (size_t i = 128; i < len / 16 * 16; i += 16)
      acc += mix16B(input + i, secret + (i - 128) + 3, seed);
    acc += mix16B(input + len - 16, secret + SECRET_SIZE_MIN - 17, seed);
    return XXH3_avalanche(acc);
  } else {
    return f_hashLong(input, len, seed, secret, secretLen);
  }
}

template <BytesType Bytes>
constexpr size_t bytes_size(const Bytes& bytes) noexcept {
  return std::size(bytes);
}

template <ByteType T, size_t N>
constexpr size_t bytes_size(T (&)[N]) noexcept {
  return (N ? N - 1 : 0);
}

/// Basic interfaces

template <ByteType T>
constexpr uint64_t XXH3_64bits_const(const T* input, size_t len) noexcept {
  return XXH3_64bits_internal(
      input, len, 0, kSecret, sizeof(kSecret),
      [](const T* input, size_t len, uint64_t, const void*,
         size_t) constexpr noexcept {
        return hashLong_64b_internal(input, len, kSecret, sizeof(kSecret));
      });
}

template <ByteType T, ByteType S>
constexpr uint64_t XXH3_64bits_withSecret_const(const T* input, size_t len,
                                                const S* secret,
                                                size_t secretSize) noexcept {
  return XXH3_64bits_internal(
      input, len, 0, secret, secretSize,
      [](const T* input, size_t len, uint64_t, const S* secret,
         size_t secretLen) constexpr noexcept {
        return hashLong_64b_internal(input, len, secret, secretLen);
      });
}

template <ByteType T>
constexpr uint64_t XXH3_64bits_withSeed_const(const T* input, size_t len,
                                              uint64_t seed) noexcept {
  if (seed == 0) return XXH3_64bits_const(input, len);
  return XXH3_64bits_internal(
      input, len, seed, kSecret, sizeof(kSecret),
      [](const T* input, size_t len, uint64_t seed, const void*,
         size_t) constexpr noexcept {
        uint8_t secret[SECRET_DEFAULT_SIZE];
        for (size_t i = 0; i < SECRET_DEFAULT_SIZE; i += 16) {
          writeLE64(secret + i, readLE64(kSecret + i) + seed);
          writeLE64(secret + i + 8, readLE64(kSecret + i + 8) - seed);
        }
        return hashLong_64b_internal(input, len, secret, sizeof(secret));
      });
}

/// Convenient interfaces

template <BytesType Bytes>
constexpr uint64_t XXH3_64bits_const(const Bytes& input) noexcept {
  return XXH3_64bits_const(std::data(input), bytes_size(input));
}

template <BytesType Bytes, BytesType Secret>
constexpr uint64_t XXH3_64bits_withSecret_const(const Bytes& input,
                                                const Secret& secret) noexcept {
  return XXH3_64bits_withSecret_const(std::data(input), bytes_size(input),
                                      std::data(secret), bytes_size(secret));
}

template <BytesType Bytes>
constexpr uint64_t XXH3_64bits_withSeed_const(const Bytes& input,
                                              uint64_t seed) noexcept {
  return XXH3_64bits_withSeed_const(std::data(input), bytes_size(input), seed);
}

}  // namespace constexpr_xxh3
