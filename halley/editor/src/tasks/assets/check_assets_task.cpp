#include "check_assets_task.h"
#include "import_assets_task.h"

using namespace Halley;

CheckAssetsTask::CheckAssetsTask(Vector<String>&& srcFolders, String dstFolder)
	: EditorTask("Check assets", false, false)
	, srcFolders(srcFolders)
	, dstFolder(dstFolder)
{}

void CheckAssetsTask::run()
{
	Vector<String> filesToImport;
	Vector<EditorTaskAnchor> tasks;

	// TODO

	if (!filesToImport.empty()) {
		tasks.emplace_back(std::make_unique<ImportAssetsTask>(std::move(filesToImport), dstFolder));
	}

	// Schedule the next one to run after one second
	tasks.emplace_back(std::make_unique<CheckAssetsTask>(std::move(srcFolders), dstFolder), 1.0f);
	setContinuations(std::move(tasks));
}
