#include "halley/resources/asset_pack.h"
#include "halley/resources/asset_database.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/compression.h"
#include "halley/maths/random.h"
#include "halley/utils/encrypt.h"

using namespace Halley;

void AssetPackHeaderV1::init(size_t assetDbSize)
{
	assetDbStartPos = sizeof(AssetPackHeaderV1) + 8;
	dataStartPos = assetDbStartPos + assetDbSize;
	memset(iv.data(), 0, iv.size());
}

void AssetPackHeaderV2::init(size_t assetDbSize)
{
	version = 2;
	assetDbStartPos = alignUp<size_t>(sizeof(AssetPackHeaderV2) + 8, 16);
	assetDbPadding = static_cast<uint8_t>(alignUp<size_t>(assetDbSize, 16) - assetDbSize);
	dataStartPos = assetDbStartPos + alignUp<size_t>(assetDbSize, 16);
	memset(iv.data(), 0, iv.size());
}

AssetPack::AssetPack()
	: assetDb(std::make_unique<AssetDatabase>())
{
	memset(iv.data(), 0, iv.size());
}

AssetPack::AssetPack(AssetPack&& other) noexcept
{
	*this = std::move(other);
}

AssetPack::AssetPack(std::unique_ptr<ResourceDataReader> _reader, std::optional<Encrypt::AESKey> encryptionKey, bool preLoad)
	: reader(std::move(_reader))
{
	size_t totalSize = reader->size();
	if (totalSize < 8) {
		throw Exception("Asset pack is invalid (too small)", HalleyExceptions::Resources);
	}

	// Read identifier
	std::array<char, 8> id;
	if (reader->read(gsl::as_writable_bytes(gsl::span(id))) != 8) {
		throw Exception("Unable to read pack header identifier", HalleyExceptions::Resources);
	}
	int packVersion = 0;
	if (memcmp(id.data(), "HALLEYPK", 8) == 0) {
		Logger::logWarning("Loading v1 asset pack");
		packVersion = 1;
	} else if (memcmp(id.data(), "HALLEYP2", 8) == 0) {
		packVersion = 2;
	} else {
		throw Exception("Asset pack is invalid (invalid identifier)", HalleyExceptions::Resources);
	}

	uint8_t assetDbPadding = 0;
	uint64_t assetDbStartPos = 0;
	uint64_t dataStartPos = 0;

	if (packVersion == 2) {
		AssetPackHeaderV2 header;
		if (reader->read(gsl::as_writable_bytes(gsl::span(&header, 1))) != sizeof(header)) {
			throw Exception("Unable to read header", HalleyExceptions::Resources);
		}
		if (header.version != 2) {
			throw Exception("Unknown asset pack version", HalleyExceptions::Resources);
		}
		iv = header.iv;
		dataOffset = header.dataStartPos;
		assetDbStartPos = header.assetDbStartPos;
		dataStartPos = header.dataStartPos;
		assetDbPadding = header.assetDbPadding;
	} else if (packVersion == 1) {
		AssetPackHeaderV1 header;
		if (reader->read(gsl::as_writable_bytes(gsl::span(&header, 1))) != sizeof(header)) {
			throw Exception("Unable to read header", HalleyExceptions::Resources);
		}
		iv = header.iv;
		dataOffset = header.dataStartPos;
		assetDbStartPos = header.assetDbStartPos;
		dataStartPos = header.dataStartPos;
	}

	// Read asset database
	{
		const size_t assetDbSize = size_t(dataStartPos - assetDbStartPos - assetDbPadding);
		auto assetDbBytes = Bytes(assetDbSize);
		if (reader->read(gsl::as_writable_bytes(gsl::span<Byte>(assetDbBytes))) != assetDbBytes.size()) {
			throw Exception("Unable to read asset database", HalleyExceptions::Resources);
		}
		assetDb = std::make_unique<AssetDatabase>();
		Deserializer::fromBytes<AssetDatabase>(*assetDb, Compression::decompress(assetDbBytes));
	}

	std::array<char, 16> ivEmpty;
	memset(ivEmpty.data(), 0, ivEmpty.size());
	const bool hasCrypt = memcmp(iv.data(), ivEmpty.data(), iv.size()) != 0 && encryptionKey.has_value();

	if (preLoad || hasCrypt) {
		readToMemory();
	}

	if (hasCrypt) {
		decrypt(*encryptionKey);
	}
}

AssetPack::~AssetPack()
{
	if (aliveToken) {
		*aliveToken = false;
	}
}

AssetPack& AssetPack::operator=(AssetPack&& other) noexcept
{
	assetDb = std::move(other.assetDb);
	dataOffset = other.dataOffset;
	reader = std::move(other.reader);
	data = std::move(other.data);

	other.reader.reset();

	return *this;
}

AssetDatabase& AssetPack::getAssetDatabase()
{
	return *assetDb;
}

const AssetDatabase& AssetPack::getAssetDatabase() const
{
	return *assetDb;
}

Bytes& AssetPack::getData()
{
	return data;
}

const Bytes& AssetPack::getData() const
{
	return data;
}

