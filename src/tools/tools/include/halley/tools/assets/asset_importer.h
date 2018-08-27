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
		IAssetImporter& getRootImporter(Path path) const;
		std::vector<std::reference_wrapper<IAssetImporter>> getImporters(ImportAssetType type) const;
		const std::vector<Path>& getAssetsSrc() const;

	private:
		std::map<ImportAssetType, std::vector<std::unique_ptr<IAssetImporter>>> importers;
		std::vector<Path> assetsSrc;
	};
}
