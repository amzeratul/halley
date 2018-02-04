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

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool headless)
	: EditorTask("Check assets", true, false)
	, project(project)
	, monitorAssets(project.getAssetsPath())
	, monitorAssetsSrc(project.getAssetsSrcPath())
	, monitorSharedAssetsSrc(project.getSharedAssetsSrcPath())
	, monitorGen(project.getGenPath())
	, monitorGenSrc(project.getGenSrcPath())
	, headless(headless)
{}

void CheckAssetsTask::run()
{
	bool first = true;
	while (!isCancelled()) {
		if (first | monitorAssets.poll() | monitorAssetsSrc.poll() | monitorSharedAssetsSrc.poll()) { // Don't short-circuit
			checkAllAssets(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, project.getAssetsPath(), "Importing assets");
		}

		if (first | monitorGen.poll() | monitorGenSrc.poll()) {
			checkAllAssets(project.getCodegenDatabase(), { project.getGenSrcPath() }, project.getGenPath(), "Generating code");
		}

		first = false;

		while (hasPendingTasks()) {
			std::this_thread::sleep_for(5ms);
		}

		if (headless) {
			return;
		}
		std::this_thread::sleep_for(monitorAssets.hasRealImplementation() ? 25ms : 100ms);
	}
}

void CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, Path dstPath, String taskName)
{
	std::map<String, ImportAssetsDatabaseEntry> assets;

	bool codegen = srcPaths.size() == 1 && srcPaths[0] == project.getGenSrcPath();

	std::vector<Path> directoryMetas;

	// Enumerate all potential assets
	for (auto srcPath : srcPaths) {
		auto allFiles = FileSystem::enumerateDirectory(srcPath);

		// First, collect all directory metas
		for (auto& filePath : allFiles) {
			if (filePath.getFilename() == "_dir.meta") {
				directoryMetas.push_back(filePath);
			}
		}

		// Next, go through normal assets
		for (auto& filePath : allFiles) {
			if (filePath.getFilename() == "_dir.meta") {
				continue;
			}

			auto& assetImporter = codegen ? project.getAssetImporter().getImporter(ImportAssetType::Codegen) : project.getAssetImporter().getImporter(filePath);
			String assetId = assetImporter.getAssetId(filePath);
			
			auto input = TimestampedPath(filePath, FileSystem::getLastWriteTime(srcPath / filePath));

			auto iter = assets.find(assetId);
			if (iter == assets.end()) {
				// New; create it
				auto& asset = assets[assetId];
				asset.assetId = assetId;
				asset.assetType = assetImporter.getType();
				asset.srcDir = srcPath;
				asset.inputFiles.push_back(input);

				// Check if there's a directory metafile to add
				auto dirMetaPath = findDirectoryMeta(directoryMetas, filePath);
				if (dirMetaPath && FileSystem::exists(srcPath / dirMetaPath.get())) {
					asset.inputFiles.push_back(TimestampedPath(dirMetaPath.get(), FileSystem::getLastWriteTime(srcPath / dirMetaPath.get())));
				}
			} else {
				// Already exists
				auto& asset = iter->second;
				if (asset.assetType != assetImporter.getType()) { // Ensure it has the correct type
					throw Exception("AssetId conflict on " + assetId);
				}
				if (asset.srcDir == srcPath) { // Don't mix files from two different source paths
					asset.inputFiles.push_back(input);
				} else {
					throw Exception("Mixed source dir input for " + assetId);
				}
			}
		}
	}

	// Check for missing input files
	db.markAssetsAsStillPresent(assets);
	auto missing = db.getAllMissing();
	if (!missing.empty()) {
		addPendingTask(EditorTaskAnchor(std::make_unique<DeleteAssetsTask>(db, dstPath, std::move(missing))));
	}

	// Import assets
	auto toImport = filterNeedsImporting(db, assets);
	if (!toImport.empty()) {
		addPendingTask(EditorTaskAnchor(std::make_unique<ImportAssetsTask>(taskName, db, project.getAssetImporter(), dstPath, std::move(toImport))));
	}
}

std::vector<ImportAssetsDatabaseEntry> CheckAssetsTask::filterNeedsImporting(ImportAssetsDatabase& db, const std::map<String, ImportAssetsDatabaseEntry>& assets)
{
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (auto &a : assets) {
		if (db.needsImporting(a.second)) {
			toImport.push_back(a.second);
		}
	}

	return toImport;
}

Maybe<Path> CheckAssetsTask::findDirectoryMeta(const std::vector<Path>& metas, const Path& path) const
{
	auto parent = path.parentPath();
	for (auto& m: metas) {
		auto n = m.getNumberPaths() - 1;
		if (m.getFront(n) == parent.getFront(n)) {
			return m;
		}
	}
	return {};
}
