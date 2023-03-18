#pragma once

namespace Halley {
    class Cryptography {
    public:
        enum class HashAlgorithm {
	        SHA256
        };

        static size_t getDigestSize(HashAlgorithm algorithm);
        static Bytes hash(HashAlgorithm algorithm, gsl::span<const gsl::byte> bytes);
        static void hash(HashAlgorithm algorithm, gsl::span<const gsl::byte> bytes, gsl::span<gsl::byte> digest);
        static bool verifySignature(HashAlgorithm algorithm, gsl::span<const gsl::byte> publicKey, gsl::span<const gsl::byte> signature, gsl::span<const gsl::byte> bytes);
    };
}
