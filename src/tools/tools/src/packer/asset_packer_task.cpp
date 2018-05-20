#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"

using namespace Halley;

AssetPackerTask::AssetPackerTask(Project& project, Maybe<std::set<String>> assetsToPack)
	: EditorTask("Pack Assets", true, true)
	, project(project)
	, assetsToPack(std::move(assetsToPack))
{
}

void AssetPackerTask::run()
{
	project.setAssetPackManifest(project.getRootPath() / "scripts" / "wargroove_manifest.yaml"); // HACK
	AssetPacker::pack(project, assetsToPack);

	if (!isCancelled()) {
		setProgress(1.0f, "");
	}
}
