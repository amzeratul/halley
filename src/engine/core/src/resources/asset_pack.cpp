#include "resources/asset_pack.h"
#include "resources/asset_database.h"
#include "halley/resources/resource_data.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/bytes/compression.h"
#include "halley/maths/random.h"
#include "halley/utils/encrypt.h"

using namespace Halley;

void AssetPackHeader::init(size_t assetDbSize)
{
	memcpy(identifier.data(), "HALLEYPK", 8);
	assetDbStartPos = sizeof(AssetPackHeader);
	dataStartPos = assetDbStartPos + assetDbSize;
	memset(iv.data(), 0, iv.size());
}

AssetPack::AssetPack()
	: assetDb(std::make_unique<AssetDatabase>())
	, hasReader(false)
{
	memset(iv.data(), 0, iv.size());
}

AssetPack::AssetPack(AssetPack&& other)
{
	*this = std::move(other);
}

AssetPack::AssetPack(std::unique_ptr<ResourceDataReader> _reader, const String& encryptionKey, bool preLoad)
	: reader(std::move(_reader))
	, hasReader(true)
{
	// Read header
	size_t totalSize = reader->size();
	if (totalSize < sizeof(AssetPackHeader)) {
		throw Exception("Asset pack is invalid (too small)", HalleyExceptions::Resources);
	}
	AssetPackHeader header;
	int nRead = reader->read(gsl::as_writable_bytes(gsl::span<AssetPackHeader>(&header, 1)));
	if (nRead != int(sizeof(header))) {
		throw Exception("Unable to read header", HalleyExceptions::Resources);
	}
	if (memcmp(header.identifier.data(), "HALLEYPK", 8) != 0) {
		throw Exception("Asset pack is invalid (invalid identifier)", HalleyExceptions::Resources);
	}
	iv = header.iv;
	dataOffset = size_t(header.dataStartPos);

	// Read asset database
	{
		const size_t assetDbSize = size_t(header.dataStartPos - header.assetDbStartPos);
		auto assetDbBytes = Bytes(assetDbSize);
		nRead = reader->read(gsl::as_writable_bytes(gsl::span<Byte>(assetDbBytes)));
		if (nRead != int(assetDbBytes.size())) {
			throw Exception("Unable to read header", HalleyExceptions::Resources);
		}
		assetDb = std::make_unique<AssetDatabase>();
		Deserializer::fromBytes<AssetDatabase>(*assetDb, Compression::decompress(assetDbBytes));
	}

	std::array<char, 16> ivEmpty;
	memset(ivEmpty.data(), 0, ivEmpty.size());
	const bool hasCrypt = memcmp(iv.data(), ivEmpty.data(), iv.size()) != 0 && !encryptionKey.isEmpty();

	if (preLoad || hasCrypt) {
		readToMemory();
	}

	if (hasCrypt) {
		decrypt(encryptionKey);
	}
}

AssetPack::~AssetPack()
{
}

AssetPack& AssetPack::operator=(AssetPack&& other)
{
	std::unique_lock<std::mutex> lock(other.readerMutex);

	assetDb = std::move(other.assetDb);
	dataOffset = other.dataOffset;
	reader = std::move(other.reader);
	data = std::move(other.data);
	hasReader = !!reader;

	other.hasReader = false;
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
	AssetPackHeader header;
	header.init(assetDbBytes.size());
	header.iv = iv;

	auto result = Bytes(size_t(header.dataStartPos + data.size()));
	memcpy(result.data(), &header, sizeof(AssetPackHeader));
	memcpy(result.data() + header.assetDbStartPos, assetDbBytes.data(), assetDbBytes.size());
	memcpy(result.data() + header.dataStartPos, data.data(), data.size());
	return result;
}

std::unique_ptr<ResourceData> AssetPack::getData(const String& asset, AssetType type, bool stream)
{
	auto path = asset;
	auto ps = assetDb->getDatabase(type).get(asset).path.split(':');
	size_t pos = size_t(ps.at(0).toInteger());
	size_t size = size_t(ps.at(1).toInteger());

	if (stream) {
		return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
			return std::make_unique<PackDataReader>(*this, pos, size);
		});
	} else {
		if (hasReader) {
			auto result = new char[size];
			try {
				readData(pos, gsl::as_writable_bytes(gsl::span<char>(result, size)));
				return std::make_unique<ResourceDataStatic>(result, size, path, true);
			} catch (...) {
				delete[] result;
				throw;
			}
		} else {
			// Preloaded
			if (pos + size > data.size()) {
				throw Exception("Asset \"" + asset + "\" is out of pack bounds.", HalleyExceptions::Resources);
			}

			return std::make_unique<ResourceDataStatic>(data.data() + pos, size, path, false);
		}
	}
}

void AssetPack::readToMemory()
{
	std::unique_lock<std::mutex> lock(readerMutex);
	reader->seek(dataOffset, SEEK_SET);
	data = reader->readAll();
	hasReader = false;
	reader.reset();
}

void AssetPack::encrypt(const String& key)
{
	// Generate IV
	Random::getGlobal().getBytes(gsl::as_writable_bytes(gsl::span<char>(iv)));
	Bytes ivBytes(iv.size());
	memcpy(ivBytes.data(), iv.data(), iv.size());

	data = Encrypt::encrypt(ivBytes, key, data);
}

void AssetPack::decrypt(const String& key)
{
	Bytes ivBytes(iv.size());
	memcpy(ivBytes.data(), iv.data(), iv.size());

	data = Encrypt::decrypt(ivBytes, key, data);
}

void AssetPack::readData(size_t pos, gsl::span<gsl::byte> dst)
{
	if (hasReader) {
		std::unique_lock<std::mutex> lock(readerMutex);
		if (reader) {
			reader->seek(pos + dataOffset, SEEK_SET);
			reader->read(dst);
			return;
		}
	}

	// Didn't read with reader, read from data
	if (pos + size_t(dst.size()) > data.size()) {
		throw Exception("Asset data is out of pack bounds.", HalleyExceptions::Resources);
	}
	memcpy(dst.data(), data.data() + pos, dst.size());
}

std::unique_ptr<ResourceDataReader> AssetPack::extractReader()
{
	std::unique_lock<std::mutex> lock(readerMutex);
	hasReader = false;
	return std::move(reader);
}

PackDataReader::PackDataReader(AssetPack& pack, size_t startPos, size_t fileSize)
	: pack(pack)
	, startPos(startPos)
	, fileSize(fileSize)
{
}

size_t PackDataReader::size() const
{
	return fileSize;
}

int PackDataReader::read(gsl::span<gsl::byte> dst)
{
	std::unique_lock<std::mutex> lock(mutex);
	size_t available = fileSize - curPos;
	size_t toRead = std::min(available, size_t(dst.size()));

	pack.readData(startPos + curPos, dst.subspan(0, toRead));
	curPos += toRead;

	return int(toRead);
}

void PackDataReader::seek(int64_t pos, int whence)
{
	std::unique_lock<std::mutex> lock(mutex);
	switch (whence) {
	case SEEK_SET:
		curPos = size_t(pos);
		break;
	case SEEK_CUR:
		curPos = size_t(curPos + pos);
		break;
	case SEEK_END:
		curPos = size_t(fileSize + pos);
		break;
	}
}

size_t PackDataReader::tell() const
{
	std::unique_lock<std::mutex> lock(mutex);
	return curPos;
}

void PackDataReader::close()
{
}

