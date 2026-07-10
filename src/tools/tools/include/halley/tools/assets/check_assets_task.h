#pragma once
#include "import_assets_database.h"
#include "halley/concurrency/task.h"
#include "halley/file/directory_monitor.h"
#include "halley/time/stopwatch.h"

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
		constexpr auto operator()() const {
			return std::to_array({
				"ImportAll",
				"ReimportAll",
				"Codegen"
			});
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

		Mutex mutex;
		ConditionVariable condition;

		std::optional<ReimportType> pendingReimport;

		uint32_t importFileCounter = 0;
		uint32_t doImportFileCounter = 0;
		Stopwatch additionalImportSW;

		using AssetTable = HashMap<std::pair<ImportAssetType, String>, ImportAssetsDatabaseEntry>;

		struct DirMetaInfo {
			std::optional<Path> path; // Absolute (i.e. includes srcPath)
			int64_t timestamp = 0;
		};

		struct FileTimes {
			int64_t file = 0;
			std::optional<int64_t> privateMeta; // Timestamp of the "file.ext.meta" sibling, if present
		};

		Vector<ImportAssetsDatabaseEntry> getAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets);

		bool importAll(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Path dstPath, String taskName, bool packAfter, Range<float> progressRange);

		AssetTable checkAllAssets(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Range<float> progressRange);

		bool requestImport(ImportAssetsDatabase& db, AssetTable assets, Path dstPath, String taskName, bool packAfter);
		const Path* findDirectoryMeta(const Vector<Path>& metas, const Path& parentDir) const;
		DirMetaInfo resolveDirMeta(const Vector<Path>& metas, const Path& srcPath, const Path& parentDir);
		bool doImportFile(ImportAssetsDatabase& db, AssetTable& assets, bool isCodegen, bool skipGen, const Vector<Path>& directoryMetas, const DirMetaInfo* dirMeta, const FileTimes* times, const Path& srcPath, const Path& filePath, Vector<std::pair<Path, Path>>* additionalFilesToImport);
		bool importFile(ImportAssetsDatabase& db, AssetTable& assets, bool useDirMetas, const DirMetaInfo& dirMeta, const FileTimes& times, const Path& srcPath, const Vector<Path>& srcPaths, const Path& filePath);
		void sleep(int ms);
	};
}
