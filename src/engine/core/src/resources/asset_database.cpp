#include "halley/core/resources/asset_database.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/exception.h"
#include "halley/resources/resource.h"
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

AssetDatabase::TypedDB::TypedDB()
{
}

AssetDatabase::TypedDB::TypedDB(AssetType type)
	: type(type)
{
}

void AssetDatabase::TypedDB::add(const String& name, Entry&& asset)
{
	assets[name] = std::move(asset);
}

const AssetDatabase::Entry& AssetDatabase::TypedDB::get(const String& name) const
{
	auto i = assets.find(name);
	if (i == assets.end()) {
		throw Exception("Asset not found: " + toString(type) + ":" + name, HalleyExceptions::Resources);
	}
	return i->second;
}

void AssetDatabase::TypedDB::serialize(Serializer& s) const
{
	s << type;
	s << assets;
}

void AssetDatabase::TypedDB::deserialize(Deserializer& s)
{
	s >> type;
	s >> assets;
}

const HashMap<String, AssetDatabase::Entry>& AssetDatabase::TypedDB::getAssets() const
{
	return assets;
}

AssetType AssetDatabase::TypedDB::getType() const
{
	return type;
}

void AssetDatabase::addAsset(const String& name, AssetType type, Entry&& entry)
{
	const auto iter = dbs.find(static_cast<int>(type));
	if (iter != dbs.end()) {
		iter->second.add(name, std::move(entry));
	} else {
		TypedDB db(type);
		db.add(name, std::move(entry));
		dbs[static_cast<int>(type)] = std::move(db);
	}
}

const AssetDatabase::TypedDB& AssetDatabase::getDatabase(AssetType type) const
{
	const int key = int(type);
	const auto iter = dbs.find(key);
	if (iter == dbs.end()) {
		const auto res = dbs.insert(std::make_pair(key, TypedDB(type)));
		return res.first->second;
	}
	return iter->second;
}

bool AssetDatabase::hasDatabase(AssetType type) const
{
	return dbs.find(static_cast<int>(type)) != dbs.end();
}

std::vector<String> AssetDatabase::getAssets() const
{
	std::set<String> contains;
	std::vector<String> result;
	for (auto& db: dbs) {
		String prefix = toString(AssetType(db.first)) + ":";
		for (const auto& asset: db.second.getAssets()) {
			String name = prefix + asset.first;
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
	if (hasDatabase(type)) {
		for (auto& asset: getDatabase(type).getAssets()) {
			result.push_back(asset.first);
		}
	}
	return result;
}
