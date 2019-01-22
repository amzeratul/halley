#include "halley/utils/encrypt.h"
#include "../contrib/tiny-aes/aes.hpp"
#include "halley/text/halleystring.h"
#include "halley/support/exception.h"
#include "halley/support/logger.h"
#include "halley/text/encode.h"

using namespace Halley;

Bytes Encrypt::encrypt(const Bytes& iv, const String& key, const Bytes& data)
{
	Expects(iv.size() == 16);

	// Prepare buffer
	Bytes result = data;
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
	AES_init_ctx_iv(&ctx, reinterpret_cast<const uint8_t*>(key.c_str()), iv.data());
	AES_CBC_encrypt_buffer(&ctx, result.data(), uint32_t(result.size()));

	return result;
}

Bytes Encrypt::decrypt(const Bytes& iv, const String& key, const Bytes& data)
{
	Expects(iv.size() == 16);

	// Prepare buffer
	Bytes result = data;

	// Decrypt
	AES_ctx ctx;
	memset(&ctx, 0, sizeof(ctx));
	AES_init_ctx_iv(&ctx, reinterpret_cast<const uint8_t*>(key.c_str()), iv.data());
	AES_CBC_decrypt_buffer(&ctx, result.data(), uint32_t(result.size()));

	Logger::logInfo(Encode::encodeBase16(result));

	// Remove padding
	unsigned char padSize = result.back();
	if (padSize >= result.size()) {
		throw Exception("Unable to undo padding", HalleyExceptions::Utils);
	}
	result.resize(result.size() - padSize);

	return result;
}
