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
			fileSystemCache.trackDirectory(project.getEditorAssetsSrcPath());
		}

		projectAssetImporter = project.getAssetImporter();

		decltype(pendingReimport) curPendingReimport;
		{
			UniqueLock lock(mutex);
			curPendingReimport = pendingReimport;
			pendingReimport = {};
		}

		// Wait for the import to finish, otherwise the DB won't be updated and it'll try updating the
		// same assets twice. Also wait if project save notifications are disabled.
		while (hasPendingTasks() || !project.isAssetSaveNotificationEnabled()) {
			sleep(5);
		}

		// Check if any files changed
		DirectoryMonitor::DelayRules delayRules;
		delayRules.baseDelay = 50;
		delayRules.rules += DirectoryMonitor::DelayRule{ ".ase", 500 };
		delayRules.rules += DirectoryMonitor::DelayRule{ ".aseprite", 500 };

		Vector<DirectoryMonitor::Event> assetsSrcChanged;
		Vector<DirectoryMonitor::Event> genSrcChanged;
		Vector<DirectoryMonitor::Event> assetsChanged;
		Vector<DirectoryMonitor::Event> genChanged;
		monitorAssetsSrc.poll(assetsSrcChanged, delayRules);
		monitorSharedAssetsSrc.poll(assetsSrcChanged, delayRules);
		monitorAssets.poll(assetsChanged, delayRules);
		monitorSharedGenSrc.poll(genSrcChanged, delayRules);
		monitorGenSrc.poll(genSrcChanged, delayRules);
		monitorSharedGen.poll(genChanged, delayRules);
		monitorGen.poll(genChanged, delayRules);
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
				//Logger::logDev("Notifying assets imported");
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
	const bool importing = requestImport(db, assets, std::move(dstPath), std::move(taskName), packAfter);

	// If an import task was queued it saves the db when it finishes; otherwise persist any changes from the check now.
	// save() only writes if the db actually changed.
	if (!importing) {
		db.save();
	}
	return importing;
}

bool CheckAssetsTask::importFile(ImportAssetsDatabase& db, AssetTable& assets, bool useDirMetas, const DirMetaInfo& dirMeta, const FileTimes& times, const Path& srcPath, const Vector<Path>& srcPaths, const Path& filePath)
{
	if (filePath.getExtensionStrView() == ".meta") {
		return false;
	}

	const bool isCodegen = srcPath == project.getGenSrcPath() || srcPath == project.getSharedGenSrcPath();
	const bool skipGen = srcPath == project.getSharedGenSrcPath() && srcPaths.size() > 1;

	const auto& basePath = skipGen ? project.getGenSrcPath() : srcPath;
	const auto& newPath = skipGen ? srcPath.makeRelativeTo(basePath) / filePath : filePath;

	Vector<Path> dummyDirMetas;

	bool dbChanged = false;
	Vector<std::pair<Path, Path>> additionalFilesToImport;
	dbChanged = doImportFile(db, assets, isCodegen, skipGen, useDirMetas ? directoryMetas : dummyDirMetas, &dirMeta, &times, basePath, newPath, &additionalFilesToImport) || dbChanged;
	for (const auto& additional: additionalFilesToImport) {
		dbChanged = doImportFile(db, assets, isCodegen, skipGen, useDirMetas ? directoryMetas : dummyDirMetas, nullptr, nullptr, additional.first, additional.second, nullptr) || dbChanged;
	}
	return dbChanged;
}

bool CheckAssetsTask::doImportFile(ImportAssetsDatabase& db, AssetTable& assets, bool isCodegen, bool skipGen, const Vector<Path>& directoryMetas, const DirMetaInfo* dirMeta, const FileTimes* times, const Path& srcPath, const Path& filePath, Vector<std::pair<Path, Path>>* additionalFilesToImport) {
	std::array<int64_t, 3> timestamps = {{ 0, 0, 0 }};
	bool dbChanged = false;

	// Collect data on main file and private meta file
	std::optional<Path> privateMetaPath;
	bool hasPrivateMeta = false;
	if (times) {
		timestamps[0] = times->file;
		timestamps[2] = times->privateMeta.value_or(0);
		hasPrivateMeta = times->privateMeta.has_value();
	} else {
		timestamps[0] = fileSystemCache.getLastWriteTime(srcPath / filePath);
		auto metaPath = srcPath / filePath.replaceExtension(filePath.getExtension() + ".meta");
		if (auto t = fileSystemCache.tryGetLastWriteTime(metaPath)) {
			timestamps[2] = *t;
			privateMetaPath = std::move(metaPath);
			hasPrivateMeta = true;
		}
	}

	// Collect data on directory meta file
	DirMetaInfo localDirMeta;
	if (!dirMeta) {
		localDirMeta = resolveDirMeta(directoryMetas, srcPath, filePath.parentPath());
		dirMeta = &localDirMeta;
	}
	timestamps[1] = dirMeta->timestamp;

	// Load metadata if needed
	const auto pathKey = filePath.toString();
	const Metadata* metadata = db.markInputPresentIfUpToDate(pathKey, timestamps);
	if (!metadata) {
		if (hasPrivateMeta && !privateMetaPath) {
			privateMetaPath = srcPath / filePath.replaceExtension(filePath.getExtension() + ".meta");
		}
		Metadata meta = MetadataImporter::getMetaData(filePath, dirMeta->path, privateMetaPath);
		if (skipGen) {
			meta.set("skipGen", true);
		}
		metadata = &db.setInputFileMetadata(pathKey, timestamps, std::move(meta), srcPath);
		dbChanged = true;
	}

	// If this file was already imported, check any previous dependencies it had too
	if (additionalFilesToImport) {
		for (const auto& [inputSrc, inputFile]: db.getFilesForAssetsThatHasAdditionalFile(srcPath, filePath)) {
			if (fileSystemCache.exists(inputSrc / inputFile)) {
				additionalFilesToImport->emplace_back(inputSrc, inputFile);
			}
		}
	}

	// Figure out the right importer and assetId for this file
	auto& assetImporter = isCodegen ? projectAssetImporter->getImporter(ImportAssetType::Codegen) : projectAssetImporter->getRootImporter(filePath);
	if (assetImporter.getType() == ImportAssetType::Skip) {
		return false;
	}
	String assetId = assetImporter.getAssetId(filePath, metadata);
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
	UniqueLock lock(mutex);
	condition.waitFor(lock, timeMs * 1ms);
}

