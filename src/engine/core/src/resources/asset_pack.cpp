#include "resources/asset_pack.h"
#include "resources/asset_database.h"
#include "halley/resources/resource_data.h"
#include "halley/file/byte_serializer.h"
#include "halley/file/compression.h"

using namespace Halley;

AssetPack::AssetPack()
	: assetDb(std::make_unique<AssetDatabase>())
{
}

AssetPack::AssetPack(AssetPack&& other)
{
	*this = std::move(other);
}

AssetPack::AssetPack(std::unique_ptr<ResourceDataReader> _reader, const String& encryptionKey, bool preLoad)
	: reader(std::move(_reader))
{
	// Read header
	size_t totalSize = reader->size();
	if (totalSize < sizeof(AssetPackHeader)) {
		throw Exception("Asset pack is invalid (too small)");
	}
	AssetPackHeader header;
	reader->read(gsl::as_writeable_bytes(gsl::span<AssetPackHeader>(&header, 1)));
	if (memcmp(header.identifier.data(), "HALLEYPK", 8) != 0) {
		throw Exception("Asset pack is invalid (invalid identifier)");
	}
	dataOffset = header.dataStartPos;

	// Read asset database
	{
		auto assetDbBytes = Bytes(header.assetDbSize);
		reader->read(gsl::as_writeable_bytes(gsl::span<Byte>(assetDbBytes)));
		assetDb = std::make_unique<AssetDatabase>();
		Deserializer::fromBytes<AssetDatabase>(*assetDb, Compression::inflate(assetDbBytes));
	}

	if (preLoad || !encryptionKey.isEmpty()) {
		readToMemory();
	}

	if (!encryptionKey.isEmpty()) {
		decrypt(encryptionKey);
	}
}

AssetPack::~AssetPack()
{
}

AssetPack& AssetPack::operator=(AssetPack&& other)
{
	assetDb = std::move(other.assetDb);
	dataOffset = other.dataOffset;
	reader = std::move(other.reader);
	data = std::move(other.data);

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
	auto assetDbBytes = Compression::deflate(Serializer::toBytes(*assetDb));

	AssetPackHeader header;
	memcpy(header.identifier.data(), "HALLEYPK", 8);
	header.assetDbSize = assetDbBytes.size();
	header.dataStartPos = sizeof(AssetPackHeader) + assetDbBytes.size();

	auto result = Bytes(header.dataStartPos + data.size());
	memcpy(result.data(), &header, sizeof(AssetPackHeader));
	memcpy(result.data() + sizeof(AssetPackHeader), assetDbBytes.data(), assetDbBytes.size());
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
		if (reader) {
			// Stream from reader
			// TODO
			return {};
		} else {
			// Preloaded
			if (pos + size > data.size()) {
				throw Exception("Asset \"" + asset + "\" is out of pack bounds.");
			}

			return std::make_unique<ResourceDataStatic>(data.data() + pos, size, path, false);
		}
	}
}

void AssetPack::readToMemory()
{
	reader->seek(dataOffset, SEEK_SET);
	data = reader->readAll();
	reader.reset();
}

void AssetPack::encrypt(const String& key)
{
	// TODO
}

void AssetPack::decrypt(const String& key)
{
	// TODO
}

void AssetPack::readData(size_t pos, gsl::span<gsl::byte> dst) const
{
	if (reader) {
		// TODO
	} else {
		if (pos + size_t(dst.size()) > data.size()) {
			throw Exception("Asset data is out of pack bounds.");
		}
		memcpy(dst.data(), data.data() + pos, dst.size());
	}
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
	size_t available = fileSize - curPos;
	size_t toRead = std::min(available, size_t(dst.size()));

	pack.readData(curPos, dst.subspan(0, toRead));
	curPos += toRead;

	return int(toRead);
}

void PackDataReader::seek(int64_t pos, int whence)
{
	switch (whence) {
	case SEEK_SET:
		curPos = pos;
		break;
	case SEEK_CUR:
		curPos += pos;
		break;
	case SEEK_END:
		curPos = fileSize + pos;
		break;
	}
}

size_t PackDataReader::tell() const
{
	return curPos;
}

void PackDataReader::close()
{
}
