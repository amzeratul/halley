#include "halley/utils/cryptography.h"
#include "halley/support/exception.h"

using namespace Halley;

size_t Cryptography::getDigestSize(HashAlgorithm algorithm)
{
	switch (algorithm) {
	case HashAlgorithm::SHA256:
		return 256 / 8;
	}
	return 0;
}

Bytes Cryptography::hash(HashAlgorithm algorithm, gsl::span<const gsl::byte> bytes)
{
	Bytes result;
	result.resize(getDigestSize(algorithm));
	hash(algorithm, bytes, result.byte_span());
	return result;
}

#ifdef WITH_SSL

#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>

void Cryptography::hash(HashAlgorithm algorithm, gsl::span<const gsl::byte> bytes, gsl::span<gsl::byte> digest)
{
	if (algorithm == HashAlgorithm::SHA256) {
		SHA256(reinterpret_cast<const uint8_t*>(bytes.data()), bytes.size(), reinterpret_cast<uint8_t*>(digest.data()));
	}
}

bool Cryptography::verifySignature(HashAlgorithm algorithm, gsl::span<const gsl::byte> publicKey, gsl::span<const gsl::byte> signature, gsl::span<const gsl::byte> bytes)
{
    auto bio = std::unique_ptr<BIO, decltype(&BIO_free)>(BIO_new_mem_buf(publicKey.data(), -1), &BIO_free);
    if (!bio) {
        return false;
    }
    const auto pubKey = std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(PEM_read_bio_PUBKEY(bio.get(), nullptr, nullptr, nullptr), &EVP_PKEY_free);
    if (!pubKey) {
        return false;
    }
	bio = {};

	const EVP_MD* type = nullptr;
	switch (algorithm) {
	case HashAlgorithm::SHA256:
		type = EVP_sha256();
	}
	if (!type) {
		return false;
	}

	const auto verifyCtx = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>(EVP_MD_CTX_create(), &EVP_MD_CTX_free);
	if (EVP_DigestVerifyInit(verifyCtx.get(), nullptr, type, nullptr, pubKey.get()) <= 0) {
		return false;
	}
	if (EVP_DigestVerifyUpdate(verifyCtx.get(), bytes.data(), bytes.size()) <= 0) {
		return false;
	}
	return EVP_DigestVerifyFinal(verifyCtx.get(), reinterpret_cast<const unsigned char*>(signature.data()), signature.size()) == 1;
}

#else 

void Cryptography::hash(HashAlgorithm algorithm, gsl::span<const gsl::byte> bytes, gsl::span<gsl::byte> digest)
{
	throw Exception("SSL not supported in this build, cryptography functions are unavailable.", HalleyExceptions::Utils);
}

bool Cryptography::verifySignature(HashAlgorithm algorithm, gsl::span<const gsl::byte> publicKey, gsl::span<const gsl::byte> signature, gsl::span<const gsl::byte> bytes)
{
	throw Exception("SSL not supported in this build, cryptography functions are unavailable.", HalleyExceptions::Utils);
}

#endif
