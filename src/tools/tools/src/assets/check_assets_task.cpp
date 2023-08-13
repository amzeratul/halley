#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include "halley/tools/file/filesystem.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/concurrency/concurrent.h"
#include "halley/support/logger.h"
#include "halley/tools/file/filesystem_cache.h"

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool oneShot)
	: Task("Checking assets", true, true)
	, project(project)
	, fileSystemCache(project.getFileSystemCache())
	, monitorAssets(project.getUnpackedAssetsPath())
	, monitorAssetsSrc(project.getAssetsSrcPath())
	, monitorSharedAssetsSrc(project.getSharedAssetsSrcPath())
	, monitorGen(project.getGenPath())
	, monitorGenSrc(project.getGenSrcPath())
	, monitorSharedGen(project.getSharedGenPath())
	, monitorSharedGenSrc(project.getSharedGenSrcPath())
	, oneShot(oneShot)
{
	project.setCheckAssetTask(this);
}

CheckAssetsTask::~CheckAssetsTask()
{
	project.setCheckAssetTask(nullptr);
}

void CheckAssetsTask::run()
{
	bool first = true;
	while (!isCancelled()) {
		bool importing = false;

		if (first) {
			setProgress(0, "Enumerating filesystem");
			fileSystemCache.trackDirectory(project.getAssetsSrcPath());
			fileSystemCache.trackDirectory(project.getSharedAssetsSrcPath());
			fileSystemCache.trackDirectory(project.getGenSrcPath());
			fileSystemCache.trackDirectory(project.getSharedGenSrcPath());
			fileSystemCache.trackDirectory(project.getUnpackedAssetsPath());
			fileSystemCache.trackDirectory(project.getSharedGenPath());
			fileSystemCache.trackDirectory(project.getGenPath());
		}

		projectAssetImporter = project.getAssetImporter();

		decltype(pendingReimport) curPendingReimport;
		{
			std::unique_lock<std::mutex> lock(mutex);
			curPendingReimport = pendingReimport;
			pendingReimport = {};
		}

		// Wait for the import to finish, otherwise the DB won't be updated and it'll try updating the same assets twice
		while (hasPendingTasks()) {
			sleep(5);
		}

		// Check if any files changed
		Vector<DirectoryMonitor::Event> assetsSrcChanged;
		Vector<DirectoryMonitor::Event> genSrcChanged;
		Vector<DirectoryMonitor::Event> assetsChanged;
		Vector<DirectoryMonitor::Event> genChanged;
		monitorAssetsSrc.poll(assetsSrcChanged, true);
		monitorSharedAssetsSrc.poll(assetsSrcChanged, true);
		monitorAssets.poll(assetsChanged, true);
		monitorSharedGenSrc.poll(genSrcChanged, true);
		monitorGenSrc.poll(genSrcChanged, true);
		monitorSharedGen.poll(genChanged, true);
		monitorGen.poll(genChanged, true);
		fileSystemCache.notifyChanges(assetsSrcChanged);
		fileSystemCache.notifyChanges(genSrcChanged);
		fileSystemCache.notifyChanges(assetsChanged);
		fileSystemCache.notifyChanges(genChanged);

		if (!assetsSrcChanged.empty()) {
			Concurrent::execute(Executors::getMainUpdateThread(), [=]()
			{
				project.notifyAssetsSrcChanged();
			});
		}
		if (!genSrcChanged.empty()) {
			Concurrent::execute(Executors::getMainUpdateThread(), [=]()
			{
				project.notifyGenSrcChanged();
			});
		}

		// First or Re-import
		const bool hasCodeGen = first || !genSrcChanged.empty() || curPendingReimport == ReimportType::Codegen;
		const bool hasAssets = first || !assetsSrcChanged.empty() || curPendingReimport == ReimportType::ImportAll || curPendingReimport == ReimportType::ReimportAll;
		if (hasCodeGen || hasAssets) {
			if (curPendingReimport) {
				setVisible(true);
			}

			if (hasCodeGen) {
				if (curPendingReimport == ReimportType::Codegen) {
					using namespace std::chrono_literals;
					FileSystem::remove(project.getGenPath());
					FileSystem::remove(project.getSharedGenPath());
					std::this_thread::sleep_for(1s);
					project.getCodegenDatabase().clear();
					project.getSharedCodegenDatabase().clear();
				}
				const float rangeEnd = hasAssets ? 0.1f : 1.0f;
				importing |= importAll(project.getCodegenDatabase(), { project.getSharedGenSrcPath(), project.getGenSrcPath() }, false, project.getGenPath(), "Generating code", false, Range(0.0f, rangeEnd * 0.5f));
				importing |= importAll(project.getSharedCodegenDatabase(), { project.getSharedGenSrcPath() }, false, project.getSharedGenPath(), "Generating code", false, Range(rangeEnd * 0.5f, rangeEnd));
			}
			if (hasAssets) {
				if (curPendingReimport == ReimportType::ReimportAll) {
					project.getImportAssetsDatabase().clear();
				}
				const float rangeStart = hasCodeGen ? 0.1f : 0.0f;
				importing |= importAll(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, true, project.getUnpackedAssetsPath(), "Importing assets", true, Range(rangeStart, 1.0f));
			}
			setVisible(false);
			while (hasPendingTasks()) {
				sleep(5);
			}
		}
		
		if ((importing || first) && !project.getImportAssetsDatabase().hasFailedFiles()) {
			Concurrent::execute(Executors::getMainUpdateThread(), [project = &project] () {
				Logger::logDev("Notifying assets imported");
				project->onAllAssetsImported();
			});
		}
		
		if (oneShot) {
			return;
		} else {
			first = false;
		}

		sleep(monitorAssets.hasRealImplementation() ? 20 : 1000);
	}
}

