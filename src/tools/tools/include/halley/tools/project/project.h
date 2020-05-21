#pragma once

#include "halley/file/path.h"
#include "halley/tools/assets/asset_importer.h"
#include "halley/plugin/halley_plugin.h"
#include <memory>

#include "halley/time/halleytime.h"
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
	class ECSData;
	class HalleyAPI;
	using HalleyPluginPtr = std::shared_ptr<IHalleyPlugin>;
	class Game;

	class Project
	{
	public:
		class IAssetLoadListener {
		public:
			virtual ~IAssetLoadListener() = default;
			virtual void onAssetsLoaded() {}
		};
		
		using AssetReloadCallback = std::function<void(const std::vector<String>&)>;

		Project(Path projectRootPath, Path halleyRootPath, const ProjectLoader& loader);
		~Project();
		
		void update(Time time);
		void onBuildDone();

		std::vector<String> getPlatforms() const;

		const Path& getHalleyRootPath() const;
		
		const Path& getRootPath() const;		
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
		ECSData& getECSData();

		const AssetImporter& getAssetImporter() const;
		std::vector<std::unique_ptr<IAssetImporter>> getAssetImportersFromPlugins(ImportAssetType type) const;

		void setDevConServer(DevConServer* server);
		void addAssetReloadCallback(AssetReloadCallback callback);
		void addAssetPackReloadCallback(AssetReloadCallback callback);
		void addAssetLoadedListener(IAssetLoadListener* listener);
		void removeAssetLoadedListener(IAssetLoadListener* listener);
		
		ProjectProperties& getProperties() const;

		Metadata getImportMetadata(AssetType type, const String& assetId) const;
		Metadata readMetadataFromDisk(const Path& filePath) const;
		void writeMetadataToDisk(const Path& filePath, const Metadata& metadata);

		std::vector<String> getAssetSrcList() const;
		std::vector<std::pair<AssetType, String>> getAssetsFromFile(const Path& path) const;

		void onAllAssetsImported();
		void reloadAssets(const std::set<String>& assets, bool packed);
		void reloadCodegen();
		void setCheckAssetTask(CheckAssetsTask* checkAssetsTask);
		void notifyAssetFileModified(Path path);

		Path getExecutablePath() const;

		void loadGameResources(const HalleyAPI& api);
		Resources& getGameResources();

		bool isDLLLoaded() const;
		
		template <typename F>
		void withDLL(F f) const
		{
			if (gameDll) {
				f(*gameDll);
			}
		}

		template <typename F>
		void withLoadedDLL(F f) const
		{
			if (isDLLLoaded()) {
				f(*gameDll);
			}
		}

		std::unique_ptr<Game> createGameInstance() const;

	private:
		std::vector<String> platforms;
		Path rootPath;
		Path halleyRootPath;
		Path assetPackManifest;

		std::vector<AssetReloadCallback> assetReloadCallbacks;
		std::vector<AssetReloadCallback> assetPackedReloadCallbacks;
		std::vector<IAssetLoadListener*> assetLoadedListeners;
		CheckAssetsTask* checkAssetsTask = nullptr;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
		std::unique_ptr<ImportAssetsDatabase> sharedCodegenDatabase;
		std::unique_ptr<AssetImporter> assetImporter;
		std::unique_ptr<ProjectProperties> properties;
		std::unique_ptr<ECSData> ecsData;

		std::vector<HalleyPluginPtr> plugins;
		std::shared_ptr<DynamicLibrary> gameDll;
		std::unique_ptr<Resources> gameResources;

		Path getDLLPath() const;
		void loadDLL();
		void loadECSData();
	};
}