Bytes AssetPack::writeOut() const
{
	auto assetDbBytes = Compression::compress(Serializer::toBytes(*assetDb));
	AssetPackHeaderV2 header;
	header.init(assetDbBytes.size());
	header.iv = iv;

	auto result = Bytes(header.dataStartPos + data.size(), 0);
	memcpy(result.data(), decltype(header)::id, 8);
	memcpy(result.data() + 8, &header, sizeof(header));
	memcpy(result.data() + header.assetDbStartPos, assetDbBytes.data(), assetDbBytes.size());
	memcpy(result.data() + header.dataStartPos, data.data(), data.size());
	return result;
}

std::unique_ptr<ResourceData> AssetPack::getData(const String& asset, AssetType type, bool stream)
{
	const auto* assetInfo = assetDb->getDatabase(type).tryGet(asset);
	if (!assetInfo) {
		return {};
	}

	auto buffer = std::array<std::string_view, 2>();
	const auto split = String::splitToBuffer(assetInfo->path, ':', buffer);
	const size_t pos = String::toInteger64(split[0]);
	const size_t size = String::toInteger64(split[1]);

	if (stream) {
		return std::make_unique<ResourceDataStream>(asset, [=, this] () -> std::unique_ptr<ResourceDataReader> {
			return std::make_unique<PackDataReader>(*this, pos, size);
		});
	} else {
		if (reader) {
			auto result = new char[size];
			try {
				readData(pos, gsl::as_writable_bytes(gsl::span<char>(result, size)));
				return std::make_unique<ResourceDataStatic>(result, size, asset, true);
			} catch (...) {
				delete[] result;
				throw;
			}
		} else {
			// Preloaded
			if (pos + size > data.size()) {
				throw Exception("Asset \"" + asset + "\" is out of pack bounds.", HalleyExceptions::Resources);
			}

			return std::make_unique<ResourceDataStatic>(data.data() + pos, size, asset, false);
		}
	}
}

void AssetPack::readToMemory()
{
	data = reader->readAll(dataOffset);
	reader = {};
}

void AssetPack::encrypt(Encrypt::AESKey key)
{
	// Generate IV
	Random::getGlobal().getBytes(gsl::as_writable_bytes(gsl::span<uint8_t>(iv)));

	data = Encrypt::encryptAES(iv, key, data);
}

void AssetPack::decrypt(Encrypt::AESKey key)
{
	data = Encrypt::decryptAES(iv, key, data);
}

void AssetPack::readData(size_t pos, gsl::span<std::byte> dst)
{
	if (reader) {
		reader->readAt(dst, pos + dataOffset);
		return;
	}

	// Didn't read with reader, read from data
	if (pos + size_t(dst.size()) > data.size()) {
		throw Exception("Asset data is out of pack bounds.", HalleyExceptions::Resources);
	}
	memcpy(dst.data(), data.data() + pos, dst.size());
}

std::unique_ptr<ResourceDataReader> AssetPack::extractReader()
{
	return std::move(reader);
}

std::shared_ptr<bool> AssetPack::getAliveToken() const
{
	if (!aliveToken) {
		aliveToken = std::make_shared<bool>(true);
	}
	return aliveToken;
}

size_t AssetPack::getMemoryUsage() const
{
	return sizeof(*this) + data.size_bytes() + assetDb->getMemoryUsage();
}

PackDataReader::PackDataReader(AssetPack& pack, size_t startPos, size_t fileSize)
	: pack(pack)
	, startPos(startPos)
	, fileSize(fileSize)
	, curPos(0)
	, aliveToken(pack.getAliveToken())
{
}

size_t PackDataReader::size() const
{
	return fileSize;
}

int PackDataReader::read(gsl::span<std::byte> dst)
{
	if (!*aliveToken) {
		return 0;
	}

	const size_t pos = curPos;
	size_t available = pos < fileSize ? fileSize - pos : 0;
	size_t toRead = std::min(available, size_t(dst.size()));

	pack.readData(startPos + pos, dst.subspan(0, toRead));
	curPos += toRead;

	return int(toRead);
}

int PackDataReader::readAt(gsl::span<std::byte> dst, size_t pos)
{
	if (!*aliveToken) {
		return 0;
	}

	size_t available = pos < fileSize ? fileSize - pos : 0;
	size_t toRead = std::min(available, size_t(dst.size()));

	pack.readData(startPos + pos, dst.subspan(0, toRead));
	curPos = pos + toRead;

	return int(toRead);
}

void PackDataReader::seek(int64_t pos, int whence)
{
	if (!*aliveToken) {
		return;
	}

	switch (whence) {
	case SEEK_SET:
		curPos = std::max<size_t>(pos, 0);
		break;
	case SEEK_CUR:
		curPos = static_cast<size_t>(static_cast<int64_t>(curPos) + pos);
		break;
	case SEEK_END:
		curPos = static_cast<size_t>(fileSize + pos);
		break;
	}

	if (curPos > fileSize) {
		curPos = fileSize;
	}
}

size_t PackDataReader::tell() const
{
	if (!*aliveToken) {
		return 0;
	}

	return curPos;
}

void PackDataReader::close()
{
}

bool PackDataReader::isAvailable() const
{
	return *aliveToken;
}

