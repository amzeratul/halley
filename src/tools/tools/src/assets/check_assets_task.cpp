#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include "halley/tools/file/filesystem.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/concurrency/concurrent.h"
#include "halley/support/logger.h"

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool oneShot)
	: Task("Checking assets", true, true)
	, project(project)
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

		projectAssetImporter = project.getAssetImporter();

		decltype(pending) curPending;
		decltype(pendingReimport) curPendingReimport;
		{
			std::unique_lock<std::mutex> lock(mutex);
			curPending = std::move(pending);
			curPendingReimport = pendingReimport;
			pendingReimport = {};
		}
		if (!curPending.empty()) {
			const auto assets = checkSpecificAssets(project.getImportAssetsDatabase(), curPending);
			if (!isCancelled()) {
				importing |= requestImport(project.getImportAssetsDatabase(), assets, project.getUnpackedAssetsPath(), "Importing assets", true);
				sleep(10);
			}
		}

		// Wait for the import to finish, otherwise the DB won't be updated and it'll try updating the same assets twice
		while (hasPendingTasks()) {
			sleep(5);
		}

		// First import
		if (first) {
			importing |= importAll(project.getCodegenDatabase(), { project.getSharedGenSrcPath(), project.getGenSrcPath() }, false, project.getGenPath(), "Generating code", false, Range<float>(0, 0.05f));
			importing |= importAll(project.getSharedCodegenDatabase(), { project.getSharedGenSrcPath() }, false, project.getSharedGenPath(), "Generating code", false, Range<float>(0.05f, 0.1f));
			importing |= importAll(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, true, project.getUnpackedAssetsPath(), "Importing assets", true, Range<float>(0.1f, 1.0f));
			setVisible(false);
			while (hasPendingTasks()) {
				sleep(5);
			}
		}

		// Reimport
		if (curPendingReimport) {
			setVisible(true);
			const bool hasCodeGen = curPendingReimport == ReimportType::Codegen;
			const bool hasAssets = curPendingReimport == ReimportType::ImportAll || curPendingReimport == ReimportType::ReimportAll;

			if (hasCodeGen) {
				project.getCodegenDatabase().clear();
				project.getSharedCodegenDatabase().clear();
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

		// Check if any files changed
		Vector<DirectoryMonitor::Event> assetsChanged;
		monitorAssetsSrc.poll(assetsChanged, true);
		monitorSharedAssetsSrc.poll(assetsChanged, true);
		monitorAssets.poll(assetsChanged, true);
		Vector<DirectoryMonitor::Event> sharedGenChanged;
		monitorSharedGenSrc.poll(sharedGenChanged, true);
		monitorSharedGen.poll(sharedGenChanged, true);
		Vector<DirectoryMonitor::Event> genChanged = sharedGenChanged; // Copy
		monitorGenSrc.poll(genChanged, true);
		monitorGen.poll(sharedGenChanged, true);

		// Re-import any changes
		importing |= importChanged(std::move(assetsChanged), project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, true, false, project.getUnpackedAssetsPath(), "Importing assets", true);
		importing |= importChanged(std::move(genChanged), project.getCodegenDatabase(), { project.getSharedGenSrcPath(), project.getGenSrcPath() }, false, true, project.getGenPath(), "Generating code", false);
		importing |= importChanged(std::move(sharedGenChanged), project.getSharedCodegenDatabase(), { project.getSharedGenSrcPath() }, false, true, project.getSharedGenPath(), "Generating code", false);

		setVisible(false);
		while (hasPendingTasks()) {
			sleep(5);
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

		if (pending.empty()) {
			sleep(monitorAssets.hasRealImplementation() ? 20 : 1000);
		}
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

bool CheckAssetsTask::importChanged(Vector<DirectoryMonitor::Event> changes, ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, bool isCodeGen, Path dstPath, String taskName, bool packAfter)
{
	if (changes.empty()) {
		return false;
	}

	// Check for full reimport
	bool reimportAll = isCodeGen;
	if (!reimportAll) {
		for (const auto& change : changes) {
			if (change.type == DirectoryMonitor::ChangeType::Unknown || change.name == "_dir.meta") {
				reimportAll = true;
				break;
			}
		}
	}
	
	// If we have a wildcard change, reimport all
	if (reimportAll) {
		return importAll(db, srcPaths, collectDirMeta, std::move(dstPath), std::move(taskName), packAfter, Range<float>(0, 0));
	}

	// Otherwise we'll only import the ones that changed
	if (isCancelled()) {
		return false;
	}

	postProcessChanges(changes);
	filterDuplicateChanges(changes);
	addFailedFiles(db, changes);
	const auto assets = checkChangedAssets(db, changes, srcPaths, dstPath, collectDirMeta);
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
	timestamps[0] = FileSystem::getLastWriteTime(srcPath / filePath);

	// Collect data on directory meta file
	auto dirMetaPath = findDirectoryMeta(directoryMetas, filePath);
	if (dirMetaPath && FileSystem::exists(srcPath / dirMetaPath.value())) {
		dirMetaPath = srcPath / dirMetaPath.value();
		timestamps[1] = FileSystem::getLastWriteTime(dirMetaPath.value());
	} else {
		dirMetaPath = {};
	}

	// Collect data on private meta file
	std::optional<Path> privateMetaPath = srcPath / filePath.replaceExtension(filePath.getExtension() + ".meta");
	if (FileSystem::exists(privateMetaPath.value())) {
		timestamps[2] = FileSystem::getLastWriteTime(privateMetaPath.value());
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
			if (Path::exists(inputSrc / inputFile)) {
				additionalFilesToImport->emplace_back(inputSrc, inputFile);
			} else {
				Logger::logWarning("Missing original input file: " + (inputSrc / inputFile));
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
					if (Path::exists(addSrcPath / additional)) {
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
	for (auto& a: inbox) {
		pending.push_back(std::move(a)); // This is buggy, just wake up (causes assets to import twice)
	}
	inbox.clear();
}

CheckAssetsTask::AssetTable CheckAssetsTask::checkSpecificAssets(ImportAssetsDatabase& db, const Vector<Path>& paths)
{
	AssetTable assets;
	bool dbChanged = false;
	for (auto& path: paths) {
		dbChanged = importFile(db, assets, true, project.getAssetsSrcPath(), { project.getAssetsSrcPath() }, path) || dbChanged;
	}
	if (dbChanged) {
		db.save();
	}
	return assets;
}

std::pair<Path, Path> CheckAssetsTask::findRelativePath(Path path, const Vector<Path>& srcPaths) const
{
	const Path* srcPathPtr = nullptr;
	for (auto& srcPath: srcPaths) {
		if (srcPath.isPrefixOf(path)) {
			srcPathPtr = &srcPath;
			break;
		}
	}
	if (!srcPathPtr) {
		//Logger::logWarning("Ignored file change: " + path.toString());
		return {{}, {}};
	}
	return { *srcPathPtr, path.makeRelativeTo(*srcPathPtr) };
}

CheckAssetsTask::AssetTable CheckAssetsTask::checkChangedAssets(ImportAssetsDatabase& db, const Vector<DirectoryMonitor::Event>& changes, const Vector<Path>& srcPaths, const Path& dstPath, bool useDirMeta)
{
	AssetTable assets;

	bool dbChanged = false;

	for (const auto& change: changes) {
		if (isCancelled()) {
			return {};
		}

		auto [srcPath, filePath] = findRelativePath(change.name, srcPaths);
		if (srcPath.isEmpty()) {
			continue;
		}

		if (change.type == DirectoryMonitor::ChangeType::FileAdded || change.type == DirectoryMonitor::ChangeType::FileModified || change.type == DirectoryMonitor::ChangeType::FileRenamed) {
			dbChanged = importFile(db, assets, useDirMeta, srcPath, srcPaths, filePath) || dbChanged;

			if (change.type == DirectoryMonitor::ChangeType::FileRenamed) {
				Path oldFilePath = change.oldName.isEmpty() ? Path() : Path(change.oldName).makeRelativeTo(srcPath);
				db.markInputMissing(oldFilePath);
			}
		} else if (change.type == DirectoryMonitor::ChangeType::FileRemoved) {
			db.markInputMissing(filePath);
		}
	}

	const auto toImport = db.markMissingAssetsAndGetPartial();
	for (const auto& file: toImport) {
		auto [srcPath, filePath] = findRelativePath(file, srcPaths);
		if (srcPath.isEmpty()) {
			continue;
		}
		dbChanged = importFile(db, assets, useDirMeta, srcPath, srcPaths, filePath) || dbChanged;
	}

	dbChanged = db.purgeMissingInputs() || dbChanged;
	
	if (dbChanged) {
		db.save();
	}

	return assets;
}

CheckAssetsTask::AssetTable CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, const Vector<Path>& srcPaths, bool collectDirMeta, Range<float> progressRange)
{
	AssetTable assets;

	bool dbChanged = false;

	if (collectDirMeta) {
		directoryMetas.clear();
	}

	db.markAllInputFilesAsMissing();

	// Enumerate all potential assets
	int i = 0;
	for (const auto& srcPath: srcPaths) {
		const auto rangeSize = progressRange.getLength() / static_cast<float>(srcPaths.size());
		const auto curRange = Range<float>(i * rangeSize + progressRange.start, (i + 1) * rangeSize + progressRange.start);

		setProgress(curRange.start, "Enumerating " + srcPath.getNativeString());

		auto allFiles = FileSystem::enumerateDirectory(srcPath);

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

void CheckAssetsTask::filterDuplicateChanges(Vector<DirectoryMonitor::Event>& changes) const
{
	Vector<DirectoryMonitor::Event> result;
	HashSet<DirectoryMonitor::Event> index;
	HashSet<String> deleteIndex;

	// Process delete changes first, since they override others
	for (auto& change: changes) {
		if (change.type == DirectoryMonitor::ChangeType::FileRemoved) {
			index.insert(change);
			deleteIndex.insert(change.name);
			result.push_back(change);
		}
	}

	for (auto& change: changes) {
		if (change.type != DirectoryMonitor::ChangeType::FileRemoved) {
			if (!index.contains(change) && !deleteIndex.contains(change.name)) {
				index.emplace(change);
				result.push_back(change);
			}
		}
	}

	changes = std::move(result);
}

void CheckAssetsTask::postProcessChanges(Vector<DirectoryMonitor::Event>& changes) const
{
	// Treat changes to a meta file as a change to the original file
	for (auto& c: changes) {
		if (c.name.endsWith(".meta")) {
			c.name = c.name.left(c.name.length() - 5);
			c.type = DirectoryMonitor::ChangeType::FileModified;
		}
		if (c.oldName.endsWith(".meta")) {
			c.oldName = c.oldName.left(c.oldName.length() - 5);
			c.type = DirectoryMonitor::ChangeType::FileModified;
		}
	}
}

void CheckAssetsTask::addFailedFiles(ImportAssetsDatabase& db, Vector<DirectoryMonitor::Event>& changes) const
{
	for (const auto& failed: db.getAllFailedFilenames()) {
		changes.push_back(DirectoryMonitor::Event{ DirectoryMonitor::ChangeType::FileModified, failed.getString(), "" });
	}
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
	if (true) {
		std::unique_lock<std::mutex> lock(mutex);
		inbox.insert(inbox.end(), paths.begin(), paths.end());
	}
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
		if (db.needsImporting(a.second, false)) {
			return true;
		}
	}
	return false;
}

Vector<ImportAssetsDatabaseEntry> CheckAssetsTask::getAssetsToImport(ImportAssetsDatabase& db, const AssetTable& assets)
{
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (const auto& a: assets) {
		if (db.needsImporting(a.second, true)) {
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
			auto n = m.getNumberPaths() - 1;
			if (m.getFront(n) == parent.getFront(n)) {
				longestPath = m;
			}
		}
	}
	return longestPath;
}
