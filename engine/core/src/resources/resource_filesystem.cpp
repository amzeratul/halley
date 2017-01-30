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
#include "resources/resource_filesystem.h"
#include "halley/core/api/system_api.h"
#include "halley/file/byte_serializer.h"

using namespace Halley;

FileSystemResourceLocator::FileSystemResourceLocator(SystemAPI& system, Path _basePath)
	: system(system)
    , basePath(_basePath)
{
	// Read assetDb
	assetDb = std::make_unique<AssetDatabase>();
	auto reader = system.getDataReader((basePath / "asset.db").string());

	Bytes result(reader->size());
	reader->read(gsl::as_writeable_bytes(gsl::span<Byte>(result)));
	Deserializer s(result);
	s >> *assetDb;
}

const AssetDatabase& FileSystemResourceLocator::getAssetDatabase() const
{
	return *assetDb;
}

std::unique_ptr<ResourceData> FileSystemResourceLocator::getData(const String& path, AssetType type, bool stream)
{
	if (stream) {
		return std::make_unique<ResourceDataStream>(path, [=] () -> std::unique_ptr<ResourceDataReader> {
			return system.getDataReader(path.cppStr());
		});
	} else {
		auto fp = system.getDataReader(path.cppStr());
		if (!fp) {
			return std::unique_ptr<ResourceDataStatic>();
		}

		size_t size = fp->size();
		char* buf = new char[size];
		fp->read(gsl::as_writeable_bytes(gsl::span<char>(buf, size)));
		return std::make_unique<ResourceDataStatic>(buf, size, path);
	}
}
