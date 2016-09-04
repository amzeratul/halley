#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include <boost/filesystem/operations.hpp>
#include "halley/tools/codegen/import_codegen_task.h"

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool headless)
	: EditorTask("Check assets", true, false)
	, project(project)
	, monitorAssets(project.getAssetsSrcPath())
	, monitorSharedAssets(project.getSharedAssetsSrcPath())
	, monitorGen(project.getGenSrcPath())
	, headless(headless)
{}

void CheckAssetsTask::run()
{
	bool first = true;
	while (!isCancelled()) {
		if (first | monitorAssets.poll() | monitorSharedAssets.poll()) { // Don't short-circuit
			checkAllAssets(project.getImportAssetsDatabase(), { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }, project.getAssetsPath(), &CheckAssetsTask::importAssets);
		}

		if (first | monitorGen.poll()) {
			checkAllAssets(project.getCodegenDatabase(), { project.getGenSrcPath() }, project.getGenPath(), &CheckAssetsTask::importCodegen);
		}

		first = false;

		do {
			std::this_thread::sleep_for(monitorAssets.hasRealImplementation() ? 25ms : 100ms);
		} while (hasPendingTasks());

		if (headless) {
			return;
		}
	}
}

void CheckAssetsTask::checkAllAssets(ImportAssetsDatabase& db, std::vector<Path> srcPaths, Path dstPath, std::function<EditorTaskAnchor(ImportAssetsDatabase&, Path, std::vector<ImportAssetsDatabaseEntry>&&)> importer)
{
	db.markAllAsMissing();

	Vector<ImportAssetsDatabaseEntry> assets;
	std::set<Path> included;

	// Enumerate all potential assets
	for (auto srcPath : srcPaths) {
		for (auto filePath : FileSystem::enumerateDirectory(srcPath)) {
			if (filePath.extension() != ".meta" && included.find(filePath) == included.end()) {
				included.insert(filePath);

				int64_t metaTime = 0;
				auto metaPath = srcPath / filePath;
				metaPath.replace_extension(metaPath.extension().string() + ".meta");
				if (FileSystem::exists(metaPath)) {
					metaTime = FileSystem::getLastWriteTime(metaPath);
				}

				assets.emplace_back(filePath, srcPath, FileSystem::getLastWriteTime(srcPath / filePath), metaTime);
				db.markAsPresent(filePath);
			}
		}
	}

	// Delete missing assets
	auto missing = db.getAllMissing();
	if (!missing.empty()) {
		addPendingTask(EditorTaskAnchor(std::make_unique<DeleteAssetsTask>(db, dstPath, std::move(missing))));
	}

	// Import assets
	auto toImport = filterNeedsImporting(assets);
	if (!toImport.empty()) {
		addPendingTask(importer(db, dstPath, std::move(toImport)));
	}
}

std::vector<ImportAssetsDatabaseEntry> CheckAssetsTask::filterNeedsImporting(const std::vector<ImportAssetsDatabaseEntry>& assets) const
{
	auto& db = project.getImportAssetsDatabase();
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (auto &a : assets) {
		if (db.needsImporting(a)) {
			toImport.push_back(a);
		}
	}

	return toImport;
}

EditorTaskAnchor CheckAssetsTask::importAssets(ImportAssetsDatabase& db, Path dstPath, std::vector<ImportAssetsDatabaseEntry>&& assets)
{
	return EditorTaskAnchor(std::make_unique<ImportAssetsTask>(db, dstPath, std::move(assets)));
}

EditorTaskAnchor CheckAssetsTask::importCodegen(ImportAssetsDatabase& db, Path dstPath, std::vector<ImportAssetsDatabaseEntry>&& assets)
{
	return EditorTaskAnchor(std::make_unique<ImportCodegenTask>(db, dstPath, std::move(assets)));
}
