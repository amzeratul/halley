#include "import_assets_task.h"

using namespace Halley;

ImportAssetsTask::ImportAssetsTask(Vector<String>&& files, String destinationFolder)
	: EditorTask("Importing assets", true, true)
	, files(std::move(files))
	, destinationFolder(destinationFolder)
{}

void ImportAssetsTask::run()
{
	// TODO
}