bool CheckAssetsTask::importAll(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Path dstPath, String taskName, bool packAfter, Range<float> progressRange)
{
	if (isCancelled()) {
		return false;
	}
	const auto assets = checkAllAssets(db, srcPaths, collectDirMeta, progressRange);

	if (isCancelled()) {
		return false;
	}
	return requestImport(db, assets, std::move(dstPath), std::move(taskName), packAfter);
}

bool CheckAssetsTask::importFile(ImportAssetsDatabase& db, AssetTable& assets, bool useDirMetas, const Path& srcPath, const Vector<Path>& srcPaths, const Path& filePath)
{
	if (filePath.getExtension() == ".meta") {
		return false;
	}

	const bool isCodegen = srcPath == project.getGenSrcPath() || srcPath == project.getSharedGenSrcPath();
	const bool skipGen = srcPath == project.getSharedGenSrcPath() && srcPaths.size() > 1;

	const auto& basePath = skipGen ? project.getGenSrcPath() : srcPath;
	const auto& newPath = skipGen ? srcPath.makeRelativeTo(basePath) / filePath : filePath;

	Vector<Path> dummyDirMetas;

	bool dbChanged = false;
	Vector<std::pair<Path, Path>> additionalFilesToImport;
	dbChanged = doImportFile(db, assets, isCodegen, skipGen, useDirMetas ? directoryMetas : dummyDirMetas, basePath, newPath, &additionalFilesToImport) || dbChanged;
	for (const auto& additional: additionalFilesToImport) {
		dbChanged = doImportFile(db, assets, isCodegen, skipGen, useDirMetas ? directoryMetas : dummyDirMetas, additional.first, additional.second, nullptr) || dbChanged;
	}
	return dbChanged;
}

