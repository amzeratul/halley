#pragma once

#include <halley/data_structures/vector.h>
#include <gsl/span>

namespace Halley {
    class Cryptography {
    public:
        enum class HashAlgorithm {
	        SHA256
        };

        static size_t getDigestSize(HashAlgorithm algorithm);
        static Bytes hash(HashAlgorithm algorithm, gsl::span<const std::byte> bytes);
        static void hash(HashAlgorithm algorithm, gsl::span<const std::byte> bytes, gsl::span<std::byte> digest);
        static bool verifySignature(HashAlgorithm algorithm, gsl::span<const std::byte> publicKey, gsl::span<const std::byte> signature, gsl::span<const std::byte> bytes);
    };
}
