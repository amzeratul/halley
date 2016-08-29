#include "halley/tools/assets/delete_assets_task.h"
#include "halley/tools/project/project.h"
#include "halley/tools/assets/import_assets_database.h"

using namespace Halley;

DeleteAssetsTask::DeleteAssetsTask(Project& project, Vector<ImportAssetsDatabaseEntry> assets)
	: EditorTask("Deleting assets", true, true)
	, project(project)
	, assets(assets)
{
}

void DeleteAssetsTask::run()
{
	std::cout << "Start deleting files." << std::endl;
	auto& db = project.getImportAssetsDatabase();
	auto root = project.getAssetsPath();

	for (auto& asset : assets) {
		if (isCancelled()) {
			break;
		}

		try {
			for (auto& f : asset.outputFiles) {
				FileSystem::remove(root / f);
			}
			db.markDeleted(asset);
		} catch (std::exception& e) {
			std::cout << "Error removing file " << asset.inputFile << ": " << e.what() << std::endl;
		}
	}
	db.save();
	std::cout << "Done deleting files." << std::endl;
}
