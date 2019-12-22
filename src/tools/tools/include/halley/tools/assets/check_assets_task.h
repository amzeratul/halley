#pragma once
#include "../tasks/editor_task.h"
#include "import_assets_database.h"
#include "halley/file/directory_monitor.h"

namespace Halley
{
	class Project;

	class CheckAssetsTask : public EditorTask
	{
	public:
		CheckAssetsTask(Project& project, bool oneShot);
		~CheckAssetsTask();

		void requestRefreshAsset(Path path);

	protected:
		void run() override;

	private:
		Project& project;
		DirectoryMonitor monitorAssets;
		DirectoryMonitor monitorAssetsSrc;
		DirectoryMonitor monitorSharedAssetsSrc;
		DirectoryMonitor monitorGen;
		DirectoryMonitor monitorGenSrc;
		bool oneShot;
		std::vector<Path> directoryMetas;

		std::mutex mutex;
		std::condition_variable condition;

		std::vector<Path> inbox;
		std::vector<Path> pending;

		static std::vector<ImportAssetsDatabaseEntry> filterNeedsImporting(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets);
		std::map<String, ImportAssetsDatabaseEntry> checkSpecificAssets(ImportAssetsDatabase& db, const std::vector<Path>& path);
		std::map<String, ImportAssetsDatabaseEntry> checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, bool collectDirMeta);
		void requestImport(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry> assets, Path dstPath, String taskName, bool packAfter);
		Maybe<Path> findDirectoryMeta(const std::vector<Path>& metas, const Path& path) const;
		bool importFile(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry>& assets, const bool isCodegen, const std::vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath);
		void sleep(int ms);
	};
}
