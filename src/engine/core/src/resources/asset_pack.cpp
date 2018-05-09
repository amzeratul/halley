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
