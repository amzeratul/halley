#include "import_assets_task.h"
#include "check_assets_task.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(Project& project, Vector<AssetToImport>&& files, Path destinationFolder)
	: EditorTask("Importing assets", true, true)
	, project(project)
	, files(std::move(files))
	, destinationFolder(destinationFolder)
{}

void ImportAssetsTask::run()
{
	for (size_t i = 0; i < files.size(); ++i) {
		if (isCancelled()) {
			break;
		}
		setProgress(float(i) / float(files.size()), files[i].name.filename().string());

		// TODO, just wasting time for now

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(500ms);
	}

	addContinuation(EditorTaskAnchor(std::make_unique<CheckAssetsTask>(project), 1.0f));
}
