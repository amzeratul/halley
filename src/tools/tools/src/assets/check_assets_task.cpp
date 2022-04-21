#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include <boost/filesystem/operations.hpp>
#include "halley/tools/file/filesystem.h"
#include "halley/resources/resource_data.h"
#include "halley/tools/assets/metadata_importer.h"
#include "halley/concurrency/concurrent.h"
#include "halley/support/logger.h"

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool oneShot)
	: Task("Checking assets", true, false)
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

		// Wait for the import to finish, otherwise the DB won't be updated and it'll try updating the same assets twice
		while (hasPendingTasks()) {
			sleep(5);
		}

		if (first | monitorAssets.poll() | monitorAssetsSrc.poll() | monitorSharedAssetsSrc.poll()) { // Don't short-circuit
			logInfo("Scanning for asset changes...");
			const auto assets = checkAllAssets(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, true);
			if (!isCancelled()) {
				importing |= requestImport(project.getImportAssetsDatabase(), assets, project.getUnpackedAssetsPath(), "Importing assets", true);
			}
		}
		
		const bool sharedGenSrcResult = first | monitorSharedGenSrc.poll();
		if (sharedGenSrcResult | monitorGen.poll() | monitorGenSrc.poll()) {
			logInfo("Scanning for codegen changes...");
			const auto assets = checkAllAssets(project.getCodegenDatabase(), { project.getSharedGenSrcPath(), project.getGenSrcPath() }, false);
			if (!isCancelled()) {
				importing |= requestImport(project.getCodegenDatabase(), assets, project.getGenPath(), "Generating code", false);
			}
		}

		if (sharedGenSrcResult | monitorSharedGen.poll()) {
			logInfo("Scanning for Halley codegen changes...");
			const auto assets = checkAllAssets(project.getSharedCodegenDatabase(), { project.getSharedGenSrcPath() }, false);
			if (!isCancelled()) {
				importing |= requestImport(project.getSharedCodegenDatabase(), assets, project.getSharedGenPath(), "Generating code", false);
			}
		}

		while (hasPendingTasks()) {
			sleep(5);
		}

		if (importing || first) {
			Concurrent::execute(Executors::getMainUpdateThread(), [project = &project] () {
				Logger::logDev("Notifying assets imported");
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

bool CheckAssetsTask::importFile(ImportAssetsDatabase& db, AssetMap& assets, bool isCodegen, bool skipGen, const Vector<Path>& directoryMetas, const Path& srcPath, const Path& filePath) {
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
	auto& assetImporter = isCodegen ? projectAssetImporter->getImporters(ImportAssetType::Codegen).at(0).get() : projectAssetImporter->getRootImporter(filePath);
	if (assetImporter.getType() == ImportAssetType::Skip) {
		return false;
	}
	String assetId = assetImporter.getAssetId(filePath, db.getMetadata(filePath));

	// Build timestamped path
	auto input = TimestampedPath(filePath, std::max(timestamps[0], std::max(timestamps[1], timestamps[2])));

	// Build the asset
	const auto key = ImportAssetKey(assetImporter.getType(), assetId);
	auto iter = assets.find(key);
	if (iter == assets.end()) {
		// New; create it
		auto& asset = assets[key];
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

CheckAssetsTask::AssetMap CheckAssetsTask::checkSpecificAssets(ImportAssetsDatabase& db, const Vector<Path>& paths)
{
	AssetMap assets;
	bool dbChanged = false;
	for (auto& path: paths) {
		dbChanged = importFile(db, assets, false, false, directoryMetas, project.getAssetsSrcPath(), path) || dbChanged;
	}
	if (dbChanged) {
		db.save();
	}
	return assets;
}

CheckAssetsTask::AssetMap CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, Vector<Path> srcPaths, bool collectDirMeta)
{
	AssetMap assets;

	bool dbChanged = false;

	if (collectDirMeta) {
		directoryMetas.clear();
	}
	Vector<Path> dummyDirMetas;

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
				dbChanged = importFile(db, assets, isCodegen, skipGen, collectDirMeta ? directoryMetas : dummyDirMetas, basePath, newPath) || dbChanged;
			} else {
				dbChanged = importFile(db, assets, isCodegen, skipGen, collectDirMeta ? directoryMetas : dummyDirMetas, srcPath, filePath) || dbChanged;
			}
		}
	}

	dbChanged = db.purgeMissingInputs() || dbChanged;
	
	if (dbChanged) {
		db.save();
	}
	db.markAssetsAsStillPresent(assets);

	return assets;
}

bool CheckAssetsTask::requestImport(ImportAssetsDatabase& db, AssetMap assets, Path dstPath, String taskName, bool packAfter)
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

bool CheckAssetsTask::hasAssetsToImport(ImportAssetsDatabase& db, const AssetMap& assets)
{
	for (const auto& a: assets) {
		if (db.needsImporting(a.second, false)) {
			return true;
		}
	}
	return false;
}

Vector<ImportAssetsDatabaseEntry> CheckAssetsTask::getAssetsToImport(ImportAssetsDatabase& db, const AssetMap& assets)
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
