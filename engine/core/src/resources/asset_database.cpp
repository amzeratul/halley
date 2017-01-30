#include "halley/core/resources/asset_database.h"
#include "halley/file/byte_serializer.h"

using namespace Halley;

void AssetDatabase::Entry::serialize(Serializer& s) const
{
	s << path;
	s << meta;
}

void AssetDatabase::Entry::deserialize(Deserializer& s)
{
	s >> path;
	s >> meta;
}

void AssetDatabase::TypedDB::add(const String& name, Entry&& asset)
{
	assets[name] = std::move(asset);
}

const AssetDatabase::Entry& AssetDatabase::TypedDB::get(const String& name)
{
	auto i = assets.find(name);
	if (i == assets.end()) {
		throw Exception("Asset not found: " + name);
	}
	return i->second;
}

void AssetDatabase::TypedDB::serialize(Serializer& s) const
{
	s << assets;
}

void AssetDatabase::TypedDB::deserialize(Deserializer& s)
{
	s >> assets;
}

void AssetDatabase::addAsset(const String& name, AssetType type, Entry&& entry)
{
	dbs[int(type)].add(name, std::move(entry));
}

const AssetDatabase::TypedDB& AssetDatabase::getDatabase(AssetType type) const
{
	return dbs[int(type)];
}

void AssetDatabase::serialize(Serializer& s) const
{
	s << dbs;
}

void AssetDatabase::deserialize(Deserializer& s)
{
	s >> dbs;
}
