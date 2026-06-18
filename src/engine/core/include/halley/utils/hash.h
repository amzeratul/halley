#pragma once

#include <cstdint>
#include <bit>
#include <gsl/gsl>
#include "utils.h"
#include "halley/text/halleystring.h"
#include "../contrib/xxhash/constexpr-xxh3.h"

namespace Halley {
    namespace Hash {
		namespace Detail {
			uint64_t hash1Byte(const void* bytes);
			uint64_t hash2Bytes(const void* bytes);
			uint64_t hash4Bytes(const void* bytes);
			uint64_t hash8Bytes(const void* bytes);
			uint64_t hash16Bytes(const void* bytes);
			uint64_t hash32Bytes(const void* bytes);
			uint64_t hash64Bytes(const void* bytes);
			uint64_t hashNBytes(const void* bytes, size_t len);
		}

        constexpr uint64_t hash(const char* bytes, size_t len)
		{
			if (std::is_constant_evaluated()) {
				return constexpr_xxh3::XXH3_64bits_const(bytes, len);
			} else {
				return Detail::hashNBytes(bytes, len);
			}
		}

        constexpr uint64_t hash(const std::byte* bytes, size_t len)
		{
			if (std::is_constant_evaluated()) {
				return constexpr_xxh3::XXH3_64bits_const(bytes, len);
			} else {
				return Detail::hashNBytes(bytes, len);
			}
		}

    	constexpr uint64_t hash(const Bytes& bytes)
		{
			return hash(std::bit_cast<const std::byte*>(bytes.data()), bytes.size_bytes());
		}

        constexpr uint64_t hash(gsl::span<const std::byte> bytes)
		{
			return hash(bytes.data(), bytes.size());
		}

        constexpr uint64_t hash(const std::string_view& str)
		{
			return hash(str.data(), str.size());
		}

    	template <typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
    	constexpr uint64_t hash(const T& v)
    	{
			if (std::is_constant_evaluated()) {
				const auto bytes = std::bit_cast<std::array<char, sizeof(T)>>(v);
				return constexpr_xxh3::XXH3_64bits_const(bytes.data(), sizeof(T));
			} else if constexpr (sizeof(T) == 1) {
				return Detail::hash1Byte(&v);
			} else if constexpr (sizeof(T) == 2) {
				return Detail::hash2Bytes(&v);
			} else if constexpr (sizeof(T) == 4) {
				return Detail::hash4Bytes(&v);
			} else if constexpr (sizeof(T) == 8) {
				return Detail::hash8Bytes(&v);
			} else if constexpr (sizeof(T) == 16) {
				return Detail::hash16Bytes(&v);
			} else if constexpr (sizeof(T) == 32) {
				return Detail::hash32Bytes(&v);
			} else if constexpr (sizeof(T) == 64) {
				return Detail::hash64Bytes(&v);
			} else {
				return hash(gsl::as_bytes(gsl::span<const T>(&v, 1)));
			}
    	}

    	template <typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
    	constexpr uint64_t hash(const T* v, size_t n)
    	{
			return hash(std::bit_cast<const std::byte*>(v), n * sizeof(T));
    	}

    	template <typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
    	constexpr uint64_t hash(gsl::span<const T> vs)
    	{
			return hash(gsl::as_bytes(vs));
    	}
		
		constexpr uint32_t combineHash32(uint32_t a, uint32_t b)
		{
			// From https://stackoverflow.com/a/27952689
			return a ^ (b + 0x9e3779b9u + (a << 6) + (a >> 2));
		}

		constexpr uint64_t combineHash64(uint64_t a, uint64_t b)
		{
			// From https://stackoverflow.com/a/27952689
			return a ^ (b + 0x517cc1b727220a95ull + (a << 6) + (a >> 2));
		}

		constexpr uint32_t compressTo32(uint64_t value)
		{
			const auto high = value >> 32;
			const auto low = value & 0xFFFFFFFFull;
			return combineHash32(static_cast<uint32_t>(high), static_cast<uint32_t>(low));
		}

		constexpr size_t combineHash(size_t a, size_t b)
		{
			if constexpr (sizeof(size_t) == 8) {
				return static_cast<size_t>(combineHash64(static_cast<uint64_t>(a), static_cast<uint64_t>(b)));
			} else {
				return static_cast<size_t>(combineHash32(static_cast<uint32_t>(a), static_cast<uint32_t>(b)));
			}
		}

		class alignas(64) Hasher
		{
		public:
			Hasher();
			~Hasher();

			void feed(const String& string)
			{
				feedBytes(gsl::as_bytes(gsl::span<const char>(string.c_str(), string.length())));
			}

			void feed(std::string_view string)
			{
				feedBytes(gsl::as_bytes(gsl::span<const char>(string.data(), string.size())));
			}

			template<typename T, std::enable_if_t<std::is_trivially_copyable_v<T>, int> = 0>
			void feed(const T& data)
			{
				if constexpr (sizeof(T) == 1) {
					feed1Byte(&data);
				} else if constexpr (sizeof(T) == 2) {
					feed2Bytes(&data);
				} else if constexpr (sizeof(T) == 4) {
					feed4Bytes(&data);
				} else if constexpr (sizeof(T) == 8) {
					feed8Bytes(&data);
				} else if constexpr (sizeof(T) == 16) {
					feed16Bytes(&data);
				} else if constexpr (sizeof(T) == 32) {
					feed32Bytes(&data);
				} else if constexpr (sizeof(T) == 64) {
					feed64Bytes(&data);
				} else {
					feedBytes(gsl::as_bytes(gsl::span<const T>(&data, 1)));
				}
			}

			void feedBytes(gsl::span<const std::byte> bytes);

			[[nodiscard]] uint64_t digest();
			void reset();

		private:
			uint64_t state[576 / 8]; // must match size of XXH3_state_s

			void feed1Byte(const void* bytes);
			void feed2Bytes(const void* bytes);
			void feed4Bytes(const void* bytes);
			void feed8Bytes(const void* bytes);
			void feed16Bytes(const void* bytes);
			void feed32Bytes(const void* bytes);
			void feed64Bytes(const void* bytes);
		};
    };
}
