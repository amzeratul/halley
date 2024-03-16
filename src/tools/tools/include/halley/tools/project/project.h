#pragma once

#include "halley/file/path.h"
#include "halley/tools/assets/asset_importer.h"
#include "halley/plugin/halley_plugin.h"
#include <memory>



#include "halley/game/scene_editor_interface.h"
#include "halley/editor_extensions/asset_preview_generator.h"
#include "halley/time/halleytime.h"
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/dll/project_dll.h"

namespace Halley
{
	class FileSystemCache;
	class ProjectComments;
	class GameProperties;
	class IHalleyEntryPoint;
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

	class Project : public IProject
	{
	public:
		class IAssetLoadListener {
		public:
			virtual ~IAssetLoadListener() = default;
			virtual void onAssetsLoaded() {}
		};

		class IAssetSrcChangeListener {
		public:
			virtual ~IAssetSrcChangeListener() = default;
			virtual void onAssetsSrcChanged() {}
			virtual void onGenSrcChanged() {}
		};
		
		using AssetReloadCallback = std::function<void(gsl::span<const String>)>;
		using AssetPackedReloadCallback = std::function<void(gsl::span<const String>, gsl::span<const String>)>;

		Project(Path projectRootPath, Path halleyRootPath, Vector<String> disabledPlatforms);
		~Project() override;

		void loadDLL(const HalleyStatics& statics, bool load);
		void setupImporter(Vector<HalleyPluginPtr> plugins, const ConfigNode& importerOptions);
		
		void update(Time time);

		bool isBuildPending() const;
		void onBuildNeeded();
		void onBuildStarted();
		void onBuildDone();

		void setPlatforms(Vector<String> platforms);
		const Vector<String>& getPlatforms() const;

		const Path& getHalleyRootPath() const;
		
		const Path& getRootPath() const;		
		Path getUnpackedAssetsPath() const;
		Path getPackedAssetsPath(const String& platform) const;
		Path getAssetsSrcPath() const override;
		Path getSharedAssetsSrcPath() const;
		Path getEditorAssetsSrcPath() const;

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

		const std::shared_ptr<AssetImporter>& getAssetImporter() const;
		Vector<std::unique_ptr<IAssetImporter>> getAssetImportersFromPlugins(ImportAssetType type) const;

		void setDevConServer(DevConServer* server);
		DevConServer* getDevConServer() const;

		[[deprecated]] size_t addAssetReloadCallback(AssetReloadCallback callback);
		void removeAssetReloadCallback(size_t idx);
		size_t addAssetPackReloadCallback(AssetPackedReloadCallback callback);
		void removeAssetPackReloadCallback(size_t idx);
		void addAssetLoadedListener(IAssetLoadListener* listener);
		void removeAssetLoadedListener(IAssetLoadListener* listener);
		void notifyAssetsSrcChanged();
		void notifyGenSrcChanged();
		void addAssetSrcChangeListener(IAssetSrcChangeListener& listener);
		void removeAssetSrcChangeListener(IAssetSrcChangeListener& listener);

		ProjectProperties& getProperties() const;
		ProjectComments& getComments() const;
		GameProperties& getGameProperties() const;

		Metadata readMetadataFromDisk(const Path& filePath) const;
		void writeMetadataToDisk(const Path& filePath, const Metadata& metadata);

		void setAssetSaveNotification(bool enabled) override;
		bool isAssetSaveNotificationEnabled() const;
		bool writeAssetToDisk(const Path& filePath, gsl::span<const gsl::byte> data) override;
		bool writeAssetToDisk(const Path& filePath, const Bytes& data) override;
		bool writeAssetToDisk(const Path& filePath, std::string_view str) override;

		Vector<String> getAssetSrcList(bool includeDirs, const Path& relPath, bool recursive) const;
		Vector<std::pair<AssetType, String>> getAssetsFromFile(const Path& path) const;

		void onAllAssetsImported();
		void reloadAssets(const std::set<String>& assets, const Vector<String>& packs, bool packed);
		void reloadCodegen();
		void setCheckAssetTask(CheckAssetsTask* checkAssetsTask);
		void notifyAssetFilesModified(gsl::span<const Path> paths);

		Path getExecutablePath() const;

		void loadGameResources(const HalleyAPI& api);
		Resources& getGameResources() override;

		bool isDLLLoaded() const;
		bool areAssetsLoaded() const;
		ProjectDLL::Status getDLLStatus() const;
		
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

		Game* getGameInstance() const override;

		std::optional<AssetPreviewData> getCachedAssetPreview(AssetType type, const String& id);
		void setCachedAssetPreview(AssetType type, const String& id, AssetPreviewData data);
		void clearCachedAssetPreviews();

		FileSystemCache& getFileSystemCache();

		void requestReimport(ReimportType reimport);

		void launchGame(Vector<String> params) const override;

		static uint64_t getSourceHash(const Path& projectRoot);
		uint64_t getSourceHash() const;
		String getBuiltSourceStr() const;
		bool isBuildSourceUpToDate() const;
		uint32_t getBuildCount() const;

	private:
		Vector<String> platforms;
		Path rootPath;
		Path halleyRootPath;
		Path assetPackManifest;
		size_t callbackIdx = 0;

		Vector<std::pair<size_t, AssetReloadCallback>> assetReloadCallbacks;
		Vector<std::pair<size_t, AssetPackedReloadCallback>> assetPackedReloadCallbacks;
		Vector<IAssetLoadListener*> assetLoadedListeners;
		Vector<IAssetSrcChangeListener*> assetSrcChangeListeners;
		CheckAssetsTask* checkAssetsTask = nullptr;

		std::unique_ptr<ImportAssetsDatabase> importAssetsDatabase;
		std::unique_ptr<ImportAssetsDatabase> codegenDatabase;
		std::unique_ptr<ImportAssetsDatabase> sharedCodegenDatabase;
		std::shared_ptr<AssetImporter> assetImporter;

		std::unique_ptr<ProjectProperties> properties;
		std::unique_ptr<ProjectComments> comments;
		std::unique_ptr<GameProperties> gameProperties;
		std::unique_ptr<ECSData> ecsData;

		Vector<HalleyPluginPtr> plugins;
		std::shared_ptr<ProjectDLL> gameDll;
		std::unique_ptr<Resources> gameResources;

		std::unique_ptr<FileSystemCache> fileSystemCache;

		bool assetNotifyImportEnabled = true;
		bool assetsImported = false;
		bool allowPackedAssets = false;
		Vector<Path> assetsToNotifyImport;

		bool buildPending = false;
		uint32_t buildCount = 0;

		DevConServer* devConServer = nullptr;

		struct AssetPreviewCache {
			int64_t timestamp;
			AssetPreviewData data;
		};
		std::map<std::pair<AssetType, String>, AssetPreviewCache> previewCache;

		Path getDLLPath() const;
		void loadECSData();
	};
}