bool CheckAssetsTask::doImportFile(ImportAssetsDatabase& db, AssetTable& assets, bool isCodegen, bool skipGen, const Vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath, Vector<std::pair<Path, Path>>* additionalFilesToImport) {
	std::array<int64_t, 3> timestamps = {{ 0, 0, 0 }};
	bool dbChanged = false;

	// Collect data on main file
	timestamps[0] = fileSystemCache.getLastWriteTime(srcPath / filePath);

	// Collect data on directory meta file
	auto dirMetaPath = findDirectoryMeta(directoryMetas, filePath);
	if (dirMetaPath && fileSystemCache.exists(srcPath / dirMetaPath.value())) {
		dirMetaPath = srcPath / dirMetaPath.value();
		timestamps[1] = fileSystemCache.getLastWriteTime(dirMetaPath.value());
	} else {
		dirMetaPath = {};
	}

	// Collect data on private meta file
	std::optional<Path> privateMetaPath = srcPath / filePath.replaceExtension(filePath.getExtension() + ".meta");
	if (fileSystemCache.exists(privateMetaPath.value())) {
		timestamps[2] = fileSystemCache.getLastWriteTime(privateMetaPath.value());
	} else {
		privateMetaPath = {};
	}

	// Load metadata if needed
	if (db.needToLoadInputMetadata(filePath, timestamps)) {
		Metadata meta = MetadataImporter::getMetaData(filePath, dirMetaPath, privateMetaPath);
		if (skipGen) {
			meta.set("skipGen", true);
		}
		db.setInputFileMetadata(filePath, timestamps, meta, srcPath);
		dbChanged = true;
	} else {
		db.markInputPresent(filePath);
	}

	// If this file was already imported, check any previous dependencies it had too
	if (additionalFilesToImport) {
		for (const auto& [inputSrc, inputFile]: db.getFilesForAssetsThatHasAdditionalFile(srcPath / filePath)) {
			if (fileSystemCache.exists(inputSrc / inputFile)) {
				additionalFilesToImport->emplace_back(inputSrc, inputFile);
			}
		}
	}

	// Figure out the right importer and assetId for this file
	auto& assetImporter = isCodegen ? projectAssetImporter->getImporters(ImportAssetType::Codegen).at(0).get() : projectAssetImporter->getRootImporter(filePath);
	if (assetImporter.getType() == ImportAssetType::Skip) {
		return false;
	}
	String assetId = assetImporter.getAssetId(filePath, db.getMetadata(filePath));
	const auto assetKey = std::pair{ assetImporter.getType(), assetId };

	// Build timestamped path
	auto input = TimestampedPath(filePath, std::max(timestamps[0], std::max(timestamps[1], timestamps[2])));

	// Build the asset
	auto iter = assets.find(assetKey);
	if (iter == assets.end()) {
		// New; create it
		auto& asset = assets[assetKey];
		asset.assetId = assetId;
		asset.assetType = assetImporter.getType();
		asset.srcDir = srcPath;
		asset.inputFiles.push_back(input);

		// Check all other input files for this asset
		if (!isCodegen && additionalFilesToImport) {
			const auto [addSrcPath, addSrcFiles] = db.getInputFiles(asset.assetType, asset.assetId);
			for (const auto& additional: addSrcFiles) {
				if (additional != filePath) {
					if (fileSystemCache.exists(addSrcPath / additional)) {
						additionalFilesToImport->emplace_back(addSrcPath, additional);
					} else {
						Logger::logInfo("File deleted: " + (addSrcPath / additional));
					}
				}
			}
		}
	} else {
		// Already exists
		auto& asset = iter->second;
		if (asset.assetType != assetImporter.getType()) { // Ensure it has the correct type
			throw Exception("AssetId conflict on " + assetId, HalleyExceptions::Tools);
		}
		if (asset.srcDir == srcPath) {
			asset.addInputFile(input);
		} else {
			auto relPath = (srcPath / input.first).makeRelativeTo(asset.srcDir);
			asset.addInputFile(input, relPath);

			// Don't mix files from two different source paths
			//throw Exception("Mixed source dir input for " + assetId, HalleyExceptions::Tools);
		}
	}

	return dbChanged;
}

void CheckAssetsTask::sleep(int timeMs)
{
	std::unique_lock<std::mutex> lock(mutex);
	condition.wait_for(lock, timeMs * 1ms);
}

