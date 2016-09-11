#pragma once
#include "iasset_importer.h"
#include "halley/file/filesystem.h"
#include "halley/text/halleystring.h"
#include <map>
#include <memory>

namespace Halley
{
	class AssetImporter
	{
	public:
		AssetImporter();
		IAssetImporter& getImporter(Path path) const;
		IAssetImporter& getImporter(AssetType type) const;

	private:
		std::map<AssetType, std::unique_ptr<IAssetImporter>> importers;
	};
}
