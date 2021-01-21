#pragma once
#include "import_assets_database.h"
#include "halley/concurrency/task.h"
#include "halley/file/directory_monitor.h"

namespace Halley
{
	class Project;

	class CheckAssetsTask : public Task
	{
	public:
		CheckAssetsTask(Project& project, bool oneShot);
		~CheckAssetsTask();

		void requestRefreshAsset(Path path);

	protected:
		void run() override;

	private:
		Project& project;
		std::shared_ptr<AssetImporter> projectAssetImporter;

		DirectoryMonitor monitorAssets;
		DirectoryMonitor monitorAssetsSrc;
		DirectoryMonitor monitorSharedAssetsSrc;
		DirectoryMonitor monitorGen;
		DirectoryMonitor monitorGenSrc;
		DirectoryMonitor monitorSharedGen;
		DirectoryMonitor monitorSharedGenSrc;
		bool oneShot;
		std::vector<Path> directoryMetas;

		std::mutex mutex;
		std::condition_variable condition;

		std::vector<Path> inbox;
		std::vector<Path> pending;

		static bool hasAssetsToImport(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets);
		static std::vector<ImportAssetsDatabaseEntry> getAssetsToImport(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets);
		
		std::map<String, ImportAssetsDatabaseEntry> checkSpecificAssets(ImportAssetsDatabase& db, const std::vector<Path>& path);
		std::map<String, ImportAssetsDatabaseEntry> checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, bool collectDirMeta);
		bool requestImport(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry> assets, Path dstPath, String taskName, bool packAfter);
		std::optional<Path> findDirectoryMeta(const std::vector<Path>& metas, const Path& path) const;
		bool importFile(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry>& assets, bool isCodegen, bool skipGen, const std::vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath);
		void sleep(int ms);
	};
}
