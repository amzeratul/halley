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
		
		void requestRefreshAssets(gsl::span<const Path> paths);

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
		Vector<Path> directoryMetas;

		std::mutex mutex;
		std::condition_variable condition;

		Vector<Path> inbox;
		Vector<Path> pending;

		static bool hasAssetsToImport(ImportAssetsDatabase& db, const HashMap<String, ImportAssetsDatabaseEntry>& assets);
		static Vector<ImportAssetsDatabaseEntry> getAssetsToImport(ImportAssetsDatabase& db, const HashMap<String, ImportAssetsDatabaseEntry>& assets);
		
		bool importAll(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Path dstPath, String taskName, bool packAfter);
		bool importChanged(const Vector<DirectoryMonitor::Event>& changes, ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Path dstPath, String taskName, bool packAfter);

		HashMap<String, ImportAssetsDatabaseEntry> checkSpecificAssets(ImportAssetsDatabase& db, const Vector<Path>& path);
		HashMap<String, ImportAssetsDatabaseEntry> checkChangedAssets(ImportAssetsDatabase& db, const Vector<DirectoryMonitor::Event>& changes, const Vector<Path>& srcPaths, const Path& dstPath, bool useDirMeta);
		HashMap<String, ImportAssetsDatabaseEntry> checkAllAssets(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta);
		Vector<DirectoryMonitor::Event> filterDuplicateChanges(const Vector<DirectoryMonitor::Event>& changes) const;

		bool requestImport(ImportAssetsDatabase& db, HashMap<String, ImportAssetsDatabaseEntry> assets, Path dstPath, String taskName, bool packAfter);
		std::optional<Path> findDirectoryMeta(const Vector<Path>& metas, const Path& path) const;
		bool doImportFile(ImportAssetsDatabase& db, HashMap<String, ImportAssetsDatabaseEntry>& assets, bool isCodegen, bool skipGen, const Vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath);
		bool importFile(ImportAssetsDatabase& db, HashMap<String, ImportAssetsDatabaseEntry>& assets, bool useDirMetas, const Path& srcPath, const Vector<Path>& srcPaths, const Path& filePath);
		void sleep(int ms);
	};
}
