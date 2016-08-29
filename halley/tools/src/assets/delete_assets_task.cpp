#include "halley/tools/assets/delete_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

DeleteAssetsTask::DeleteAssetsTask(Project& project, bool headless, Vector<Path> files)
	: EditorTask("Deleting assets", true, true)
	, project(project)
	, headless(headless)
	, files(files)
{
}

void DeleteAssetsTask::run()
{
	auto& db = project.getImportAssetsDatabase();
	auto root = project.getAssetsPath();

	for (auto& f : files) {
		if (isCancelled()) {
			break;
		}

		try {
			FileSystem::remove(root / f);
			db.markDeleted(f);
		} catch (std::exception& e) {
			std::cout << "Error removing file " << f << ": " << e.what() << std::endl;
		}
	}
	db.save();
}
