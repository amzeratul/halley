#pragma once

#include "utils.h"

namespace Halley {
	class String;

	class Encrypt {
	public:
		using AESKey = gsl::span<const uint8_t, 16>;
		using AESIV = gsl::span<const uint8_t, 16>;
		using AESIVMut = gsl::span<uint8_t, 16>;

		static Bytes encryptAES(AESIV iv, AESKey key, Bytes data);
		static Bytes decryptAES(AESIV iv, AESKey key, Bytes data);
		static void generateIV(AESIVMut iv, AESKey key, uint64_t v0, uint64_t v1 = 0);
	};
}