CheckAssetsTask::AssetTable CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Range<float> progressRange)
{
	Stopwatch sw;
	sw.start();
	AssetTable assets;

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

		const auto listings = fileSystemCache.enumerateDirectoryListings(srcPath);

		size_t nFiles = 0;
		for (const auto& listing: listings) {
			nFiles += listing.files.size();
		}

		// First, collect all directory metas
		if (collectDirMeta) {
			for (const auto& listing: listings) {
				for (const auto& [fileName, time]: listing.files) {
					if (fileName == "_dir.meta") {
						directoryMetas.push_back(listing.dir / fileName);
					}
				}
			}
		}

		// Next, go through normal files, one directory at a time
		HashMap<String, int64_t> metaFiles;
		size_t j = 0;
		for (const auto& listing: listings) {
			if (isCancelled()) {
				return {};
			}

			const float prog = lerp(curRange.start, curRange.end, j / static_cast<float>(std::max(nFiles, size_t(1))));
			setProgress(prog, "Checking " + listing.dir.getNativeString(false));

			DirMetaInfo dirMeta;
			if (collectDirMeta) {
				dirMeta = resolveDirMeta(directoryMetas, srcPath, listing.dir);
			}

			// Collect private meta files ("file.ext.meta") so they can be paired with their files below
			metaFiles.clear();
			for (const auto& [fileName, time]: listing.files) {
				if (fileName.endsWith(".meta")) {
					metaFiles[FileSystemCache::getCaseCorrectedPath(fileName)] = time;
				}
			}

			for (const auto& [fileName, time]: listing.files) {
				++j;
				if (fileName.endsWith(".meta")) {
					continue;
				}

				FileTimes times;
				times.file = time;
				if (!metaFiles.empty()) {
					const auto metaIter = metaFiles.find(FileSystemCache::getCaseCorrectedPath(fileName + ".meta"));
					if (metaIter != metaFiles.end()) {
						times.privateMeta = metaIter->second;
					}
				}

				importFile(db, assets, collectDirMeta, dirMeta, times, srcPath, srcPaths, listing.dir / fileName);
			}
		}

		i++;
	}

	db.purgeMissingInputs();
	db.markAssetsAsStillPresent(assets);

	sw.pause();
	//Logger::logDev("Check all assets took " + toString(sw.elapsedMilliseconds()) + " ms");
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
	auto toImport = getAssetsToImport(db, assets);
	if (!toImport.empty() || !deletedAssets.empty()) {
		addPendingTask(std::make_unique<ImportAssetsTask>(taskName, db, projectAssetImporter, dstPath, std::move(toImport), std::move(deletedAssets), project, packAfter));
		return true;
	}
	return false;
}

void CheckAssetsTask::requestRefreshAssets(gsl::span<const Path> paths)
{
	condition.notifyOne();
}

void CheckAssetsTask::requestReimport(ReimportType type)
{
	UniqueLock lock(mutex);
	pendingReimport = type;
}

Vector<ImportAssetsDatabaseEntry> CheckAssetsTask::getAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets)
{
	Stopwatch sw;
	sw.start();
	Vector<ImportAssetsDatabaseEntry> toImport;
	bool hasNonFailed = false;

	for (const auto& a: assets) {
		switch (db.checkNeedsImporting(a.second, fileSystemCache)) {
		case ImportAssetsDatabase::ImportAction::Import:
			hasNonFailed = true;
			toImport.push_back(a.second);
			break;
		case ImportAssetsDatabase::ImportAction::RetryFailed:
			toImport.push_back(a.second);
			break;
		case ImportAssetsDatabase::ImportAction::None:
			break;
		}
	}

	// Only retry failed assets if something else changed, otherwise they'd be retried on every check
	if (!hasNonFailed) {
		toImport.clear();
	}

	sw.pause();
	//Logger::logDev("getAssetsToImport took " + toString(sw.elapsedMilliseconds()) + " ms");

	return toImport;
}

const Path* CheckAssetsTask::findDirectoryMeta(const Vector<Path>& metas, const Path& parentDir) const
{
	const Path* longestPath = nullptr;
	for (const auto& m: metas) {
		if (!longestPath || longestPath->getNumberOfParts() < m.getNumberOfParts()) {
			const auto n = m.getNumberOfParts() - 1;
			if (m.getFrontStrView(n) == parentDir.getFrontStrView(std::min(n, parentDir.getNumberOfParts()))) {
				longestPath = &m;
			}
		}
	}
	return longestPath;
}

CheckAssetsTask::DirMetaInfo CheckAssetsTask::resolveDirMeta(const Vector<Path>& metas, const Path& srcPath, const Path& parentDir)
{
	DirMetaInfo result;
	if (const auto* dirMetaPath = findDirectoryMeta(metas, parentDir)) {
		auto absPath = srcPath / *dirMetaPath;
		if (auto t = fileSystemCache.tryGetLastWriteTime(absPath)) {
			result.timestamp = *t;
			result.path = std::move(absPath);
		}
	}
	return result;
}
