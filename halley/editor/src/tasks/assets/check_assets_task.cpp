#include "check_assets_task.h"
#include "import_assets_task.h"
#include "../../project/project.h"
#include <unordered_set>
#include "import_assets_database.h"
#include "delete_assets_task.h"

using namespace Halley;

CheckAssetsTask::CheckAssetsTask(Project& project)
	: EditorTask("Check assets", false, false)
	, project(project)
{}

void CheckAssetsTask::run()
{
	Vector<AssetToImport> filesToImport;
	std::unordered_set<std::string> included;

	// Prepare database
	auto& db = project.getImportAssetsDatabase();
	db.markAllAsMissing();

	// Enumerate all potential assets
	for (auto srcPath : { project.getAssetsSrcPath(), project.getSharedAssetsSrcPath() }) {
		if (boost::filesystem::exists(srcPath)) {
			using RDI = boost::filesystem::recursive_directory_iterator;
			RDI end;
			for (RDI i(srcPath); i != end; ++i) {
				Path fullPath = i->path();
				if (boost::filesystem::is_regular_file(fullPath)) {
					Path filePath = fullPath.lexically_relative(srcPath);

					std::string filename = filePath.filename().string();
					if (included.find(filename) == included.end()) {
						included.insert(filename);

						// First time we're seeing this file. Check if it needs to be imported
						auto time = boost::filesystem::last_write_time(fullPath);
						db.markAsPresent(filePath);
						if (db.needsImporting(filePath, time)) {
							filesToImport.push_back(AssetToImport(filePath, srcPath, time));
						}
					}
				}
			}
		}
	}

	if (!filesToImport.empty()) {
		addContinuation(EditorTaskAnchor(std::make_unique<ImportAssetsTask>(project, std::move(filesToImport))));
	} else {
		// Schedule the next one to run after one second
		addContinuation(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(project), 1.0f));
	}

	Vector<Path> filesToDelete = db.getAllMissing();
	if (!filesToDelete.empty()) {
		addContinuation(EditorTaskAnchor(std::make_unique<DeleteAssetsTask>(project, std::move(filesToDelete))));
	}
}
