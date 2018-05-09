#include "resources/asset_pack.h"
#include "resources/asset_database.h"
#include "halley/resources/resource_data.h"
#include "halley/file/byte_serializer.h"

using namespace Halley;

AssetPack::AssetPack()
	: assetDb(std::make_unique<AssetDatabase>())
{
}

AssetPack::AssetPack(AssetPack&& other)
{
	*this = std::move(other);
}

AssetPack::AssetPack(std::unique_ptr<ResourceDataReader> reader, const String& encryptionKey, bool readToMemory)
{
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

void AssetPack::serialize(Serializer& s) const
{
	writeHeader(s);
	s << data;
}

void AssetPack::deserialize(Deserializer& s)
{
	readHeader(s);
	s >> data;
}

void AssetPack::writeHeader(Serializer& s) const
{
	s << *assetDb;
}

void AssetPack::readHeader(Deserializer& s)
{
	s >> *assetDb;
}
