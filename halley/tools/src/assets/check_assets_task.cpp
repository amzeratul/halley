#include <set>
#include <thread>
#include "halley/tools/assets/check_assets_task.h"
#include "halley/tools/assets/import_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"
#include "halley/tools/assets/delete_assets_task.h"
#include <boost/filesystem/operations.hpp>

using namespace Halley;
using namespace std::chrono_literals;

CheckAssetsTask::CheckAssetsTask(Project& project, bool headless)
	: EditorTask("Check assets", true, false)
	, project(project)
	, monitor(project.getAssetsSrcPath())
	, monitorShared(project.getSharedAssetsSrcPath())
	, headless(headless)
{}

void CheckAssetsTask::run()
{
	bool first = true;
	while (!isCancelled()) {
		if (first | monitor.poll() | monitorShared.poll()) { // Don't short-circuit
			first = false;
			checkAllAssets();
		}

		do {
			std::this_thread::sleep_for(monitor.hasRealImplementation() ? 25ms : 100ms);
		} while (hasPendingTasks());

		if (headless) {
			return;
		}
	}
}

void CheckAssetsTask::checkAllAssets()
{
	auto& db = project.getImportAssetsDatabase();
	db.markAllAsMissing();

	Vector<ImportAssetsDatabaseEntry> assets;
	std::set<Path> included;

	// Enumerate all potential assets
	for (auto srcPath : { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }) {
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

	// Missing
	deleteMissing(db.getAllMissing());

	// Added
	checkAssets(assets);
}

void CheckAssetsTask::checkAssets(const std::vector<ImportAssetsDatabaseEntry>& assets)
{
	auto& db = project.getImportAssetsDatabase();
	Vector<ImportAssetsDatabaseEntry> toImport;

	for (auto &a : assets) {
		if (db.needsImporting(a)) {
			toImport.push_back(a);
		}
	}

	if (!toImport.empty()) {
		addPendingTask(EditorTaskAnchor(std::make_unique<ImportAssetsTask>(project, std::move(toImport))));
	}
}

void CheckAssetsTask::deleteMissing(const std::vector<ImportAssetsDatabaseEntry>& assets)
{
	if (!assets.empty()) {
		addPendingTask(EditorTaskAnchor(std::make_unique<DeleteAssetsTask>(project, assets)));
	}
}
