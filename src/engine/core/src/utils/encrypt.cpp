#include <cstring>
#include "halley/utils/encrypt.h"
#include "../contrib/tiny-aes/aes.hpp"
#include "halley/text/halleystring.h"
#include "halley/support/exception.h"
#include "halley/support/logger.h"
#include "halley/text/encode.h"

using namespace Halley;

Bytes Encrypt::encryptAES(AESIV iv, AESKey key, Bytes data)
{
	// Prepare buffer
	Bytes result = std::move(data);
	const size_t origSize = result.size();
	size_t newSize = alignUp(origSize, size_t(AES_BLOCKLEN));
	if (newSize == origSize) {
		// Must always add some padding, otherwise PKCS7 can't be undone
		newSize += AES_BLOCKLEN;
	}

	// Pad with PKCS7
	const unsigned char pad = static_cast<unsigned char>(newSize - origSize);
	result.resize(newSize);
	for (size_t i = origSize; i < newSize; ++i) {
		result[i] = pad;
	}

	// Encrypt
	AES_ctx ctx;
	AES_init_ctx_iv(&ctx, key.data(), iv.data());
	AES_CBC_encrypt_buffer(&ctx, result.data(), uint32_t(result.size()));

	return result;
}

Bytes Encrypt::decryptAES(AESIV iv, AESKey key, Bytes data)
{
	// Prepare buffer
	Bytes result = std::move(data);

	// If result is not mod AES_BLOCKLEN, things have already gone fucked, but we'll do our best...
	if (result.size() % AES_BLOCKLEN != 0) {
		Logger::logError("Encrypted block does not have the correct length.");
		result.resize(alignUp(result.size(), size_t(AES_BLOCKLEN)));
	}

	// Decrypt
	AES_ctx ctx;
	std::memset(&ctx, 0, sizeof(ctx));
	AES_init_ctx_iv(&ctx, key.data(), iv.data());
	AES_CBC_decrypt_buffer(&ctx, result.data(), uint32_t(result.size()));

	// Remove padding
	unsigned char padSize = result.back();
	if (padSize >= result.size()) {
		throw Exception("Unable to undo padding", HalleyExceptions::Utils);
	}
	result.resize(result.size() - padSize);

	return result;
}

void Encrypt::generateIV(AESIVMut iv, AESKey key, uint64_t v0, uint64_t v1)
{
	static_assert(sizeof(v0) + sizeof(v1) == 16);

	Bytes buffer;
	buffer.resize(16);

	memcpy(buffer.data(), &v0, sizeof(v0));
	memcpy(buffer.data() + sizeof(v0), &v1, sizeof(v1));

	const std::array<uint8_t, 16> zeroIV = {};
	const auto encBlock = encryptAES(zeroIV, key, buffer);
	memcpy(iv.data(), encBlock.data(), iv.size());
}
