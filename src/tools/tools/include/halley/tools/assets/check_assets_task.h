#pragma once
#include "import_assets_database.h"
#include "halley/concurrency/task.h"
#include "halley/file/directory_monitor.h"

namespace Halley
{
	class FileSystemCache;
	class Project;

	enum class ReimportType {
		ImportAll,
		ReimportAll,
		Codegen
	};

	template <>
	struct EnumNames<ReimportType> {
		constexpr std::array<const char*, 3> operator()() const {
			return{{
				"ImportAll",
				"ReimportAll",
				"Codegen"
			}};
		}
	};

	class CheckAssetsTask : public Task
	{
	public:
		CheckAssetsTask(Project& project, bool oneShot);
		~CheckAssetsTask();

		void requestRefreshAssets(gsl::span<const Path> paths);
		void requestReimport(ReimportType type);

	protected:
		void run() override;

	private:
		Project& project;
		FileSystemCache& fileSystemCache;
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
		std::optional<ReimportType> pendingReimport;

		using AssetTable = HashMap<std::pair<ImportAssetType, String>, ImportAssetsDatabaseEntry>;

		bool hasAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets);
		Vector<ImportAssetsDatabaseEntry> getAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets);

		bool importAll(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Path dstPath, String taskName, bool packAfter, Range<float> progressRange);

		AssetTable checkSpecificAssets(ImportAssetsDatabase& db, const Vector<Path>& path);
		AssetTable checkAllAssets(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Range<float> progressRange);

		bool requestImport(ImportAssetsDatabase& db, AssetTable assets, Path dstPath, String taskName, bool packAfter);
		std::optional<Path> findDirectoryMeta(const Vector<Path>& metas, const Path& path) const;
		bool doImportFile(ImportAssetsDatabase& db, AssetTable& assets, bool isCodegen, bool skipGen, const Vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath, Vector<std::pair<Path, Path>>* additionalFilesToImport);
		bool importFile(ImportAssetsDatabase& db, AssetTable& assets, bool useDirMetas, const Path& srcPath, const Vector<Path>& srcPaths, const Path& filePath);
		void sleep(int ms);
	};
}
