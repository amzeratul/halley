#pragma once

#include "halley/file/filesystem.h"
#include "halley/tools/assets/asset_importer.h"

namespace Halley
{
	class ImportAssetsDatabase;

	class Project
	{
	public:
		Project(Path rootPath, Path sharedAssetsSrcPath);
		~Project();
		
		Path getAssetsPath() const;
		Path getAssetsSrcPath() const;
		Path getSharedAssetsSrcPath() const;

		Path getGenPath() const;
		Path getGenSrcPath() const;

		ImportAssetsDatabase& getImportAssetsDatabase();
		ImportAssetsDatabase& getCodegenDatabase();

		const AssetImporter& getAssetImporter() const;
	
	private:
		Path rootPath;
		Path sharedAssetsSrcPath;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
		std::unique_ptr<AssetImporter> assetImporter;
	};
}
