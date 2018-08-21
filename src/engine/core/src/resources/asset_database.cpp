#include "halley/core/resources/asset_database.h"
#include "halley/file/byte_serializer.h"
#include "halley/support/exception.h"
#include <set>

using namespace Halley;

AssetDatabase::Entry::Entry() {}

AssetDatabase::Entry::Entry(const String& path, const Metadata& meta)
	: path(path)
	, meta(meta)
{}

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

const AssetDatabase::Entry& AssetDatabase::TypedDB::get(const String& name) const
{
	auto i = assets.find(name);
	if (i == assets.end()) {
		throw Exception("Asset not found: " + name, HalleyExceptions::Resources);
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

const HashMap<String, AssetDatabase::Entry>& AssetDatabase::TypedDB::getAssets() const
{
	return assets;
}

void AssetDatabase::addAsset(const String& name, AssetType type, Entry&& entry)
{
	dbs[int(type)].add(name, std::move(entry));
}

const AssetDatabase::TypedDB& AssetDatabase::getDatabase(AssetType type) const
{
	return dbs[int(type)];
}

std::vector<String> AssetDatabase::getAssets() const
{
	std::set<String> contains;
	std::vector<String> result;
	for (auto& db: dbs) {
		for (auto& asset: db.second.getAssets()) {
			const String& name = asset.first;
			if (contains.find(name) == contains.end()) {
				contains.insert(name);
				result.push_back(name);
			}
		}
	}

	return result;
}

void AssetDatabase::serialize(Serializer& s) const
{
	s << dbs;
}

void AssetDatabase::deserialize(Deserializer& s)
{
	s >> dbs;
}

std::vector<String> AssetDatabase::enumerate(AssetType type) const
{
	std::vector<String> result;
	for (auto& asset: getDatabase(type).getAssets()) {
		result.push_back(asset.first);
	}
	return result;
}
