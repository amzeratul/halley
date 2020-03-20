#pragma once

#include "halley/file/path.h"
#include "halley/tools/assets/asset_importer.h"
#include "halley/plugin/halley_plugin.h"
#include <memory>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/dll/dynamic_library.h"

namespace Halley
{
	class ProjectLoader;
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

		Project(Path projectRootPath, Path halleyRootPath, const ProjectLoader& loader);
		~Project();
		
		std::vector<String> getPlatforms() const;

		Path getRootPath() const;
		Path getUnpackedAssetsPath() const;
		Path getPackedAssetsPath(const String& platform) const;
		Path getAssetsSrcPath() const;
		Path getSharedAssetsSrcPath() const;

		Path getGenPath() const;
		Path getGenSrcPath() const;
		Path getSharedGenPath() const;
		Path getSharedGenSrcPath() const;

		void setAssetPackManifest(const Path& path);
		Path getAssetPackManifestPath() const;

		ImportAssetsDatabase& getImportAssetsDatabase() const;
		ImportAssetsDatabase& getCodegenDatabase() const;
		ImportAssetsDatabase& getSharedCodegenDatabase() const;

		const AssetImporter& getAssetImporter() const;
		std::vector<std::unique_ptr<IAssetImporter>> getAssetImportersFromPlugins(ImportAssetType type) const;

		void setDevConServer(DevConServer* server);
		void addAssetReloadCallback(AssetReloadCallback callback);
		void addAssetPackReloadCallback(AssetReloadCallback callback);
		
		ProjectProperties& getProperties() const;

		Metadata getImportMetadata(AssetType type, const String& assetId) const;
		Metadata readMetadataFromDisk(const Path& filePath) const;
		void writeMetadataToDisk(const Path& filePath, const Metadata& metadata);

		std::vector<String> getAssetSrcList() const;
		std::vector<std::pair<AssetType, String>> getAssetsFromFile(const Path& path) const;

		void reloadAssets(const std::set<String>& assets, bool packed);
		void setCheckAssetTask(CheckAssetsTask* checkAssetsTask);
		void notifyAssetFileModified(Path path);

	private:
		std::vector<String> platforms;
		Path rootPath;
		Path halleyRootPath;
		Path assetPackManifest;

		std::vector<AssetReloadCallback> assetReloadCallbacks;
		std::vector<AssetReloadCallback> assetPackedReloadCallbacks;
		CheckAssetsTask* checkAssetsTask = nullptr;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
		std::unique_ptr<ImportAssetsDatabase> sharedCodegenDatabase;
		std::unique_ptr<AssetImporter> assetImporter;
		std::unique_ptr<ProjectProperties> properties;

		std::vector<HalleyPluginPtr> plugins;
		std::shared_ptr<DynamicLibrary> gameDll;
	};
}
