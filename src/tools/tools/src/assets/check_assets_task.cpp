#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include <boost/filesystem/operations.hpp>
#include "halley/tools/file/filesystem.h"
#include "halley/support/logger.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool oneShot)
	: EditorTask("Check assets", true, false)
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

		decltype(pending) curPending;
		{
			std::unique_lock<std::mutex> lock(mutex);
			curPending = std::move(pending);
		}
		if (!curPending.empty()) {
			const auto assets = checkSpecificAssets(project.getImportAssetsDatabase(), curPending);
			if (!isCancelled()) {
				importing |= requestImport(project.getImportAssetsDatabase(), assets, project.getUnpackedAssetsPath(), "Importing assets", true);
				sleep(10);
			}
		}

		if (first | monitorAssets.poll() | monitorAssetsSrc.poll() | monitorSharedAssetsSrc.poll()) { // Don't short-circuit
			Logger::logInfo("Scanning for asset changes...");
			const auto assets = checkAllAssets(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, true);
			if (!isCancelled()) {
				importing |= requestImport(project.getImportAssetsDatabase(), assets, project.getUnpackedAssetsPath(), "Importing assets", true);
			}
		}
		
		const bool sharedGenSrcResult = first | monitorSharedGenSrc.poll();
		if (sharedGenSrcResult | monitorGen.poll() | monitorGenSrc.poll()) {
			Logger::logInfo("Scanning for codegen changes...");
			const auto assets = checkAllAssets(project.getCodegenDatabase(), { project.getSharedGenSrcPath(), project.getGenSrcPath() }, false);
			if (!isCancelled()) {
				importing |= requestImport(project.getCodegenDatabase(), assets, project.getGenPath(), "Generating code", false);
			}
		}

		if (sharedGenSrcResult | monitorSharedGen.poll()) {
			Logger::logInfo("Scanning for Halley codegen changes...");
			const auto assets = checkAllAssets(project.getSharedCodegenDatabase(), { project.getSharedGenSrcPath() }, false);
			if (!isCancelled()) {
				importing |= requestImport(project.getSharedCodegenDatabase(), assets, project.getSharedGenPath(), "Generating code", false);
			}
		}

		while (hasPendingTasks()) {
			sleep(5);
		}

		if (importing || first) {
			Concurrent::execute(Executors::getMainThread(), [project = &project] () {
				project->onAllAssetsImported();
			});
		}
		first = false;
		
		if (oneShot) {
			return;
		}

		if (pending.empty()) {
			sleep(monitorAssets.hasRealImplementation() ? 100 : 1000);
		}
	}
}