CheckAssetsTask::AssetTable CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Range<float> progressRange)
{
	AssetTable assets;

	bool dbChanged = false;

	if (collectDirMeta) {
		directoryMetas.clear();
	}

	db.markAllInputFilesAsMissing();
	db.updateAdditionalFileCache();

	// Enumerate all potential assets
	int i = 0;
	for (const auto& srcPath: srcPaths) {
		const auto rangeSize = progressRange.getLength() / static_cast<float>(srcPaths.size());
		const auto curRange = Range<float>(i * rangeSize + progressRange.start, (i + 1) * rangeSize + progressRange.start);

		setProgress(curRange.start, "Enumerating " + srcPath.getNativeString());

		auto allFiles = fileSystemCache.enumerateDirectory(srcPath);

		// First, collect all directory metas
		if (collectDirMeta) {
			for (auto& filePath : allFiles) {
				if (filePath.getFilename() == "_dir.meta") {
					directoryMetas.push_back(filePath);
				}
			}
		}

		// Next, go through normal files
		Path curPath;
		size_t j = 0;
		for (const auto& filePath: allFiles) {
			auto parentPath = filePath.parentPath();
			if (parentPath != curPath) {
				curPath = parentPath;
				const float prog = lerp(curRange.start, curRange.end, j / static_cast<float>(allFiles.size()));
				setProgress(prog, "Checking " + curPath.getNativeString(false));
			}

			if (isCancelled()) {
				return {};
			}

			dbChanged = importFile(db, assets, collectDirMeta, srcPath, srcPaths, filePath) || dbChanged;
			j++;
		}

		i++;
	}

	dbChanged = db.purgeMissingInputs() || dbChanged;
	
	if (dbChanged) {
		db.save();
	}
	db.markAssetsAsStillPresent(assets);

	return assets;
}

bool CheckAssetsTask::requestImport(ImportAssetsDatabase& db, AssetTable assets, Path dstPath, String taskName, bool packAfter)
{
	// Check for missing input files
	auto toDelete = db.getAllMissing();
	Vector<String> deletedAssets;
	if (!toDelete.empty()) {
		for (auto& a: toDelete) {
			for (auto& out: a.outputFiles) {
				deletedAssets.push_back(toString(out.type) + ":" + out.name);
			}
		}

		addPendingTask(std::make_unique<DeleteAssetsTask>(db, dstPath, std::move(toDelete)));
	}

	// Import assets
	const bool hasImport = hasAssetsToImport(db, assets);
	if (hasImport || !deletedAssets.empty()) {
		auto toImport = hasImport ? getAssetsToImport(db, assets) : Vector<ImportAssetsDatabaseEntry>();
		addPendingTask(std::make_unique<ImportAssetsTask>(taskName, db, projectAssetImporter, dstPath, std::move(toImport), std::move(deletedAssets), project, packAfter));
		return true;
	}
	return false;
}

void CheckAssetsTask::requestRefreshAssets(gsl::span<const Path> paths)
{
	condition.notify_one();
}

void CheckAssetsTask::requestReimport(ReimportType type)
{
	std::unique_lock<std::mutex> lock(mutex);
	pendingReimport = type;
}

bool CheckAssetsTask::hasAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets)
{
	for (const auto& a: assets) {
		if (db.needsImporting(a.second, fileSystemCache, false)) {
			return true;
		}
	}
	return false;
}

Vector<ImportAssetsDatabaseEntry> CheckAssetsTask::getAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets)
{
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (const auto& a: assets) {
		if (db.needsImporting(a.second, fileSystemCache, true)) {
			toImport.push_back(a.second);
		}
	}

	return toImport;
}

std::optional<Path> CheckAssetsTask::findDirectoryMeta(const Vector<Path>& metas, const Path& path) const
{
	auto parent = path.parentPath();
	std::optional<Path> longestPath;
	for (auto& m: metas) {
		if (!longestPath || longestPath->getNumberPaths() < m.getNumberPaths()) {
			const auto n = m.getNumberPaths() - 1;
			if (m.getParts().subspan(0, n) == parent.getParts().subspan(0, std::min(n, parent.getNumberPaths()))) {
				longestPath = m;
			}
		}
	}
	return longestPath;
}
