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
#include "halley/core/api/system_api.h"
#include "halley/file/byte_serializer.h"

using namespace Halley;

FileSystemResourceLocator::FileSystemResourceLocator(SystemAPI& system, const Path& _basePath)
	: system(system)
    , basePath(_basePath)
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

void FileSystemResourceLocator::purge(SystemAPI& system)
{
	assetDb.reset();
}

void FileSystemResourceLocator::loadAssetDb()
{
	assetDb = std::make_unique<AssetDatabase>();
	auto reader = system.getDataReader((basePath / "assets.db").string());
	if (!reader) {
		throw Exception("Unable to load assets.");
	}

	Deserializer::fromBytes<AssetDatabase>(*assetDb, reader->readAll());
}

std::unique_ptr<ResourceData> FileSystemResourceLocator::getData(const String& asset, AssetType type, bool stream)
{
	if (!assetDb) {
		loadAssetDb();
	}

	auto path = (basePath / assetDb->getDatabase(type).get(asset).path).string();

	if (stream) {
		return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
			return system.getDataReader(path);
		});
	} else {
		auto fp = system.getDataReader(path);
		if (!fp) {
			return std::unique_ptr<ResourceDataStatic>();
		}

		size_t size = fp->size();
		char* buf = new char[size];
		fp->read(gsl::as_writeable_bytes(gsl::span<char>(buf, size)));
		return std::make_unique<ResourceDataStatic>(buf, size, path);
	}
}
