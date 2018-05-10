#pragma once

#include "halley/file/path.h"
#include "halley/tools/assets/asset_importer.h"
#include "halley/plugin/halley_plugin.h"

namespace Halley
{
	class HalleyStatics;
	class ImportAssetsDatabase;

	class Project
	{
	public:
		Project(const HalleyStatics& statics, const String& platform, Path projectRootPath, Path halleyRootPath);
		~Project();
		
		Path getUnpackedAssetsPath() const;
		Path getPackedAssetsPath() const;
		Path getAssetsSrcPath() const;
		Path getSharedAssetsSrcPath() const;

		Path getGenPath() const;
		Path getGenSrcPath() const;

		ImportAssetsDatabase& getImportAssetsDatabase();
		ImportAssetsDatabase& getCodegenDatabase();

		const AssetImporter& getAssetImporter() const;
		std::unique_ptr<IAssetImporter> getAssetImporterOverride(ImportAssetType type) const;
	
	private:
		const HalleyStatics& statics;
		String platform;
		Path rootPath;
		Path halleyRootPath;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
		std::unique_ptr<AssetImporter> assetImporter;

		using HalleyPluginPtr = std::unique_ptr<IHalleyPlugin, std::function<void(IHalleyPlugin*)>>;
		std::vector<HalleyPluginPtr> plugins;

		void initialisePlugins();
		HalleyPluginPtr loadPlugin(const Path& path);
	};
}
