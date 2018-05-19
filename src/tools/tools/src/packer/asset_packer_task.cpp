#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"

using namespace Halley;

AssetPackerTask::AssetPackerTask(Project& project)
	: EditorTask("Pack Assets", true, true)
	, project(project)
{
}

void AssetPackerTask::run()
{
	project.setAssetPackManifest(project.getRootPath() / "scripts" / "wargroove_manifest.yaml"); // HACK
	AssetPacker::pack(project);

	if (!isCancelled()) {
		setProgress(1.0f, "");
	}
}
