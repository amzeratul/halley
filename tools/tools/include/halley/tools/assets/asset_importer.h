#pragma once
#include "halley/plugin/iasset_importer.h"
#include <map>
#include <memory>
#include "halley/resources/resource.h"

namespace Halley
{
	class Project;

	class AssetImporter
	{
	public:
		AssetImporter(Project& project, const std::vector<Path>& assetsSrc);
		IAssetImporter& getImporter(Path path) const;
		IAssetImporter& getImporter(AssetType type) const;
		const std::vector<Path>& getAssetsSrc() const;

	private:
		std::map<AssetType, std::unique_ptr<IAssetImporter>> importers;
		std::vector<Path> assetsSrc;
	};
}
