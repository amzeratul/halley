#include "delete_assets_task.h"
#include "src/project/project.h"
#include "import_assets_database.h"

using namespace Halley;

DeleteAssetsTask::DeleteAssetsTask(Project& project, Vector<Path> files)
	: EditorTask("Deleting assets", true, true)
	, project(project)
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
			boost::filesystem::remove(root / f);
			db.markDeleted(f);
		} catch (std::exception& e) {
			std::cout << "Error removing file " << f << ": " << e.what() << std::endl;
		}
	}
	db.save();
}
