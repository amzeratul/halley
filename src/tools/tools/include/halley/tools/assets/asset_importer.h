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
		AssetImporter(Project& project, Vector<Path> assetsSrc);

		ImportAssetType getImportAssetType(const Path& path, bool skipRedundantTypes) const;
		IAssetImporter& getRootImporter(const Path& path) const;
		Vector<std::reference_wrapper<IAssetImporter>> getImporters(ImportAssetType type) const;
		const Vector<Path>& getAssetsSrc() const;

	private:
		std::map<ImportAssetType, Vector<std::unique_ptr<IAssetImporter>>> importers;
		Vector<Path> assetsSrc;
		bool importByExtension = false;

		void addImporter(Vector<std::unique_ptr<IAssetImporter>>& dst, std::unique_ptr<IAssetImporter> importer);
	};
}
