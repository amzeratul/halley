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

	// Enumerate all potential assets
	for (auto srcPath : srcPaths) {
		for (auto filePath : FileSystem::enumerateDirectory(srcPath)) {
			auto& assetImporter = codegen ? project.getAssetImporter().getImporter(AssetType::Codegen) : project.getAssetImporter().getImporter(filePath);
			String assetId = assetImporter.getAssetId(filePath);

			auto input = ImportAssetsDatabaseEntry::InputFile(filePath, FileSystem::getLastWriteTime(srcPath / filePath));

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
					throw Exception("AssetId conflict on " + assetId);
				}
				if (asset.srcDir == srcPath) { // Don't mix files from two different source paths
					asset.inputFiles.push_back(input);
				}
			}
		}
	}

	// Check for missing input files
	db.markAllInputsAsMissing();
	for (auto& a : assets) {
		db.markInputAsPresent(a.second);
	}
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