bool CheckAssetsTask::importFile(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry>& assets, bool isCodegen, bool skipGen, const std::vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath) {
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

	// Figure out the right importer and assetId for this file
	auto& assetImporter = isCodegen ? project.getAssetImporter().getImporters(ImportAssetType::Codegen).at(0).get() : project.getAssetImporter().getRootImporter(filePath);
	if (assetImporter.getType() == ImportAssetType::Skip) {
		return false;
	}
	String assetId = assetImporter.getAssetId(filePath, db.getMetadata(filePath));

	// Build timestamped path
	auto input = TimestampedPath(filePath, std::max(timestamps[0], std::max(timestamps[1], timestamps[2])));

	// Build the asset
	auto iter = assets.find(assetId);
	if (iter == assets.end()) {
		// New; create it
		auto& asset = assets[assetId];
		asset.assetId = assetId;
		asset.assetType = assetImporter.getType();
		asset.srcDir = srcPath;
		asset.inputFiles.push_back(input);
	} else {
		// Already exists
		auto& asset = iter->second;
		if (asset.assetType != assetImporter.getType()) { // Ensure it has the correct type
			throw Exception("AssetId conflict on " + assetId, HalleyExceptions::Tools);
		}
		if (asset.srcDir == srcPath) {
			asset.inputFiles.push_back(input);
		} else {
			auto relPath = (srcPath / input.first).makeRelativeTo(asset.srcDir);
			asset.inputFiles.emplace_back(input, relPath);

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

std::map<String, ImportAssetsDatabaseEntry> CheckAssetsTask::checkSpecificAssets(ImportAssetsDatabase& db, const std::vector<Path>& paths)
{
	std::map<String, ImportAssetsDatabaseEntry> assets;
	bool dbChanged = false;
	for (auto& path: paths) {
		dbChanged = dbChanged | importFile(db, assets, false, false, directoryMetas, project.getAssetsSrcPath(), path);
	}
	if (dbChanged) {
		db.save();
	}
	return assets;
}

std::map<String, ImportAssetsDatabaseEntry> CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, bool collectDirMeta)
{
	std::map<String, ImportAssetsDatabaseEntry> assets;

	bool dbChanged = false;

	if (collectDirMeta) {
		directoryMetas.clear();
	}
	std::vector<Path> dummyDirMetas;

	db.markAllInputFilesAsMissing();

	// Enumerate all potential assets
	for (const auto& srcPath: srcPaths) {
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
		const bool isCodegen = srcPath == project.getGenSrcPath() || srcPath == project.getSharedGenSrcPath();
		const bool skipGen = srcPath == project.getSharedGenSrcPath() && srcPaths.size() > 1;
		for (const auto& filePath : allFiles) {
			if (filePath.getExtension() == ".meta") {
				continue;
			}
			if (isCancelled()) {
				return {};
			}

			if (skipGen) {
				const auto basePath = project.getGenSrcPath();
				const auto newPath = srcPath.makeRelativeTo(basePath) / filePath;
				dbChanged = dbChanged | importFile(db, assets, isCodegen, skipGen, collectDirMeta ? directoryMetas : dummyDirMetas, basePath, newPath);
			} else {
				dbChanged = dbChanged | importFile(db, assets, isCodegen, skipGen, collectDirMeta ? directoryMetas : dummyDirMetas, srcPath, filePath);
			}
		}
	}

	dbChanged = dbChanged | db.purgeMissingInputs();
	
	if (dbChanged) {
		db.save();
	}
	db.markAssetsAsStillPresent(assets);

	return assets;
}

bool CheckAssetsTask::requestImport(ImportAssetsDatabase& db, std::map<String, ImportAssetsDatabaseEntry> assets, Path dstPath, String taskName, bool packAfter)
{
	// Check for missing input files
	auto toDelete = db.getAllMissing();
	std::vector<String> deletedAssets;
	if (!toDelete.empty()) {
		for (auto& a: toDelete) {
			for (auto& out: a.outputFiles) {
				deletedAssets.push_back(toString(out.type) + ":" + out.name);
			}
		}

		Logger::logInfo("Assets to be deleted: " + toString(toDelete.size()));
		addPendingTask(EditorTaskAnchor(std::make_unique<DeleteAssetsTask>(db, dstPath, std::move(toDelete))));
	}

	// Import assets
	auto toImport = filterNeedsImporting(db, assets);
	if (!toImport.empty() || !deletedAssets.empty()) {
		Logger::logInfo("Assets to be imported: " + toString(toImport.size()));
		addPendingTask(EditorTaskAnchor(std::make_unique<ImportAssetsTask>(taskName, db, project.getAssetImporter(), dstPath, std::move(toImport), std::move(deletedAssets), project, packAfter)));
		return true;
	}
	return false;
}

void CheckAssetsTask::requestRefreshAsset(Path path)
{
	{
		std::unique_lock<std::mutex> lock(mutex);
		inbox.push_back(std::move(path));
	}
	condition.notify_one();
}

std::vector<ImportAssetsDatabaseEntry> CheckAssetsTask::filterNeedsImporting(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets)
{
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (auto& a: assets) {
		if (db.needsImporting(a.second)) {
			toImport.push_back(a.second);
		}
	}

	return toImport;
}

std::optional<Path> CheckAssetsTask::findDirectoryMeta(const std::vector<Path>& metas, const Path& path) const
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
