#pragma once

#include "halley/file/path.h"
#include "halley/tools/assets/asset_importer.h"
#include "halley/plugin/halley_plugin.h"
#include <memory>

namespace Halley
{
	class ImportAssetsDatabase;

	class HalleyStatics;
	class IHalleyPlugin;
	class DevConServer;
	class ProjectProperties;
	using HalleyPluginPtr = std::shared_ptr<IHalleyPlugin>;

	class Project
	{
	public:
		using AssetReloadCallback = std::function<void(const std::vector<String>&)>;

		Project(std::vector<String> platforms, Path projectRootPath, Path halleyRootPath, std::vector<HalleyPluginPtr> plugins);
		~Project();
		
		std::vector<String> getPlatforms() const;

		Path getRootPath() const;
		Path getUnpackedAssetsPath() const;
		Path getPackedAssetsPath(const String& platform) const;
		Path getAssetsSrcPath() const;
		Path getSharedAssetsSrcPath() const;

		Path getGenPath() const;
		Path getGenSrcPath() const;

		void setAssetPackManifest(const Path& path);
		Path getAssetPackManifestPath() const;

		ImportAssetsDatabase& getImportAssetsDatabase() const;
		ImportAssetsDatabase& getCodegenDatabase() const;

		const AssetImporter& getAssetImporter() const;
		std::vector<std::unique_ptr<IAssetImporter>> getAssetImportersFromPlugins(ImportAssetType type) const;

		void setDevConServer(DevConServer* server);
		void addAssetReloadCallback(AssetReloadCallback callback);
		
		ProjectProperties& getProperties() const;
		
		void reloadAssets(const std::set<String>& assets);

	private:
		std::vector<String> platforms;
		Path rootPath;
		Path halleyRootPath;
		Path assetPackManifest;

		std::vector<AssetReloadCallback> assetReloadCallbacks;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
		std::unique_ptr<AssetImporter> assetImporter;
		std::unique_ptr<ProjectProperties> properties;

		std::vector<HalleyPluginPtr> plugins;
	};
}
