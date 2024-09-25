/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include <cassert>
#include "resource_filesystem.h"
#include "halley/api/system_api.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/game/scene_editor_interface.h"

using namespace Halley;

FileSystemResourceLocator::FileSystemResourceLocator(SystemAPI& system, const Path& _basePath, IFileSystemCache* cache)
	: system(system)
    , basePath(_basePath)
	, assetDBLoaded(false)
	, cache(cache)
{
	loadAssetDb();
}

const AssetDatabase& FileSystemResourceLocator::getAssetDatabase()
{
	if (!assetDb) {
		loadAssetDb();
	}
	return *assetDb;
}

int FileSystemResourceLocator::getPriority() const
{
	return 1;
}

void FileSystemResourceLocator::purgeAll(SystemAPI& system)
{
	assetDb.reset();
}

bool FileSystemResourceLocator::purgeIfAffected(SystemAPI& system, gsl::span<const String> assetIds, gsl::span<const String> packIds)
{
	assetDb.reset();
	return true;
}

String FileSystemResourceLocator::getName() const
{
	return "filesystem";
}

size_t FileSystemResourceLocator::getMemoryUsage() const
{
	return sizeof(*this) + assetDb->getMemoryUsage();
}

void FileSystemResourceLocator::loadAssetDb()
{
	auto lock = std::unique_lock(mutex);
	if (assetDb) {
		return;
	}

	assetDb = std::make_unique<AssetDatabase>();
	auto reader = system.getDataReader((basePath / "assets.db").string());
	if (!reader) {
		throw Exception("Unable to load assets.", HalleyExceptions::Resources);
	}

	Deserializer::fromBytes<AssetDatabase>(*assetDb, reader->readAll());

	assetDBLoaded = true;
}

std::unique_ptr<ResourceData> FileSystemResourceLocator::getData(const String& asset, AssetType type, bool stream)
{
	if (!assetDBLoaded) {
		loadAssetDb();
	}

	auto path = (basePath / assetDb->getDatabase(type).get(asset).path).string();

	if (stream) {
		return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
			return system.getDataReader(path);
		});
	} else if (cache) {
		auto data = cache->readFile(path);
		auto size = data.size();
		char* buf = new char[size];
		memcpy(buf, data.data(), data.size());
		return std::make_unique<ResourceDataStatic>(buf, size, path);
	} else {
		auto fp = system.getDataReader(path);
		if (!fp) {
			return std::unique_ptr<ResourceDataStatic>();
		}

		size_t size = fp->size();
		char* buf = new char[size];
		fp->read(gsl::as_writable_bytes(gsl::span<char>(buf, size)));
		return std::make_unique<ResourceDataStatic>(buf, size, path);
	}
}
