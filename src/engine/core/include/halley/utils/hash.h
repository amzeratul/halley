#pragma once

#include <cstdint>
#include <gsl/gsl>
#include "utils.h"
#include "halley/text/halleystring.h"

namespace Halley {
    namespace Hash {
        uint64_t hash(const Bytes& bytes);
        uint64_t hash(gsl::span<const std::byte> bytes);
		
    	template <typename T>
    	uint64_t hashValue(const T& v)
    	{
    		return hash(gsl::as_bytes(gsl::span<const T>(&v, 1)));
    	}

		uint32_t compressTo32(uint64_t value);


		class Hasher
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
			void* state;

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
